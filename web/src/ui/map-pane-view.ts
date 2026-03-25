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
import { isPlotGroup, parsePlotGroup, toMapCoordinates } from "./plot-support";
import type { PlotGroup, PositionPlotSeries } from "./plot-support";

export interface MapPaneView {
  pane: PaneElements;
  render(stack: BindingStackEntry[]): void;
  setDisplaySettings(settings: DisplaySettings): void;
}

const mapPalette = [
  { stroke: "rgba(111, 157, 115, 0.9)", fill: "#353535" },
  { stroke: "rgba(68, 110, 170, 0.9)", fill: "#446eaa" },
  { stroke: "rgba(181, 118, 20, 0.9)", fill: "#b57614" },
  { stroke: "rgba(138, 89, 166, 0.9)", fill: "#8a59a6" },
  { stroke: "rgba(178, 85, 85, 0.9)", fill: "#b25555" },
  { stroke: "rgba(78, 137, 128, 0.9)", fill: "#4e8980" },
];

export function createMapPaneView(
  onToggle: (expanded: boolean) => void,
  initialDisplaySettings: DisplaySettings,
): MapPaneView {
  let displaySettings = initialDisplaySettings;
  let latestStack: BindingStackEntry[] = [];
  let latestMapGroup: PlotGroup | null = null;

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
    style: (feature) => {
      const index = Number(feature.get("seriesIndex") ?? 0) % mapPalette.length;
      const palette = mapPalette[index];
      return new Style({
        image: new CircleStyle({
          radius: 4,
          fill: new Fill({ color: palette.fill }),
          stroke: new Stroke({ color: "rgba(250, 247, 239, 0.95)", width: 1.5 }),
        }),
      });
    },
  });

  const mapLineLayer = new VectorLayer({
    source: mapLineSource,
    style: (feature) => {
      const index = Number(feature.get("seriesIndex") ?? 0) % mapPalette.length;
      const palette = mapPalette[index];
      return new Style({
        stroke: new Stroke({
          color: palette.stroke,
          width: 2.5,
        }),
      });
    },
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

  const applyMapView = (group: PlotGroup | null) => {
    const view = map.getView();
    if (group == null || group.kind !== "position") {
      view.setCenter(fromLonLat([0, 0]));
      view.setZoom(1.5);
      return;
    }

    const mapCoordinates = toMapCoordinates(
      (group.items as PositionPlotSeries[]).flatMap((series) => series.points),
    );
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

  const pane = createPane("Map", (expanded) => {
    if (expanded) {
      requestAnimationFrame(() => {
        map.updateSize();
        applyMapView(latestMapGroup);
      });
    }
    onToggle(expanded);
  });

  const renderMap = (groupList: PlotGroup[]) => {
    const currentGroup = [...groupList].reverse().find((group) => group.kind === "position");
    pane.count.textContent = `${currentGroup?.items.length ?? 0}`;
    latestMapGroup = currentGroup ?? null;

    if (currentGroup == null || currentGroup.kind !== "position") {
      mapMeta.textContent = "World view";
      mapPointSource.clear();
      mapLineSource.clear();
      requestAnimationFrame(() => {
        map.updateSize();
        applyMapView(null);
      });
      return;
    }

    const seriesList = currentGroup.items as PositionPlotSeries[];
    const totalPositions = seriesList.reduce(
      (sum, series) => sum + series.points.length,
      0,
    );
    mapMeta.textContent = `${currentGroup.label} | ${seriesList.length} paths | ${totalPositions} positions`;
    mapPointSource.clear();
    mapLineSource.clear();

    seriesList.forEach((series, seriesIndex) => {
      const mapCoordinates = toMapCoordinates(series.points);
      mapPointSource.addFeatures(
        mapCoordinates.map((coordinate) => {
          const feature = new Feature(new Point(coordinate));
          feature.set("seriesIndex", seriesIndex);
          return feature;
        }),
      );
      if (connectMapLinesToggle.checked && mapCoordinates.length >= 2) {
        const feature = new Feature(new LineString(mapCoordinates));
        feature.set("seriesIndex", seriesIndex);
        mapLineSource.addFeature(feature);
      }
    });

    if (pane.body.hidden) {
      return;
    }

    requestAnimationFrame(() => {
      map.updateSize();
      applyMapView(currentGroup);
    });
  };

  const rerenderMap = () => {
    renderMap(latestStack.map((entry) => parsePlotGroup(entry)).filter(isPlotGroup));
  };

  const applyMapHeight = (height: number) => {
    const clampedHeight = Math.max(180, Math.min(height, 640));
    mapElement.style.height = `${Math.round(clampedHeight)}px`;
    requestAnimationFrame(() => {
      map.updateSize();
      if (!pane.body.hidden) {
        applyMapView(latestMapGroup);
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
