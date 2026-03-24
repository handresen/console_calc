import type { BindingStackEntry } from "../bridge/console-wasm";
import Feature from "ol/Feature";
import LineString from "ol/geom/LineString";
import Point from "ol/geom/Point";
import MouseWheelZoom from "ol/interaction/MouseWheelZoom";
import { defaults as defaultInteractions } from "ol/interaction/defaults";
import { Tile as TileLayer, Vector as VectorLayer } from "ol/layer";
import OlMap from "ol/Map";
import View from "ol/View";
import { fromLonLat } from "ol/proj";
import { OSM, Vector as VectorSource } from "ol/source";
import { Circle as CircleStyle, Fill, Stroke, Style } from "ol/style";
import type { DisplaySettings } from "./display-settings";
import { createPane } from "./pane-controls";
import type { PaneElements } from "./pane-controls";
import { isPlotSeries, parsePlotSeries, toMapCoordinates } from "./plot-support";
import type { PlotItem, PositionPlotSeries } from "./plot-support";

export interface MapPaneView {
  pane: PaneElements;
  render(stack: BindingStackEntry[]): void;
  setDisplaySettings(settings: DisplaySettings): void;
}

export function createMapPaneView(
  onToggle: (expanded: boolean) => void,
  initialDisplaySettings: DisplaySettings,
): MapPaneView {
  let displaySettings = initialDisplaySettings;
  let latestStack: BindingStackEntry[] = [];
  let latestMapSeries: PositionPlotSeries | null = null;

  const pane = createPane("Map", (expanded) => {
    if (expanded) {
      requestAnimationFrame(() => {
        map.updateSize();
        applyMapView(latestMapSeries);
      });
    }
    onToggle(expanded);
  });

  const mapMeta = document.createElement("div");
  mapMeta.className = "map-meta";

  const mapControls = document.createElement("label");
  mapControls.className = "plot-controls";

  const connectMapLinesToggle = document.createElement("input");
  connectMapLinesToggle.type = "checkbox";
  connectMapLinesToggle.checked = displaySettings.mapDefaultConnectLines;

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

  const renderMap = (seriesList: PlotItem[]) => {
    const currentSeries = [...seriesList].reverse().find((series) => "points" in series);
    pane.count.textContent = currentSeries != null && "points" in currentSeries ? "1" : "0";
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

    if (pane.body.hidden) {
      return;
    }

    requestAnimationFrame(() => {
      map.updateSize();
      applyMapView(currentSeries);
    });
  };

  const rerenderMap = () => {
    renderMap(latestStack.map((entry) => parsePlotSeries(entry)).filter(isPlotSeries));
  };

  const applyMapHeight = (height: number) => {
    const clampedHeight = Math.max(180, Math.min(height, 640));
    mapElement.style.height = `${Math.round(clampedHeight)}px`;
    requestAnimationFrame(() => {
      map.updateSize();
      if (!pane.body.hidden) {
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

  connectMapLinesToggle.addEventListener("change", rerenderMap);

  pane.body.append(mapMeta, mapControls, mapElement, mapResizeHandle);

  return {
    pane,
    render(stack) {
      latestStack = stack;
      rerenderMap();
    },
    setDisplaySettings(settings) {
      displaySettings = settings;
      connectMapLinesToggle.checked = displaySettings.mapDefaultConnectLines;
      rerenderMap();
    },
  };
}
