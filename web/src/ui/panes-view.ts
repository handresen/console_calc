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

function stackDisplay(entry: BindingStackEntry): string {
  return `${entry.level}:${entry.display}`;
}

function definitionDisplay(entry: BindingDefinitionEntry): string {
  return `${entry.name}:${entry.expression}`;
}

function constantDisplay(entry: BindingConstantEntry): string {
  return `${entry.name}:${entry.value}`;
}

function functionDisplay(entry: BindingFunctionEntry): string {
  return `${entry.signature} - ${entry.summary}`;
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

export function createPanesView(): PanesView {
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
  const plotPane = createPane("Plot");

  const plotMeta = document.createElement("div");
  plotMeta.className = "plot-meta";

  const plotSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  plotSvg.setAttribute("viewBox", "0 0 320 180");
  plotSvg.setAttribute("preserveAspectRatio", "none");
  plotSvg.classList.add("plot-svg");

  const plotGrid = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotGrid.setAttribute("d", "M 0 90 L 320 90 M 0 0 L 0 180 M 0 180 L 320 180");
  plotGrid.setAttribute("class", "plot-grid");

  const plotLine = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotLine.setAttribute("class", "plot-line");

  plotSvg.append(plotGrid, plotLine);
  plotPane.body.append(plotMeta, plotSvg);

  const renderPlot = (seriesList: PlotSeries[]) => {
    plotPane.count.textContent = `${seriesList.length}`;

    if (seriesList.length === 0) {
      plotMeta.textContent = "No list values in stack";
      plotLine.setAttribute("d", "");
      return;
    }

    const currentSeries = seriesList[seriesList.length - 1];
    const suffix = currentSeries.truncated ? " | using visible values" : "";
    plotMeta.textContent = `${currentSeries.label} | ${currentSeries.values.length} points${suffix}`;
    plotLine.setAttribute("d", buildPlotPath(currentSeries.values, 320, 180));
  };

  infoPanel.append(
    status,
    stackPane.section,
    definitionsPane.section,
    constantsPane.section,
    functionsPane.section,
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
      renderTextList(
        functionsPane.body,
        snapshot.functions.map((entry) => functionDisplay(entry)),
      );
      renderPlot(snapshot.stack.map((entry) => parsePlotSeries(entry)).filter(isPlotSeries));
    },
  };
}
