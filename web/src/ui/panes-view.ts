import type {
  BindingConstantEntry,
  BindingDefinitionEntry,
  BindingFunctionEntry,
  BindingPositionEntry,
  BindingSnapshot,
  BindingStackEntry,
} from "../bridge/console-wasm";
import Feature from "ol/Feature";
import LineString from "ol/geom/LineString";
import Point from "ol/geom/Point";
import OlMap from "ol/Map";
import View from "ol/View";
import { OSM, Vector as VectorSource } from "ol/source";
import { Tile as TileLayer, Vector as VectorLayer } from "ol/layer";
import { defaults as defaultInteractions } from "ol/interaction/defaults";
import MouseWheelZoom from "ol/interaction/MouseWheelZoom";
import { Fill, Stroke, Style, Circle as CircleStyle } from "ol/style";
import { fromLonLat } from "ol/proj";
import type { DisplaySettings } from "./display-settings";
import { formatNumericText } from "./display-settings";

export interface PanesView {
  element: HTMLElement;
  render(snapshot: BindingSnapshot): void;
  setDisplaySettings(settings: DisplaySettings): void;
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
  rawValues: number[];
  truncated: boolean;
}

interface PositionPlotSeries {
  key: string;
  label: string;
  points: BindingPositionEntry[];
  rawPoints: BindingPositionEntry[];
  truncated: boolean;
}

type PlotItem = PlotSeries | PositionPlotSeries;
type PlotPoint = { x: number; y: number };
const scalarPlotVerticalPadding = 5;

function isPlotSeries(value: PlotItem | null): value is PlotItem {
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

function stackDisplay(entry: BindingStackEntry, settings: DisplaySettings): string {
  const formatScalarPreview = (value: number) =>
    formatNumericText(`${value}`, settings.stackDecimals);
  const formatPositionPreview = (value: BindingPositionEntry) =>
    `pos(${formatNumericText(`${value.latitude_deg}`, settings.stackDecimals)}, ${formatNumericText(`${value.longitude_deg}`, settings.stackDecimals)})`;

  const positionListValues = entry.position_list_values ?? [];
  if (positionListValues.length > 0) {
    const preview = positionListValues.slice(0, 2).map(formatPositionPreview).join(", ");
    const suffix =
      positionListValues.length > 2 ? `, ... <${positionListValues.length - 2} more>` : "";
    return `${entry.level}:{${preview}${suffix}}`;
  }

  const listValues = entry.list_values ?? [];
  if (listValues.length > 0 || entry.display.trim() === "{}") {
    const preview = listValues.slice(0, 2).map(formatScalarPreview).join(", ");
    const suffix = listValues.length > 2 ? `, ... <${listValues.length - 2} more>` : "";
    return `${entry.level}:{${preview}${suffix}}`;
  }

  return `${entry.level}:${formatNumericText(entry.display, settings.stackDecimals)}`;
}

function definitionDisplay(entry: BindingDefinitionEntry): string {
  return `${entry.name}:${entry.expression}`;
}

function constantDisplay(entry: BindingConstantEntry): string {
  return `${entry.name}:${entry.value}`;
}

function parsePlotSeries(entry: BindingStackEntry): PlotItem | null {
  const positionListValues = entry.position_list_values ?? [];
  if (positionListValues.length > 0 || entry.display.trim().startsWith("{pos(")) {
    const maxPositionPoints = 10000;
    const points =
      positionListValues.length <= maxPositionPoints
        ? positionListValues
        : Array.from({ length: maxPositionPoints }, (_, index) => {
            const sourceIndex = Math.round(
              (index / (maxPositionPoints - 1)) * (positionListValues.length - 1),
            );
            return (
              positionListValues[sourceIndex] ?? {
                latitude_deg: 0,
                longitude_deg: 0,
              }
            );
          });
    return {
      key: `stack-${entry.level}`,
      label: `Stack ${entry.level}`,
      points,
      rawPoints: positionListValues,
      truncated: positionListValues.length > 500,
    };
  }

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
    rawValues: listValues,
    truncated,
  };
}

