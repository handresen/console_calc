import type {
  BindingConstantEntry,
  BindingDefinitionEntry,
  BindingFunctionEntry,
  BindingSnapshot,
  BindingStackEntry,
} from "../bridge/console-wasm";

export interface PanesView {
  element: HTMLElement;
  render(snapshot: BindingSnapshot): void;
}

const sampleExpressions = [
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

interface PlotSeries {
  key: string;
  label: string;
  values: number[];
  truncated: boolean;
}

function isPlotSeries(value: PlotSeries | null): value is PlotSeries {
  return value !== null;
}

function renderTextList(container: HTMLElement, values: string[]): void {
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

function renderFunctionTable(
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

  const table = document.createElement("table");
  table.className = "function-table";

  const body = document.createElement("tbody");
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

    const headingRow = document.createElement("tr");
    headingRow.className = "function-category-row";

    const heading = document.createElement("th");
    heading.className = "function-category-heading";
    heading.colSpan = 2;
    heading.scope = "colgroup";
    heading.textContent = categoryLabels.get(category) ?? category;

    headingRow.append(heading);
    body.append(headingRow);

    for (const value of entries) {
      const row = document.createElement("tr");
      row.className = "function-entry-row";

      const signature = document.createElement("td");
      signature.className = "function-signature";
      signature.textContent = value.signature;

      const summary = document.createElement("td");
      summary.className = "function-summary";
      summary.textContent = value.summary;

      row.append(signature, summary);
      body.append(row);
    }
  }

  table.append(body);
  container.append(table);
}

function stackDisplay(entry: BindingStackEntry): string {
  return `${entry.level}:${entry.display}`;
}

function definitionDisplay(entry: BindingDefinitionEntry): string {
  return `${entry.name}:${entry.expression}`;
}

function constantDisplay(entry: BindingConstantEntry): string {
  return `${entry.name}:${entry.value}`;
}

function parsePlotSeries(entry: BindingStackEntry): PlotSeries | null {
  const listValues = entry.list_values ?? [];
  if (listValues.length === 0 && !entry.display.trim().startsWith("{}")) {
    return null;
  }
  const values =
    listValues.length <= 500
      ? listValues
      : Array.from({ length: 500 }, (_, index) => {
          const sourceIndex = Math.round((index / 499) * (listValues.length - 1));
          return listValues[sourceIndex] ?? 0;
        });
  const truncated = listValues.length > 500;

  return {
    key: `stack-${entry.level}`,
    label: `Stack ${entry.level}`,
    values,
    truncated,
  };
}

function buildPlotPath(values: number[], width: number, height: number): string {
  if (values.length === 0) {
    return "";
  }

  if (values.length === 1) {
    const y = height / 2;
    return `M 0 ${y} L ${width} ${y}`;
  }

  const min = Math.min(...values);
  const max = Math.max(...values);
  const range = max - min || 1;

  return values
    .map((value, index) => {
      const x = (index / (values.length - 1)) * width;
      const y = height - ((value - min) / range) * height;
      return `${index === 0 ? "M" : "L"} ${x.toFixed(2)} ${y.toFixed(2)}`;
    })
    .join(" ");
}

function buildZeroAxisPath(values: number[], width: number, height: number): string {
  if (values.length === 0) {
    return `M 0 ${height} L ${width} ${height}`;
  }

  const min = Math.min(...values);
  const max = Math.max(...values);

  if (min > 0 || max < 0) {
    return "";
  }

  const range = max - min || 1;
  const y = height - ((0 - min) / range) * height;
  return `M 0 ${y.toFixed(2)} L ${width} ${y.toFixed(2)}`;
}

interface PaneElements {
  section: HTMLElement;
  body: HTMLElement;
  count: HTMLElement;
}

function createPane(titleText: string): PaneElements {
  const section = document.createElement("section");
  section.className = "pane";

  const title = document.createElement("button");
  title.className = "pane-title";
  title.type = "button";
  title.setAttribute("aria-expanded", "false");

  const titleLabel = document.createElement("span");
  titleLabel.className = "pane-title-label";
  titleLabel.textContent = titleText;

  const count = document.createElement("span");
  count.className = "pane-count";
  count.textContent = "0";

  const marker = document.createElement("span");
  marker.className = "pane-marker";
  marker.textContent = "+";

  const body = document.createElement("div");
  body.className = "pane-body";
  body.hidden = true;

  title.addEventListener("click", () => {
    const expanded = title.getAttribute("aria-expanded") !== "false";
    title.setAttribute("aria-expanded", expanded ? "false" : "true");
    body.hidden = expanded;
    marker.textContent = expanded ? "+" : "−";
  });

  title.append(titleLabel, count, marker);
  section.append(title, body);
  return { section, body, count };
}

export function createPanesView(onSampleSelected?: (expression: string) => void): PanesView {
  const section = document.createElement("section");
  section.className = "panes-view";

  const infoPanel = document.createElement("section");
  infoPanel.className = "info-panel";

  const plotPanel = document.createElement("section");
  plotPanel.className = "plot-panel";

  const status = document.createElement("div");
  status.className = "pane-status";

  const stackPane = createPane("Stack");
  const definitionsPane = createPane("Definitions");
  const constantsPane = createPane("Constants");
  const functionsPane = createPane("Functions");
  const samplesPane = createPane("Samples");
  const plotPane = createPane("Plot");

  functionsPane.body.classList.add("functions-pane-body");

  const functionTableContainer = document.createElement("div");
  functionTableContainer.className = "function-table-container";

  const samplesList = document.createElement("div");
  samplesList.className = "sample-list";

  for (const expression of sampleExpressions) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "sample-button";
    button.textContent = expression;
    button.addEventListener("click", () => {
      onSampleSelected?.(expression);
    });
    samplesList.append(button);
  }

  functionsPane.body.append(functionTableContainer);
  samplesPane.body.append(samplesList);

  const plotMeta = document.createElement("div");
  plotMeta.className = "plot-meta";

  const plotSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  plotSvg.setAttribute("viewBox", "0 0 320 180");
  plotSvg.setAttribute("preserveAspectRatio", "none");
  plotSvg.classList.add("plot-svg");

  const plotGrid = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotGrid.setAttribute("d", "M 0 0 L 0 180 M 0 180 L 320 180");
  plotGrid.setAttribute("class", "plot-grid");

  const plotZeroAxis = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotZeroAxis.setAttribute("class", "plot-zero-axis");

  const plotLine = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotLine.setAttribute("class", "plot-line");

  plotSvg.append(plotGrid, plotZeroAxis, plotLine);
  plotPane.body.append(plotMeta, plotSvg);

  const renderPlot = (seriesList: PlotSeries[]) => {
    plotPane.count.textContent = `${seriesList.length}`;

    if (seriesList.length === 0) {
      plotMeta.textContent = "No list values in stack";
      plotZeroAxis.setAttribute("d", "");
      plotLine.setAttribute("d", "");
      return;
    }

    const currentSeries = seriesList[seriesList.length - 1];
    const suffix = currentSeries.truncated ? " | using visible values" : "";
    plotMeta.textContent = `${currentSeries.label} | ${currentSeries.values.length} points${suffix}`;
    plotZeroAxis.setAttribute("d", buildZeroAxisPath(currentSeries.values, 320, 180));
    plotLine.setAttribute("d", buildPlotPath(currentSeries.values, 320, 180));
  };

  infoPanel.append(
    status,
    stackPane.section,
    definitionsPane.section,
    constantsPane.section,
    functionsPane.section,
    samplesPane.section,
  );

  plotPanel.append(
    plotPane.section,
  );

  section.append(infoPanel, plotPanel);

  return {
    element: section,
    render(snapshot) {
      status.textContent = `Mode ${snapshot.display_mode} | stack ${snapshot.stack.length}/${snapshot.max_stack_depth}`;
      stackPane.count.textContent = `${snapshot.stack.length}`;
      definitionsPane.count.textContent = `${snapshot.definitions.length}`;
      constantsPane.count.textContent = `${snapshot.constants.length}`;
      functionsPane.count.textContent = `${snapshot.functions.length}`;
      samplesPane.count.textContent = `${sampleExpressions.length}`;
      renderTextList(
        stackPane.body,
        snapshot.stack.map((entry) => stackDisplay(entry)),
      );
      renderTextList(
        definitionsPane.body,
        snapshot.definitions.map((entry) => definitionDisplay(entry)),
      );
      renderTextList(
        constantsPane.body,
        snapshot.constants.map((entry) => constantDisplay(entry)),
      );
      renderFunctionTable(functionTableContainer, snapshot.functions);
      renderPlot(snapshot.stack.map((entry) => parsePlotSeries(entry)).filter(isPlotSeries));
    },
  };
}
