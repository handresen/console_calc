import type {
  BindingPositionEntry,
  BindingStackEntry,
} from "../bridge/console-wasm";
import type { DisplaySettings } from "./display-settings";
import { formatNumericText } from "./display-settings";

function formatScalarExact(value: number): string {
  return `${value}`;
}

function formatPositionExact(value: BindingPositionEntry): string {
  return `pos(${formatScalarExact(value.latitude_deg)}, ${formatScalarExact(value.longitude_deg)})`;
}

function formatScalarListExact(values: number[]): string {
  return `{${values.map(formatScalarExact).join(", ")}}`;
}

function formatPositionListExact(values: BindingPositionEntry[]): string {
  return `{${values.map(formatPositionExact).join(", ")}}`;
}

export function stackEntryExactValue(entry: BindingStackEntry): string {
  const multiPositionListValues = entry.multi_position_list_values ?? [];
  if (multiPositionListValues.length > 0) {
    return `{${multiPositionListValues.map(formatPositionListExact).join(", ")}}`;
  }

  const multiListValues = entry.multi_list_values ?? [];
  if (multiListValues.length > 0) {
    return `{${multiListValues.map(formatScalarListExact).join(", ")}}`;
  }

  const positionListValues = entry.position_list_values ?? [];
  if (positionListValues.length > 0) {
    return formatPositionListExact(positionListValues);
  }

  const listValues = entry.list_values ?? [];
  if (listValues.length > 0 || entry.display.trim() === "{}") {
    return formatScalarListExact(listValues);
  }

  if (entry.position !== undefined && entry.position !== null) {
    return formatPositionExact(entry.position);
  }

  return entry.display;
}

export function stackDisplay(
  entry: BindingStackEntry,
  settings: DisplaySettings,
): string {
  const formatScalarPreview = (value: number) =>
    formatNumericText(`${value}`, settings.stackDecimals);
  const formatPositionPreview = (value: BindingPositionEntry) =>
    `pos(${formatNumericText(`${value.latitude_deg}`, settings.stackDecimals)}, ${formatNumericText(`${value.longitude_deg}`, settings.stackDecimals)})`;
  const formatScalarListPreview = (values: number[]) => {
    const preview = values
      .slice(0, settings.listPreviewLength)
      .map(formatScalarPreview)
      .join(", ");
    const suffix =
      values.length > settings.listPreviewLength
        ? `, ... <${values.length - settings.listPreviewLength} more>`
        : "";
    return `{${preview}${suffix}}`;
  };
  const formatPositionListPreview = (values: BindingPositionEntry[]) => {
    const preview = values
      .slice(0, settings.listPreviewLength)
      .map(formatPositionPreview)
      .join(", ");
    const suffix =
      values.length > settings.listPreviewLength
        ? `, ... <${values.length - settings.listPreviewLength} more>`
        : "";
    return `{${preview}${suffix}}`;
  };

  const multiPositionListValues = entry.multi_position_list_values ?? [];
  if (multiPositionListValues.length > 0) {
    const preview = multiPositionListValues
      .slice(0, settings.listPreviewLength)
      .map(formatPositionListPreview)
      .join(", ");
    const suffix =
      multiPositionListValues.length > settings.listPreviewLength
        ? `, ... <${multiPositionListValues.length - settings.listPreviewLength} more>`
        : "";
    return `${entry.level}:{${preview}${suffix}}`;
  }

  const multiListValues = entry.multi_list_values ?? [];
  if (multiListValues.length > 0) {
    const preview = multiListValues
      .slice(0, settings.listPreviewLength)
      .map(formatScalarListPreview)
      .join(", ");
    const suffix =
      multiListValues.length > settings.listPreviewLength
        ? `, ... <${multiListValues.length - settings.listPreviewLength} more>`
        : "";
    return `${entry.level}:{${preview}${suffix}}`;
  }

  const positionListValues = entry.position_list_values ?? [];
  if (positionListValues.length > 0) {
    return `${entry.level}:${formatPositionListPreview(positionListValues)}`;
  }

  const listValues = entry.list_values ?? [];
  if (listValues.length > 0 || entry.display.trim() === "{}") {
    return `${entry.level}:${formatScalarListPreview(listValues)}`;
  }

  return `${entry.level}:${formatNumericText(entry.display, settings.stackDecimals)}`;
}

export function renderStackList(
  container: HTMLElement,
  values: BindingStackEntry[],
  settings: DisplaySettings,
): void {
  container.replaceChildren();
  if (values.length === 0) {
    const empty = document.createElement("div");
    empty.className = "pane-empty";
    empty.textContent = "Empty";
    container.append(empty);
    return;
  }

  for (const entry of values) {
    const row = document.createElement("div");
    row.className = "stack-row";

    const copyButton = document.createElement("button");
    copyButton.type = "button";
    copyButton.className = "stack-copy-button";
    copyButton.textContent = "⧉";
    copyButton.setAttribute("aria-label", `Copy stack ${entry.level}`);
    copyButton.title = "Copy value";
    copyButton.addEventListener("click", async () => {
      const previousLabel = copyButton.textContent;
      try {
        await navigator.clipboard.writeText(stackEntryExactValue(entry));
        copyButton.textContent = "✓";
        globalThis.setTimeout(() => {
          copyButton.textContent = previousLabel;
        }, 900);
      } catch {
        copyButton.textContent = "!";
        globalThis.setTimeout(() => {
          copyButton.textContent = previousLabel;
        }, 900);
      }
    });

    const text = document.createElement("div");
    text.className = "pane-line stack-row-text";
    text.textContent = stackDisplay(entry, settings);

    row.append(copyButton, text);
    container.append(row);
  }
}