function buildScalarBounds(values: number[]): { min: number; max: number; range: number } {
  const min = Math.min(...values);
  const max = Math.max(...values);
  return {
    min,
    max,
    range: max - min || 1,
  };
}

function formatScalarAxisLabel(value: number): string {
  return value.toPrecision(6);
}

function buildPlotPath(
  values: number[],
  width: number,
  height: number,
  bounds = buildScalarBounds(values),
): string {
  if (values.length === 0) {
    return "";
  }

  if (values.length === 1) {
    const y = height / 2;
    return `M 0 ${y} L ${width} ${y}`;
  }

  const plotHeight = height - scalarPlotVerticalPadding * 2;

  return values
    .map((value, index) => {
      const x = (index / (values.length - 1)) * width;
      const y =
        height -
        scalarPlotVerticalPadding -
        ((value - bounds.min) / bounds.range) * plotHeight;
      return `${index === 0 ? "M" : "L"} ${x.toFixed(2)} ${y.toFixed(2)}`;
    })
    .join(" ");
}

function buildZeroAxisPath(
  values: number[],
  width: number,
  height: number,
  bounds = buildScalarBounds(values),
): string {
  if (values.length === 0) {
    return `M 0 ${height} L ${width} ${height}`;
  }

  if (bounds.min > 0 || bounds.max < 0) {
    return "";
  }

  const plotHeight = height - scalarPlotVerticalPadding * 2;
  const y =
    height -
    scalarPlotVerticalPadding -
    ((0 - bounds.min) / bounds.range) * plotHeight;
  return `M 0 ${y.toFixed(2)} L ${width} ${y.toFixed(2)}`;
}

function buildPositionBounds(points: BindingPositionEntry[]): {
  minX: number;
  maxX: number;
  minY: number;
  maxY: number;
} {
  const longitudes = points.map((point) => point.longitude_deg);
  const latitudes = points.map((point) => point.latitude_deg);
  const minX = Math.min(...longitudes);
  const maxX = Math.max(...longitudes);
  const minY = Math.min(...latitudes);
  const maxY = Math.max(...latitudes);
  const spanX = maxX - minX;
  const spanY = maxY - minY;
  const paddedSpanX = spanX === 0 ? 0.02 : spanX * 1.08;
  const paddedSpanY = spanY === 0 ? 0.02 : spanY * 1.14;
  const unifiedSpan = Math.max(paddedSpanX, paddedSpanY);
  const centerX = (minX + maxX) / 2;
  const centerY = (minY + maxY) / 2;
  const halfSpan = unifiedSpan / 2;

  return {
    minX: centerX - halfSpan,
    maxX: centerX + halfSpan,
    minY: centerY - halfSpan,
    maxY: centerY + halfSpan,
  };
}

function formatLatitude(value: number): string {
  const suffix = value < 0 ? "S" : "N";
  return `${Math.abs(value).toFixed(1).padStart(4, "0")}${suffix}`;
}

function formatLongitude(value: number): string {
  const suffix = value < 0 ? "W" : "E";
  return `${Math.abs(value).toFixed(1).padStart(5, "0")}${suffix}`;
}

function formatCornerLabel(latitude: number, longitude: number): string {
  return `${formatLatitude(latitude)}-${formatLongitude(longitude)}`;
}

function buildPositionPlotPath(
  points: BindingPositionEntry[],
  width: number,
  height: number,
  bounds = buildPositionBounds(points),
): string {
  if (points.length === 0) {
    return "";
  }

  const rangeX = bounds.maxX - bounds.minX || 1;
  const rangeY = bounds.maxY - bounds.minY || 1;
  const padding = 8;
  const plotWidth = width - padding * 2;
  const plotHeight = height - padding * 2;

  return points
    .map((point, index) => {
      const x = padding + ((point.longitude_deg - bounds.minX) / rangeX) * plotWidth;
      const y =
        height - padding - ((point.latitude_deg - bounds.minY) / rangeY) * plotHeight;
      return `${index === 0 ? "M" : "L"} ${x.toFixed(2)} ${y.toFixed(2)}`;
    })
    .join(" ");
}

