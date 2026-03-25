import type { BindingStackEntry } from "../bridge/console-wasm";
import type { DisplaySettings } from "./display-settings";
import { createPane } from "./pane-controls";
import type { PaneElements } from "./pane-controls";
import {
  buildPlotPath,
  buildScalarGroupBounds,
  buildScalarBounds,
  buildScalarPlotPoints,
  buildZeroAxisPath,
  formatScalarAxisLabel,
  isPlotGroup,
  parsePlotGroup,
} from "./plot-support";
import type { PlotGroup, PlotItem, PlotPoint, PlotSeries } from "./plot-support";

export interface PlotPaneView {
  pane: PaneElements;
  render(stack: BindingStackEntry[]): void;
  setDisplaySettings(settings: DisplaySettings): void;
}

export function createPlotPaneView(
  onToggle: (expanded: boolean) => void,
  initialDisplaySettings: DisplaySettings,
): PlotPaneView {
  const pane = createPane("Plot", onToggle);
  let displaySettings = initialDisplaySettings;
  let latestStack: BindingStackEntry[] = [];
  let latestPlotGroup: PlotGroup | null = null;
  let latestHoverSeries: PlotItem | null = null;
  let latestPlotPoints: PlotPoint[] = [];

  const plotMeta = document.createElement("div");
  plotMeta.className = "plot-meta";

  const plotControls = document.createElement("div");
  plotControls.className = "plot-controls";

  const lineToggleLabel = document.createElement("label");
  lineToggleLabel.className = "plot-toggle";

  const lineToggle = document.createElement("input");
  lineToggle.type = "checkbox";
  lineToggle.checked = displaySettings.plotDefaultLine;

  const lineToggleText = document.createElement("span");
  lineToggleText.textContent = "Line";

  lineToggleLabel.append(lineToggle, lineToggleText);

  const pointsToggleLabel = document.createElement("label");
  pointsToggleLabel.className = "plot-toggle";

  const pointsToggle = document.createElement("input");
  pointsToggle.type = "checkbox";
  pointsToggle.checked = displaySettings.plotDefaultPoints;

  const pointsToggleText = document.createElement("span");
  pointsToggleText.textContent = "Points";

  pointsToggleLabel.append(pointsToggle, pointsToggleText);
  plotControls.append(lineToggleLabel, pointsToggleLabel);

  const plotSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  plotSvg.setAttribute("viewBox", "0 0 320 180");
  plotSvg.setAttribute("preserveAspectRatio", "xMidYMid meet");
  plotSvg.classList.add("plot-svg");

  const plotGrid = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotGrid.setAttribute("d", "M 0 0 L 0 180 M 0 180 L 320 180");
  plotGrid.setAttribute("class", "plot-grid");

  const plotZeroAxis = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotZeroAxis.setAttribute("class", "plot-zero-axis");

  const plotPrimeMeridian = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotPrimeMeridian.setAttribute("class", "plot-reference-axis");

  const plotLines = document.createElementNS("http://www.w3.org/2000/svg", "g");
  plotLines.setAttribute("class", "plot-lines");

  const plotPoints = document.createElementNS("http://www.w3.org/2000/svg", "g");
  plotPoints.setAttribute("class", "plot-points");

  const plotCornerLabels = document.createElementNS("http://www.w3.org/2000/svg", "g");
  plotCornerLabels.setAttribute("class", "plot-corner-labels");

  const plotHover = document.createElementNS("http://www.w3.org/2000/svg", "g");
  plotHover.setAttribute("class", "plot-hover");

  const plotHoverGuide = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotHoverGuide.setAttribute("class", "plot-hover-guide");

  const plotHoverPoint = document.createElementNS("http://www.w3.org/2000/svg", "circle");
  plotHoverPoint.setAttribute("class", "plot-hover-point");
  plotHoverPoint.setAttribute("r", "3.2");

  const plotHoverBubble = document.createElementNS("http://www.w3.org/2000/svg", "rect");
  plotHoverBubble.setAttribute("class", "plot-hover-bubble");
  plotHoverBubble.setAttribute("rx", "4");
  plotHoverBubble.setAttribute("ry", "4");

  const plotHoverLabel = document.createElementNS("http://www.w3.org/2000/svg", "text");
  plotHoverLabel.setAttribute("class", "plot-hover-label");

  const plotHoverXAxisBubble = document.createElementNS(
    "http://www.w3.org/2000/svg",
    "rect",
  );
  plotHoverXAxisBubble.setAttribute("class", "plot-hover-bubble");
  plotHoverXAxisBubble.setAttribute("rx", "4");
  plotHoverXAxisBubble.setAttribute("ry", "4");

  const plotHoverXAxisLabel = document.createElementNS(
    "http://www.w3.org/2000/svg",
    "text",
  );
  plotHoverXAxisLabel.setAttribute("class", "plot-hover-label");

  plotHover.append(
    plotHoverGuide,
    plotHoverBubble,
    plotHoverXAxisBubble,
    plotHoverPoint,
    plotHoverLabel,
    plotHoverXAxisLabel,
  );
  plotHover.style.display = "none";

  const createCornerLabel = (x: string, y: string, anchor: string, baseline: string) => {
    const label = document.createElementNS("http://www.w3.org/2000/svg", "text");
    label.setAttribute("x", x);
    label.setAttribute("y", y);
    label.setAttribute("text-anchor", anchor);
    label.setAttribute("dominant-baseline", baseline);
    label.setAttribute("class", "plot-corner-label");
    return label;
  };

  const topLeftLabel = createCornerLabel("6", "6", "start", "hanging");
  const topRightLabel = createCornerLabel("314", "6", "end", "hanging");
  const bottomLeftLabel = createCornerLabel("6", "174", "start", "auto");
  const bottomRightLabel = createCornerLabel("314", "174", "end", "auto");

  plotCornerLabels.append(
    topLeftLabel,
    topRightLabel,
    bottomLeftLabel,
    bottomRightLabel,
  );

  plotSvg.append(
    plotGrid,
    plotZeroAxis,
    plotPrimeMeridian,
    plotLines,
    plotPoints,
    plotCornerLabels,
    plotHover,
  );
  pane.body.append(plotMeta, plotControls, plotSvg);

  const hidePlotHover = () => {
    plotHover.style.display = "none";
  };

  const renderPointMarkers = (seriesList: PlotItem[], pointsList: PlotPoint[][]) => {
    plotPoints.replaceChildren();
    seriesList.forEach((series, seriesIndex) => {
      const group = document.createElementNS("http://www.w3.org/2000/svg", "g");
      group.setAttribute("class", `plot-series plot-series-${seriesIndex % 6}`);
      for (const point of pointsList[seriesIndex] ?? []) {
        const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
        circle.setAttribute("cx", point.x.toFixed(2));
        circle.setAttribute("cy", point.y.toFixed(2));
        circle.setAttribute("r", "1.6");
        circle.setAttribute("class", "plot-point");
        group.append(circle);
      }
      plotPoints.append(group);
    });
  };

  const renderLinePaths = (seriesList: PlotItem[], paths: string[]) => {
    plotLines.replaceChildren();
    seriesList.forEach((_, seriesIndex) => {
      const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
      path.setAttribute("class", `plot-line plot-series-${seriesIndex % 6}`);
      path.setAttribute("d", paths[seriesIndex] ?? "");
      path.setAttribute("fill", "none");
      plotLines.append(path);
    });
  };

  const formatHoverLabel = (series: PlotItem, index: number) => {
    if ("rawValues" in series) {
      const value = series.rawValues[index] ?? 0;
      return value.toPrecision(6);
    }

    const point = series.rawPoints[index] ?? { latitude_deg: 0, longitude_deg: 0 };
    return `${point.latitude_deg.toFixed(4)}, ${point.longitude_deg.toFixed(4)}`;
  };

  const formatHoverXAxisLabel = (series: PlotItem, index: number) => {
    if ("rawValues" in series) {
      return `${index}`;
    }

    const point = series.rawPoints[index] ?? { latitude_deg: 0, longitude_deg: 0 };
    return point.longitude_deg.toFixed(4);
  };

  const showPlotHover = (series: PlotItem, points: PlotPoint[], index: number) => {
    const point = points[index];
    if (point == null) {
      hidePlotHover();
      return;
    }

    plotHover.style.display = "";
    plotHoverGuide.setAttribute(
      "d",
      `M ${point.x.toFixed(2)} 174 L ${point.x.toFixed(2)} ${point.y.toFixed(2)}`,
    );
    plotHoverPoint.setAttribute("cx", point.x.toFixed(2));
    plotHoverPoint.setAttribute("cy", point.y.toFixed(2));
    plotHoverLabel.textContent = formatHoverLabel(series, index);
    plotHoverXAxisLabel.textContent = formatHoverXAxisLabel(series, index);

    const padding = 6;
    const gap = 8;
    const preferredX = point.x + gap;
    const preferredY = point.y - gap;

    plotHoverLabel.setAttribute("x", preferredX.toFixed(2));
    plotHoverLabel.setAttribute("y", preferredY.toFixed(2));

    const textBounds = plotHoverLabel.getBBox();
    let textX = preferredX;
    let textY = preferredY;

    if (textX + textBounds.width > 314) {
      textX = Math.max(6, point.x - gap - textBounds.width);
    }
    if (textY - textBounds.height < 6) {
      textY = Math.min(174, point.y + gap + textBounds.height * 0.8);
    }

    plotHoverLabel.setAttribute("x", textX.toFixed(2));
    plotHoverLabel.setAttribute("y", textY.toFixed(2));

    const adjustedBounds = plotHoverLabel.getBBox();
    const bubbleX = Math.max(2, adjustedBounds.x - padding);
    const bubbleY = Math.max(2, adjustedBounds.y - padding / 2);
    const bubbleWidth = Math.min(316 - bubbleX, adjustedBounds.width + padding * 2);
    const bubbleHeight = Math.min(176 - bubbleY, adjustedBounds.height + padding);

    plotHoverBubble.setAttribute("x", bubbleX.toFixed(2));
    plotHoverBubble.setAttribute("y", bubbleY.toFixed(2));
    plotHoverBubble.setAttribute("width", bubbleWidth.toFixed(2));
    plotHoverBubble.setAttribute("height", bubbleHeight.toFixed(2));

    const xAxisY = 170;
    plotHoverXAxisLabel.setAttribute("x", point.x.toFixed(2));
    plotHoverXAxisLabel.setAttribute("y", xAxisY.toFixed(2));
    plotHoverXAxisLabel.setAttribute("text-anchor", "middle");
    plotHoverXAxisLabel.setAttribute("dominant-baseline", "auto");

    const xAxisBounds = plotHoverXAxisLabel.getBBox();
    let xAxisLabelX = point.x;
    if (xAxisBounds.x < 4) {
      xAxisLabelX += 4 - xAxisBounds.x;
    }
    if (xAxisBounds.x + xAxisBounds.width > 316) {
      xAxisLabelX -= xAxisBounds.x + xAxisBounds.width - 316;
    }

    plotHoverXAxisLabel.setAttribute("x", xAxisLabelX.toFixed(2));
    const adjustedXAxisBounds = plotHoverXAxisLabel.getBBox();
    plotHoverXAxisBubble.setAttribute(
      "x",
      Math.max(2, adjustedXAxisBounds.x - padding).toFixed(2),
    );
    plotHoverXAxisBubble.setAttribute(
      "y",
      Math.max(158, adjustedXAxisBounds.y - padding / 2).toFixed(2),
    );
    plotHoverXAxisBubble.setAttribute(
      "width",
      Math.min(316, adjustedXAxisBounds.width + padding * 2).toFixed(2),
    );
    plotHoverXAxisBubble.setAttribute(
      "height",
      Math.min(18, adjustedXAxisBounds.height + padding).toFixed(2),
    );
  };

  const renderPlot = (groupList: PlotGroup[]) => {
    const currentGroup = groupList[groupList.length - 1] ?? null;
    pane.count.textContent = `${currentGroup?.items.length ?? 0}`;
    latestPlotGroup = currentGroup;
    latestHoverSeries = null;
    latestPlotPoints = [];
    hidePlotHover();

    if (currentGroup == null || currentGroup.items.length === 0) {
      plotMeta.textContent = "No plottable list values in stack";
      plotZeroAxis.setAttribute("d", "");
      plotPrimeMeridian.setAttribute("d", "");
      plotLines.replaceChildren();
      renderPointMarkers([], []);
      topLeftLabel.textContent = "";
      topRightLabel.textContent = "";
      bottomLeftLabel.textContent = "";
      bottomRightLabel.textContent = "";
      return;
    }

    const suffix = currentGroup.items.some((series) => series.truncated)
      ? " | using visible values"
      : "";
    const scalarSeries = currentGroup.items as PlotSeries[];
    const scalarBounds = buildScalarGroupBounds(scalarSeries);
    const totalValues = scalarSeries.reduce(
      (sum, series) => sum + series.rawValues.length,
      0,
    );
    plotMeta.textContent = `${currentGroup.label} | ${scalarSeries.length} series | ${totalValues} values${suffix}`;
    plotZeroAxis.setAttribute(
      "d",
      buildZeroAxisPath(
        scalarSeries.flatMap((series) => series.rawValues),
        320,
        180,
        scalarBounds,
      ),
    );
    plotPrimeMeridian.setAttribute("d", "");
    const pointsList = scalarSeries.map((series) =>
      buildScalarPlotPoints(series.values, 320, 180, scalarBounds),
    );
    latestHoverSeries = scalarSeries[scalarSeries.length - 1] ?? null;
    latestPlotPoints = latestHoverSeries
      ? buildScalarPlotPoints(latestHoverSeries.rawValues, 320, 180, scalarBounds)
      : [];
    renderLinePaths(
      scalarSeries,
      lineToggle.checked
        ? scalarSeries.map((series) => buildPlotPath(series.values, 320, 180, scalarBounds))
        : scalarSeries.map(() => ""),
    );
    renderPointMarkers(scalarSeries, pointsToggle.checked ? pointsList : scalarSeries.map(() => []));
    topLeftLabel.textContent = formatScalarAxisLabel(scalarBounds.max);
    topRightLabel.textContent = "";
    bottomLeftLabel.textContent = formatScalarAxisLabel(scalarBounds.min);
    bottomRightLabel.textContent = "";
  };

  const rerenderPlot = () => {
    renderPlot(latestStack.map((entry) => parsePlotGroup(entry)).filter(isPlotGroup));
  };

  lineToggle.addEventListener("change", rerenderPlot);
  pointsToggle.addEventListener("change", rerenderPlot);

  plotSvg.addEventListener("mousemove", (event) => {
    if (latestHoverSeries == null || latestPlotPoints.length === 0) {
      hidePlotHover();
      return;
    }

    const bounds = plotSvg.getBoundingClientRect();
    if (bounds.width <= 0 || bounds.height <= 0) {
      hidePlotHover();
      return;
    }

    const x = ((event.clientX - bounds.left) / bounds.width) * 320;
    let bestIndex = 0;

    if (latestHoverSeries.kind === "scalar") {
      bestIndex =
        latestHoverSeries.rawValues.length <= 1
          ? 0
          : Math.round(
              (Math.max(0, Math.min(320, x)) / 320) *
                (latestHoverSeries.rawValues.length - 1),
            );
    } else {
      let bestDistance = Number.POSITIVE_INFINITY;
      for (let index = 0; index < latestPlotPoints.length; index += 1) {
        const distance = Math.abs((latestPlotPoints[index]?.x ?? 0) - x);
        if (distance < bestDistance) {
          bestDistance = distance;
          bestIndex = index;
        }
      }
    }

    showPlotHover(latestHoverSeries, latestPlotPoints, bestIndex);
  });

  plotSvg.addEventListener("mouseleave", hidePlotHover);

  return {
    pane,
    render(stack) {
      latestStack = stack;
      rerenderPlot();
    },
    setDisplaySettings(settings) {
      displaySettings = settings;
      lineToggle.checked = displaySettings.plotDefaultLine;
      pointsToggle.checked = displaySettings.plotDefaultPoints;
      rerenderPlot();
    },
  };
}
