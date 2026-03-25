import type { BindingCommandResult } from "../bridge/console-wasm";
import { formatNumericText } from "./display-settings";
import { constantDisplayRows } from "./pane-renderers";
import type { TranscriptView } from "./transcript-view";

const maxTranscriptMultiListChars = 84;

function countTopLevelItems(content: string): number {
  const trimmed = content.trim();
  if (trimmed.length === 0) {
    return 0;
  }

  let depthBraces = 0;
  let depthParens = 0;
  let items = 1;
  for (const char of trimmed) {
    if (char === "{") {
      depthBraces += 1;
    } else if (char === "}") {
      depthBraces = Math.max(0, depthBraces - 1);
    } else if (char === "(") {
      depthParens += 1;
    } else if (char === ")") {
      depthParens = Math.max(0, depthParens - 1);
    } else if (char === "," && depthBraces === 0 && depthParens === 0) {
      items += 1;
    }
  }
  return items;
}

function splitTopLevelInnerLists(payload: string): string[] | null {
  if (!payload.startsWith("{{") || !payload.endsWith("}}")) {
    return null;
  }

  const inner = payload.slice(1, -1);
  const lists: string[] = [];
  let segmentStart = -1;
  let braceDepth = 0;
  let parenDepth = 0;

  for (let index = 0; index < inner.length; index += 1) {
    const char = inner[index];
    if (char === "(") {
      parenDepth += 1;
      continue;
    }
    if (char === ")") {
      parenDepth = Math.max(0, parenDepth - 1);
      continue;
    }
    if (parenDepth > 0) {
      continue;
    }
    if (char === "{") {
      if (braceDepth === 0) {
        segmentStart = index;
      }
      braceDepth += 1;
      continue;
    }
    if (char === "}") {
      braceDepth -= 1;
      if (braceDepth === 0 && segmentStart >= 0) {
        lists.push(inner.slice(segmentStart, index + 1));
        segmentStart = -1;
      }
    }
  }

  return lists.length > 0 ? lists : null;
}

function summarizeMultiListPayload(payload: string): string {
  if (payload.length <= maxTranscriptMultiListChars) {
    return payload;
  }

  const innerLists = splitTopLevelInnerLists(payload);
  if (innerLists === null || innerLists.length === 0) {
    return payload;
  }

  const totalItems = innerLists.reduce(
    (sum, list) => sum + countTopLevelItems(list.slice(1, -1)),
    0,
  );

  let preview = "{";
  let shown = 0;
  for (const innerList of innerLists) {
    const candidate = `${preview}${shown === 0 ? "" : ","}${innerList}`;
    if (
      shown >= 2 ||
      `${candidate},<${innerLists.length - shown - 1} more lists, ${totalItems} items total>}`
        .length > maxTranscriptMultiListChars
    ) {
      break;
    }
    preview = candidate;
    shown += 1;
  }

  if (shown === 0) {
    shown = 1;
    preview = `{${innerLists[0]}`;
  }

  const remaining = innerLists.length - shown;
  if (remaining <= 0) {
    return payload;
  }

  return `${preview},<${remaining} more lists, ${totalItems} items total>}`;
}

export function formatTranscriptText(text: string, decimals: number): string {
  const formatted = formatNumericText(text, decimals);
  const multiListIndex = formatted.indexOf("{{");
  if (multiListIndex < 0) {
    return formatted;
  }

  const prefix = formatted.slice(0, multiListIndex);
  const payload = formatted.slice(multiListIndex);
  return `${prefix}${summarizeMultiListPayload(payload)}`;
}

function expectedFunctionSignature(
  result: BindingCommandResult,
  input?: string,
): string | null {
  for (const event of result.events) {
    if (event.kind === "error" && event.error?.expected_signature) {
      return event.error.expected_signature;
    }
  }

  if (input === undefined) {
    return null;
  }

  const match = input.match(/^\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(/);
  if (match === null) {
    return null;
  }

  const functionName = match[1];
  if (functionName === undefined) {
    return null;
  }

  const functionEntry = result.snapshot.functions.find((entry) => entry.name === functionName);
  return functionEntry?.signature ?? null;
}

export function renderTranscriptResult(
  transcript: TranscriptView,
  result: BindingCommandResult,
  input?: string,
  elapsedMs?: number,
  transcriptDecimals = 8,
): void {
  let renderedEvent = false;
  let timingRendered = false;
  const timingText = elapsedMs === undefined ? undefined : `${elapsedMs.toFixed(1)} ms`;
  const isListValueText = (text: string) => text.trimStart().startsWith("{");

  const appendMessage = (text: string, kind: string, extraClassName?: string): void => {
    transcript.appendMessage(
      formatTranscriptText(text, transcriptDecimals),
      kind,
      timingRendered ? undefined : timingText,
      extraClassName,
    );
    timingRendered = true;
  };

  for (const event of result.events) {
    renderedEvent = true;
    switch (event.kind) {
      case "value":
        appendMessage(
          input === undefined ? event.text : `${input} = ${event.text}`,
          "value",
          isListValueText(event.text) ? "transcript-line-value-list" : undefined,
        );
        break;
      case "error": {
        appendMessage(`error: ${event.text}`, "error");
        const signature = expectedFunctionSignature(result, input);
        if (signature !== null && !event.text.includes(signature)) {
          appendMessage(`expected: ${signature}`, "listing");
        }
        break;
      }
      case "text":
        appendMessage(event.text, "text");
        break;
      case "stack_listing":
        for (const entry of event.stack) {
          appendMessage(`${entry.level}:${entry.display}`, "listing");
        }
        break;
      case "definition_listing":
        for (const entry of event.definitions) {
          appendMessage(`${entry.name}:${entry.expression}`, "listing");
        }
        break;
      case "constant_listing":
        for (const row of constantDisplayRows(event.constants)) {
          appendMessage(
            row.text,
            "listing",
            row.kind === "heading" ? "transcript-line-listing-heading" : undefined,
          );
        }
        break;
      case "function_listing":
        for (const entry of event.functions) {
          appendMessage(`${entry.signature} - ${entry.summary}`, "listing");
        }
        break;
      default:
        appendMessage(event.text, "text");
        break;
    }
  }

  if (!renderedEvent && input !== undefined && input.includes(":")) {
    appendMessage(input, "text");
  }
}