function buildPositionReferenceAxes(
  points: BindingPositionEntry[],
  width: number,
  height: number,
  bounds = buildPositionBounds(points),
): { equator: string; primeMeridian: string } {
  if (points.length === 0) {
    return { equator: "", primeMeridian: "" };
  }

  const rangeX = bounds.maxX - bounds.minX || 1;
  const rangeY = bounds.maxY - bounds.minY || 1;
  const padding = 8;
  const plotWidth = width - padding * 2;
  const plotHeight = height - padding * 2;

  let equator = "";
  if (bounds.minY <= 0 && bounds.maxY >= 0) {
    const y = height - padding - ((0 - bounds.minY) / rangeY) * plotHeight;
    equator = `M ${padding} ${y.toFixed(2)} L ${(width - padding).toFixed(2)} ${y.toFixed(2)}`;
  }

  let primeMeridian = "";
  if (bounds.minX <= 0 && bounds.maxX >= 0) {
    const x = padding + ((0 - bounds.minX) / rangeX) * plotWidth;
    primeMeridian = `M ${x.toFixed(2)} ${padding} L ${x.toFixed(2)} ${(height - padding).toFixed(2)}`;
  }

  return { equator, primeMeridian };
}

function toMapCoordinates(points: BindingPositionEntry[]): [number, number][] {
  const maxMercatorLatitude = 85.05112878;
  return points.map((point) =>
    fromLonLat([
      point.longitude_deg,
      Math.max(-maxMercatorLatitude, Math.min(maxMercatorLatitude, point.latitude_deg)),
    ]),
  );
}

interface PaneElements {
  section: HTMLElement;
  header: HTMLElement;
  title: HTMLButtonElement;
  body: HTMLElement;
  count: HTMLElement;
  actions: HTMLElement;
}

function createPane(titleText: string, onToggle?: (expanded: boolean) => void): PaneElements {
  const section = document.createElement("section");
  section.className = "pane";

  const header = document.createElement("div");
  header.className = "pane-header";

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

  const actions = document.createElement("div");
  actions.className = "pane-header-actions";

  const body = document.createElement("div");
  body.className = "pane-body";
  body.hidden = true;

  title.addEventListener("click", () => {
    const expanded = title.getAttribute("aria-expanded") !== "false";
    title.setAttribute("aria-expanded", expanded ? "false" : "true");
    body.hidden = expanded;
    marker.textContent = expanded ? "+" : "−";
    onToggle?.(!expanded);
  });

  title.append(titleLabel, marker);
  header.append(title, actions, count);
  section.append(header, body);
  return { section, header, title, body, count, actions };
}

