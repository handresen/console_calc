import type {
  BindingConstantEntry,
  BindingDefinitionEntry,
  BindingFunctionEntry,
} from "../bridge/console-wasm";

export const sampleExpressions = [
  "map(linspace(0,40*pi,500),sin(_))",
  "map(linspace(0,40*pi,500),sin(_)+0.2*sin(3*_))",
  "map(linspace(-10,10,400),guard(1/_,0))",
  "map(linspace(0,4*pi,500),guard(1/sin(_),0))",
  "map(linspace(-3,3,400),_ * _)",
  "dist(pos(59.9139,10.7522),pos(60.3913,5.3221))",
  "bearing(pos(59.9139,10.7522),pos(60.3913,5.3221))",
  "br_to_pos(pos(59.9139,10.7522),270,100000)",
  "lon(br_to_pos(pos(0,0),90,111319.49079327357))",
];

export interface ConstantDisplayRow {
  kind: "heading" | "entry";
  text: string;
}

interface ConstantGroup {
  namespace: string;
  label: string;
  entries: BindingConstantEntry[];
}

export function renderTextList(container: HTMLElement, values: string[]): void {
  container.replaceChildren();
  if (values.length === 0) {
    const empty = document.createElement("div");
    empty.className = "pane-empty";
    empty.textContent = "Empty";
    container.append(empty);
    return;
  }

  for (const value of values) {
    const line = document.createElement("div");
    line.className = "pane-line";
    line.textContent = value;
    container.append(line);
  }
}

function constantNamespace(name: string): string {
  const separator = name.indexOf(".");
  return separator === -1 ? "root" : name.slice(0, separator);
}

function constantNamespaceLabel(namespace: string): string {
  switch (namespace) {
    case "root":
      return "Math (root)";
    case "m":
      return "Math (m)";
    case "c":
      return "Conversions (c)";
    case "ph":
      return "Physical (ph)";
    default:
      return namespace;
  }
}

function groupConstants(values: BindingConstantEntry[]): ConstantGroup[] {
  const groups: ConstantGroup[] = [];
  let currentGroup: ConstantGroup | null = null;
  for (const value of values) {
    const namespace = constantNamespace(value.name);
    if (currentGroup === null || currentGroup.namespace !== namespace) {
      currentGroup = {
        namespace,
        label: constantNamespaceLabel(namespace),
        entries: [],
      };
      groups.push(currentGroup);
    }
    currentGroup.entries.push(value);
  }
  return groups;
}

export function constantDisplayRows(
  values: BindingConstantEntry[],
): ConstantDisplayRow[] {
  const rows: ConstantDisplayRow[] = [];
  for (const group of groupConstants(values)) {
    rows.push({ kind: "heading", text: group.label });
    for (const value of group.entries) {
      rows.push({ kind: "entry", text: constantDisplay(value) });
    }
  }
  return rows;
}

export function renderConstantList(
  container: HTMLElement,
  values: BindingConstantEntry[],
): void {
  container.replaceChildren();
  if (values.length === 0) {
    const empty = document.createElement("div");
    empty.className = "pane-empty";
    empty.textContent = "Empty";
    container.append(empty);
    return;
  }

  for (const group of groupConstants(values)) {
    const section = document.createElement("section");
    section.className = "pane-group";

    const toggle = document.createElement("button");
    toggle.type = "button";
    toggle.className = "pane-group-toggle";
    toggle.setAttribute("aria-expanded", "false");

    const marker = document.createElement("span");
    marker.className = "pane-group-marker";
    marker.textContent = "+";

    const label = document.createElement("span");
    label.className = "pane-group-heading";
    label.textContent = group.label;

    const body = document.createElement("div");
    body.className = "pane-group-body";
    body.hidden = true;

    toggle.addEventListener("click", () => {
      const expanded = body.hidden;
      body.hidden = !expanded;
      toggle.setAttribute("aria-expanded", expanded ? "true" : "false");
      marker.textContent = expanded ? "−" : "+";
    });

    for (const value of group.entries) {
      const line = document.createElement("div");
      line.className = "pane-line";
      line.textContent = constantDisplay(value);
      body.append(line);
    }

    toggle.append(marker, label);
    section.append(toggle, body);
    container.append(section);
  }
}

export function renderFunctionTable(
  container: HTMLElement,
  values: BindingFunctionEntry[],
): void {
  container.replaceChildren();
  if (values.length === 0) {
    const empty = document.createElement("div");
    empty.className = "pane-empty";
    empty.textContent = "Empty";
    container.append(empty);
    return;
  }

  const categoryOrder = ["scalar", "position", "list", "list_generation"];
  const categoryLabels = new Map<string, string>([
    ["scalar", "Scalar functions"],
    ["position", "Position functions"],
    ["list", "List functions"],
    ["list_generation", "List generation functions"],
  ]);
  const groupedValues = new Map<string, BindingFunctionEntry[]>();

  for (const value of values) {
    const group = groupedValues.get(value.category) ?? [];
    group.push(value);
    groupedValues.set(value.category, group);
  }

  const orderedCategories = [
    ...categoryOrder.filter((category) => groupedValues.has(category)),
    ...Array.from(groupedValues.keys()).filter(
      (category) => !categoryOrder.includes(category),
    ),
  ];

  for (const category of orderedCategories) {
    const entries = groupedValues.get(category) ?? [];
    if (entries.length === 0) {
      continue;
    }

    const section = document.createElement("section");
    section.className = "pane-group";

    const toggle = document.createElement("button");
    toggle.type = "button";
    toggle.className = "pane-group-toggle";
    toggle.setAttribute("aria-expanded", "false");

    const marker = document.createElement("span");
    marker.className = "pane-group-marker";
    marker.textContent = "+";

    const heading = document.createElement("span");
    heading.className = "pane-group-heading";
    heading.textContent = categoryLabels.get(category) ?? category;

    const body = document.createElement("div");
    body.className = "pane-group-body function-group-body";
    body.hidden = true;

    toggle.addEventListener("click", () => {
      const expanded = body.hidden;
      body.hidden = !expanded;
      toggle.setAttribute("aria-expanded", expanded ? "true" : "false");
      marker.textContent = expanded ? "−" : "+";
    });

    for (const value of entries) {
      const row = document.createElement("div");
      row.className = "function-entry-row";

      const signature = document.createElement("div");
      signature.className = "function-signature";
      signature.textContent = value.signature;

      const summary = document.createElement("div");
      summary.className = "function-summary";
      summary.textContent = value.summary;

      row.append(signature, summary);
      body.append(row);
    }

    toggle.append(marker, heading);
    section.append(toggle, body);
    container.append(section);
  }
}

export function definitionDisplay(entry: BindingDefinitionEntry): string {
  return `${entry.name}:${entry.expression}`;
}

export function constantDisplay(entry: BindingConstantEntry): string {
  return `${entry.name}:${entry.value}`;
}
