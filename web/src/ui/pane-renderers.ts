import type {
  BindingConstantEntry,
  BindingDefinitionEntry,
  BindingFunctionEntry,
} from "../bridge/console-wasm";

export interface SampleGroup {
  label: string;
  entries: string[];
}

export const sampleGroups: SampleGroup[] = [
  {
    label: "Basic",
    entries: [
      "1+2*3",
      "(2+3)^4",
      "sqrt(2)",
      "rand(10,20)",
    ],
  },
  {
    label: "Functions",
    entries: [
      "map(linspace(0,40*pi,500),sin(_))",
      "map(linspace(0,40*pi,500),sin(_)+0.2*sin(3*_))",
      "map(linspace(-10,10,400),guard(1/_,0))",
      "sort_by({-3,5,2,-19},abs(_))",
      "median({{5,1,9},{1,2,10,20}})",
    ],
  },
  {
    label: "User Functions",
    entries: [
      "f(x):(x-1)*(x+1)",
      "map(range(-4,9),f(_))",
      "g(x,y):sqrt(x*x+y*y)",
      "g(3,4)",
    ],
  },
  {
    label: "Lists And Multilists",
    entries: [
      "sum({1,2,3,4})",
      "reverse({{1,2,3},{4,5,6},{7,8}})",
      "avg({{1,2,3},{10,20,30},{2,4,8}})",
      "flatten({{1,2,3},{4,5},{6,7,8}})",
      "sort_by({3,-4,8,-1,2},abs(_))",
    ],
  },
  {
    label: "Geo",
    entries: [
      "dist(pos(59.9139,10.7522),pos(60.3913,5.3221))",
      "bearing(pos(59.9139,10.7522),pos(60.3913,5.3221))",
      "#fjord_route:{pos(70.1269,30.6399),pos(70.3444,31.4439),pos(71.1268,28.5094),pos(71.1917,27.1829),pos(71.0356,26.9015),pos(70.7858,27.1427),pos(70.5060,26.8613),pos(70.5472,26.7058),pos(70.6819,26.7622),pos(70.8940,26.8221),pos(71.1216,26.5917),pos(71.2084,25.9882),pos(71.1505,24.5959),pos(70.7172,22.0326),pos(70.5468,21.5785)}",
      "offset_path(fjord_route,10000,15000)",
      "#norway_areas:{{pos(59.9139,10.7522),pos(60.5867,10.2136),pos(61.0667,11.2936),pos(60.3227,11.9736),pos(59.9139,10.7522)},{pos(60.39299,5.32415),pos(60.86507,4.17095),pos(61.53707,5.13095),pos(60.83307,6.45095),pos(60.39299,5.32415)},{pos(63.4305,10.3951),pos(64.0665,9.1543),pos(64.9065,10.5143),pos(64.0265,11.9143),pos(63.4305,10.3951)}}",
      "rotate_path(norway_areas,15)",
      "scale_path(norway_areas,1.2)",
    ],
  },
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

  const categoryOrder = ["scalar", "position", "list", "statistics", "list_generation"];
  const categoryLabels = new Map<string, string>([
    ["scalar", "Scalar functions"],
    ["position", "Position functions"],
    ["list", "List functions"],
    ["statistics", "Statistics functions"],
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

export function renderSampleList(
  container: HTMLElement,
  groups: SampleGroup[],
  onSampleSelected?: (expression: string) => void,
): void {
  const expandedState = new Map<string, boolean>();
  for (const section of Array.from(container.querySelectorAll<HTMLElement>(".pane-group"))) {
    const label = section.dataset.groupLabel;
    const toggle = section.querySelector<HTMLElement>(".pane-group-toggle");
    if (label === undefined || toggle === null) {
      continue;
    }
    expandedState.set(label, toggle.getAttribute("aria-expanded") === "true");
  }

  container.replaceChildren();
  if (groups.length === 0) {
    const empty = document.createElement("div");
    empty.className = "pane-empty";
    empty.textContent = "Empty";
    container.append(empty);
    return;
  }

  for (const group of groups) {
    if (group.entries.length === 0) {
      continue;
    }

    const section = document.createElement("section");
    section.className = "pane-group";
    section.dataset.groupLabel = group.label;

    const toggle = document.createElement("button");
    toggle.type = "button";
    toggle.className = "pane-group-toggle";

    const marker = document.createElement("span");
    marker.className = "pane-group-marker";

    const heading = document.createElement("span");
    heading.className = "pane-group-heading";
    heading.textContent = group.label;

    const body = document.createElement("div");
    body.className = "pane-group-body sample-group-body";

    const expanded = expandedState.get(group.label) === true;
    body.hidden = !expanded;
    toggle.setAttribute("aria-expanded", expanded ? "true" : "false");
    marker.textContent = expanded ? "−" : "+";

    toggle.addEventListener("click", () => {
      const expanded = body.hidden;
      body.hidden = !expanded;
      toggle.setAttribute("aria-expanded", expanded ? "true" : "false");
      marker.textContent = expanded ? "−" : "+";
    });

    for (const expression of group.entries) {
      const button = document.createElement("button");
      button.type = "button";
      button.className = "sample-button";
      button.textContent = expression;
      button.title = expression;
      button.addEventListener("click", () => {
        onSampleSelected?.(expression);
      });
      body.append(button);
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