export function createPanesView(
  onSampleSelected?: (expression: string) => void,
  onClearStack?: () => void,
  initialDisplaySettings: DisplaySettings = { transcriptDecimals: 8, stackDecimals: 6 },
): PanesView {
  const section = document.createElement("section");
  section.className = "panes-view";

  const infoPanel = document.createElement("section");
  infoPanel.className = "info-panel";

  const plotPanel = document.createElement("section");
  plotPanel.className = "plot-panel";

  const mapPanel = document.createElement("section");
  mapPanel.className = "map-panel";
  let latestMapSeries: PositionPlotSeries | null = null;
  let displaySettings = initialDisplaySettings;
  let latestSnapshot: BindingSnapshot | null = null;

  const status = document.createElement("div");
  status.className = "pane-status";

  const stackPane = createPane("Stack");
  const definitionsPane = createPane("Definitions");
  const constantsPane = createPane("Constants");
  const functionsPane = createPane("Functions");
  const samplesPane = createPane("Samples");
  const plotPane = createPane("Plot");
  const mapPane = createPane("Map", (expanded) => {
    if (expanded) {
      requestAnimationFrame(() => {
        map.updateSize();
        applyMapView(latestMapSeries);
      });
    }
  });

  functionsPane.body.classList.add("functions-pane-body");
  const stackTitleLabel = stackPane.title.querySelector<HTMLElement>(".pane-title-label");

  const functionTableContainer = document.createElement("div");
  functionTableContainer.className = "function-table-container";

  const clearStackButton = document.createElement("button");
  clearStackButton.type = "button";
  clearStackButton.className = "pane-icon-button";
  clearStackButton.textContent = "×";
  clearStackButton.setAttribute("aria-label", "Clear stack");
  clearStackButton.title = "Clear stack";
  clearStackButton.addEventListener("click", () => {
    onClearStack?.();
  });

  const stackList = document.createElement("div");

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

  stackPane.actions.append(clearStackButton);
  stackPane.body.append(stackList);
  functionsPane.body.append(functionTableContainer);
  samplesPane.body.append(samplesList);

  const plotMeta = document.createElement("div");
  plotMeta.className = "plot-meta";

  const plotControls = document.createElement("div");
  plotControls.className = "plot-controls";

  const lineToggleLabel = document.createElement("label");
  lineToggleLabel.className = "plot-toggle";

  const lineToggle = document.createElement("input");
  lineToggle.type = "checkbox";
  lineToggle.checked = true;

  const lineToggleText = document.createElement("span");
  lineToggleText.textContent = "Line";

  lineToggleLabel.append(lineToggle, lineToggleText);

  const pointsToggleLabel = document.createElement("label");
  pointsToggleLabel.className = "plot-toggle";

  const pointsToggle = document.createElement("input");
  pointsToggle.type = "checkbox";
  pointsToggle.checked = false;

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

  const plotLine = document.createElementNS("http://www.w3.org/2000/svg", "path");
  plotLine.setAttribute("class", "plot-line");

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

  const plotHoverXAxisBubble = document.createElementNS("http://www.w3.org/2000/svg", "rect");
  plotHoverXAxisBubble.setAttribute("class", "plot-hover-bubble");
  plotHoverXAxisBubble.setAttribute("rx", "4");
  plotHoverXAxisBubble.setAttribute("ry", "4");

  const plotHoverXAxisLabel = document.createElementNS("http://www.w3.org/2000/svg", "text");
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
    plotLine,
    plotPoints,
    plotCornerLabels,
    plotHover,
  );
  plotPane.body.append(plotMeta, plotControls, plotSvg);

  const mapMeta = document.createElement("div");
  mapMeta.className = "map-meta";

  const mapControls = document.createElement("label");
  mapControls.className = "plot-controls";

  const connectMapLinesToggle = document.createElement("input");
  connectMapLinesToggle.type = "checkbox";
  connectMapLinesToggle.checked = false;

  const connectMapLinesLabel = document.createElement("span");
  connectMapLinesLabel.textContent = "Connect lines";

  mapControls.append(connectMapLinesToggle, connectMapLinesLabel);

  const mapElement = document.createElement("div");
  mapElement.className = "map-canvas";

  const mapResizeHandle = document.createElement("div");
  mapResizeHandle.className = "map-resize-handle";
  mapResizeHandle.setAttribute("role", "separator");
  mapResizeHandle.setAttribute("aria-orientation", "horizontal");
  mapResizeHandle.setAttribute("aria-label", "Resize map height");

  const applyMapHeight = (height: number) => {
    const clampedHeight = Math.max(180, Math.min(height, 640));
    mapElement.style.height = `${Math.round(clampedHeight)}px`;
    requestAnimationFrame(() => {
      map.updateSize();
      if (!mapPane.body.hidden) {
        applyMapView(latestMapSeries);
      }
    });
  };

  mapResizeHandle.addEventListener("pointerdown", (event) => {
    event.preventDefault();
    mapResizeHandle.setPointerCapture(event.pointerId);
    document.body.classList.add("is-resizing-map");

    const startY = event.clientY;
    const startHeight = mapElement.getBoundingClientRect().height;

    const onPointerMove = (moveEvent: PointerEvent) => {
      applyMapHeight(startHeight + (moveEvent.clientY - startY));
    };

    const stopDragging = () => {
      document.body.classList.remove("is-resizing-map");
      mapResizeHandle.removeEventListener("pointermove", onPointerMove);
      mapResizeHandle.removeEventListener("pointerup", stopDragging);
      mapResizeHandle.removeEventListener("pointercancel", stopDragging);
    };

    mapResizeHandle.addEventListener("pointermove", onPointerMove);
    mapResizeHandle.addEventListener("pointerup", stopDragging);
    mapResizeHandle.addEventListener("pointercancel", stopDragging);
  });

  mapPane.body.append(mapMeta, mapControls, mapElement, mapResizeHandle);

  const mapPointSource = new VectorSource();
  const mapLineSource = new VectorSource();

  const mapPointLayer = new VectorLayer({
    source: mapPointSource,
    style: new Style({
      image: new CircleStyle({
        radius: 4,
        fill: new Fill({ color: "#2e5d31" }),
        stroke: new Stroke({ color: "rgba(250, 247, 239, 0.95)", width: 1.5 }),
      }),
    }),
  });

  const mapLineLayer = new VectorLayer({
    source: mapLineSource,
    style: new Style({
      stroke: new Stroke({
        color: "rgba(46, 93, 49, 0.8)",
        width: 2.5,
      }),
    }),
  });

  const map = new OlMap({
    target: mapElement,
    layers: [
      new TileLayer({
        source: new OSM(),
      }),
      mapLineLayer,
      mapPointLayer,
    ],
    interactions: defaultInteractions({
      mouseWheelZoom: false,
    }).extend([
      new MouseWheelZoom({
        maxDelta: 5,
        duration: 120,
        timeout: 50,
        useAnchor: true,
      }),
    ]),
    view: new View({
      center: fromLonLat([0, 0]),
      zoom: 1.5,
    }),
  });

  let latestPlotSeries: PlotItem | null = null;
  let latestPlotPoints: PlotPoint[] = [];

  const hidePlotHover = () => {
    plotHover.style.display = "none";
  };

  const renderPointMarkers = (points: PlotPoint[]) => {
    plotPoints.replaceChildren();
    for (const point of points) {
      const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
      circle.setAttribute("cx", point.x.toFixed(2));
      circle.setAttribute("cy", point.y.toFixed(2));
      circle.setAttribute("r", "1.6");
      circle.setAttribute("class", "plot-point");
      plotPoints.append(circle);
    }
  };

  const buildScalarPlotPoints = (
    values: number[],
    width: number,
    height: number,
    bounds = buildScalarBounds(values),
  ): PlotPoint[] => {
    if (values.length === 0) {
      return [];
    }
    if (values.length === 1) {
      return [{ x: width / 2, y: height / 2 }];
    }

    const plotHeight = height - scalarPlotVerticalPadding * 2;

    return values.map((value, index) => ({
      x: (index / (values.length - 1)) * width,
      y:
        height -
        scalarPlotVerticalPadding -
        ((value - bounds.min) / bounds.range) * plotHeight,
    }));
  };

  const buildPositionPlotPoints = (
    points: BindingPositionEntry[],
    width: number,
    height: number,
    bounds = buildPositionBounds(points),
  ): PlotPoint[] => {
    if (points.length === 0) {
      return [];
    }

    const rangeX = bounds.maxX - bounds.minX || 1;
    const rangeY = bounds.maxY - bounds.minY || 1;
    const padding = 8;
    const plotWidth = width - padding * 2;
    const plotHeight = height - padding * 2;

    return points.map((point) => ({
      x: padding + ((point.longitude_deg - bounds.minX) / rangeX) * plotWidth,
      y:
        height - padding - ((point.latitude_deg - bounds.minY) / rangeY) * plotHeight,
    }));
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
    plotHoverXAxisBubble.setAttribute("x", Math.max(2, adjustedXAxisBounds.x - padding).toFixed(2));
    plotHoverXAxisBubble.setAttribute("y", Math.max(158, adjustedXAxisBounds.y - padding / 2).toFixed(2));
    plotHoverXAxisBubble.setAttribute(
      "width",
      Math.min(316, adjustedXAxisBounds.width + padding * 2).toFixed(2),
    );
    plotHoverXAxisBubble.setAttribute(
      "height",
      Math.min(18, adjustedXAxisBounds.height + padding).toFixed(2),
    );
  };

  const renderPlot = (seriesList: PlotItem[]) => {
    plotPane.count.textContent = `${seriesList.length}`;
    latestPlotSeries = null;
    latestPlotPoints = [];
    hidePlotHover();

    if (seriesList.length === 0) {
      plotMeta.textContent = "No plottable list values in stack";
      plotZeroAxis.setAttribute("d", "");
      plotPrimeMeridian.setAttribute("d", "");
      plotLine.setAttribute("d", "");
      renderPointMarkers([]);
      topLeftLabel.textContent = "";
      topRightLabel.textContent = "";
      bottomLeftLabel.textContent = "";
      bottomRightLabel.textContent = "";
      return;
    }

    const currentSeries = seriesList[seriesList.length - 1];
    const suffix = currentSeries.truncated ? " | using visible values" : "";
    if ("rawValues" in currentSeries) {
      const scalarBounds = buildScalarBounds(currentSeries.rawValues);
      plotMeta.textContent = `${currentSeries.label} | ${currentSeries.rawValues.length} values${suffix}`;
      plotZeroAxis.setAttribute(
        "d",
        buildZeroAxisPath(currentSeries.rawValues, 320, 180, scalarBounds),
      );
      plotPrimeMeridian.setAttribute("d", "");
      const points = buildScalarPlotPoints(currentSeries.values, 320, 180, scalarBounds);
      latestPlotSeries = currentSeries;
      latestPlotPoints = buildScalarPlotPoints(
        currentSeries.rawValues,
        320,
        180,
        scalarBounds,
      );
      plotLine.setAttribute(
        "d",
        lineToggle.checked
          ? buildPlotPath(currentSeries.values, 320, 180, scalarBounds)
          : "",
      );
      renderPointMarkers(pointsToggle.checked ? points : []);
      topLeftLabel.textContent = formatScalarAxisLabel(scalarBounds.max);
      topRightLabel.textContent = "";
      bottomLeftLabel.textContent = formatScalarAxisLabel(scalarBounds.min);
      bottomRightLabel.textContent = "";
      return;
    }

    plotMeta.textContent = `${currentSeries.label} | ${currentSeries.rawPoints.length} positions | lon/x, lat/y${suffix}`;
    const bounds = buildPositionBounds(currentSeries.rawPoints);
    const referenceAxes = buildPositionReferenceAxes(currentSeries.rawPoints, 320, 180, bounds);
    plotZeroAxis.setAttribute("d", referenceAxes.equator);
    plotPrimeMeridian.setAttribute("d", referenceAxes.primeMeridian);
    const points = buildPositionPlotPoints(currentSeries.points, 320, 180, bounds);
    latestPlotSeries = currentSeries;
    latestPlotPoints = buildPositionPlotPoints(currentSeries.rawPoints, 320, 180, bounds);
    plotLine.setAttribute(
      "d",
      lineToggle.checked
        ? buildPositionPlotPath(currentSeries.points, 320, 180, bounds)
        : "",
    );
    renderPointMarkers(pointsToggle.checked ? points : []);
    topLeftLabel.textContent = formatCornerLabel(bounds.maxY, bounds.minX);
    topRightLabel.textContent = formatCornerLabel(bounds.maxY, bounds.maxX);
    bottomLeftLabel.textContent = formatCornerLabel(bounds.minY, bounds.minX);
    bottomRightLabel.textContent = formatCornerLabel(bounds.minY, bounds.maxX);
  };

  const rerenderPlotFromSnapshot = () => {
    const stackEntries = (section as HTMLElement).dataset.plotSnapshot;
    if (stackEntries == null) {
      return;
    }
    const series = (JSON.parse(stackEntries) as BindingStackEntry[])
      .map((entry) => parsePlotSeries(entry))
      .filter(isPlotSeries);
    renderPlot(series);
  };

  const renderMap = (seriesList: PlotItem[]) => {
    const currentSeries = [...seriesList].reverse().find((series) => "points" in series);
    mapPane.count.textContent = currentSeries != null && "points" in currentSeries ? "1" : "0";
    latestMapSeries =
      currentSeries != null && "points" in currentSeries ? currentSeries : null;

    if (currentSeries == null || !("points" in currentSeries)) {
      mapMeta.textContent = "World view";
      mapPointSource.clear();
      mapLineSource.clear();
      requestAnimationFrame(() => {
        map.updateSize();
        applyMapView(null);
      });
      return;
    }

    const mapCoordinates = toMapCoordinates(currentSeries.points);
    mapMeta.textContent = `${currentSeries.label} | ${currentSeries.points.length} positions`;
    mapPointSource.clear();
    mapLineSource.clear();
    mapPointSource.addFeatures(
      mapCoordinates.map((coordinate) => new Feature(new Point(coordinate))),
    );
    if (connectMapLinesToggle.checked && mapCoordinates.length >= 2) {
      mapLineSource.addFeature(new Feature(new LineString(mapCoordinates)));
    }

    if (mapPane.body.hidden) {
      return;
    }

    requestAnimationFrame(() => {
      map.updateSize();
      applyMapView(currentSeries);
    });
  };

  const applyMapView = (series: PositionPlotSeries | null) => {
    const view = map.getView();
    if (series == null) {
      view.setCenter(fromLonLat([0, 0]));
      view.setZoom(1.5);
      return;
    }

    const mapCoordinates = toMapCoordinates(series.points);
    if (mapCoordinates.length === 0) {
      view.setCenter(fromLonLat([0, 0]));
      view.setZoom(1.5);
      return;
    }
    if (mapCoordinates.length === 1) {
      view.setCenter(mapCoordinates[0]);
      view.setZoom(8);
      return;
    }

    view.fit(mapPointSource.getExtent(), {
      size: map.getSize(),
      padding: [48, 48, 48, 48],
      maxZoom: 12,
    });
  };

  lineToggle.addEventListener("change", rerenderPlotFromSnapshot);
  pointsToggle.addEventListener("change", rerenderPlotFromSnapshot);

  plotSvg.addEventListener("mousemove", (event) => {
    if (latestPlotSeries == null || latestPlotPoints.length === 0) {
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

    if ("rawValues" in latestPlotSeries) {
      bestIndex =
        latestPlotSeries.rawValues.length <= 1
          ? 0
          : Math.round((Math.max(0, Math.min(320, x)) / 320) * (latestPlotSeries.rawValues.length - 1));
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

    showPlotHover(latestPlotSeries, latestPlotPoints, bestIndex);
  });

  plotSvg.addEventListener("mouseleave", () => {
    hidePlotHover();
  });

  connectMapLinesToggle.addEventListener("change", () => {
    const stackEntries = (section as HTMLElement).dataset.plotSnapshot;
    if (stackEntries == null) {
      return;
    }
    const series = (JSON.parse(stackEntries) as BindingStackEntry[])
      .map((entry) => parsePlotSeries(entry))
      .filter(isPlotSeries);
    renderMap(series);
  });

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

  mapPanel.append(mapPane.section);

  section.append(infoPanel, plotPanel, mapPanel);

  return {
    element: section,
    render(snapshot) {
      latestSnapshot = snapshot;
      if (stackTitleLabel !== null) {
        stackTitleLabel.textContent = `Stack d${displaySettings.stackDecimals}`;
      }
      status.textContent = `Mode ${snapshot.display_mode} | stack ${snapshot.stack.length}/${snapshot.max_stack_depth}`;
      stackPane.count.textContent = `${snapshot.stack.length}`;
      clearStackButton.disabled = snapshot.stack.length === 0;
      definitionsPane.count.textContent = `${snapshot.definitions.length}`;
      constantsPane.count.textContent = `${snapshot.constants.length}`;
      functionsPane.count.textContent = `${snapshot.functions.length}`;
      samplesPane.count.textContent = `${sampleExpressions.length}`;
      renderTextList(
        stackList,
        snapshot.stack.map((entry) => stackDisplay(entry, displaySettings)),
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
      section.dataset.plotSnapshot = JSON.stringify(snapshot.stack);
      const series = snapshot.stack.map((entry) => parsePlotSeries(entry)).filter(isPlotSeries);
      renderPlot(series);
      renderMap(series);
    },
    setDisplaySettings(settings) {
      displaySettings = settings;
      if (stackTitleLabel !== null) {
        stackTitleLabel.textContent = `Stack d${displaySettings.stackDecimals}`;
      }
      if (latestSnapshot !== null) {
        renderTextList(
          stackList,
          latestSnapshot.stack.map((entry) => stackDisplay(entry, displaySettings)),
        );
      }
    },
  };
}
