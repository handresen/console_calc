import type { BindingStackEntry } from "../bridge/console-wasm";
import Feature from "ol/Feature";
import LineString from "ol/geom/LineString";
import Point from "ol/geom/Point";
import Polygon from "ol/geom/Polygon";
import MouseWheelZoom from "ol/interaction/MouseWheelZoom";
import { defaults as defaultInteractions } from "ol/interaction/defaults";
import { Tile as TileLayer, Vector as VectorLayer } from "ol/layer";
import OlMap from "ol/Map";
import View from "ol/View";
import { fromLonLat } from "ol/proj";
import { OSM, Vector as VectorSource, XYZ } from "ol/source";
import { Circle as CircleStyle, Fill, Stroke, Style } from "ol/style";
import type { DisplaySettings } from "./display-settings";
import { createPane } from "./pane-controls";
import type { PaneElements } from "./pane-controls";
import {
  isClosedPositionSeries,
  isPlotGroup,
  parseMapGroup,
  toMapCoordinates,
} from "./plot-support";
import type { PlotGroup, PositionPlotSeries } from "./plot-support";

export interface MapPaneView {
  pane: PaneElements;
  render(stack: BindingStackEntry[]): void;
  setDisplaySettings(settings: DisplaySettings): void;
  refreshLayout(): void;
}

interface ApplyMapViewOptions {
  animate?: boolean;
}

const mapPalette = [
  {
    stroke: "rgba(111, 157, 115, 0.9)",
    pointFill: "#353535",
    areaFill: "rgba(111, 157, 115, 0.24)",
  },
  {
    stroke: "rgba(68, 110, 170, 0.9)",
    pointFill: "#446eaa",
    areaFill: "rgba(68, 110, 170, 0.24)",
  },
  {
    stroke: "rgba(181, 118, 20, 0.9)",
    pointFill: "#b57614",
    areaFill: "rgba(181, 118, 20, 0.24)",
  },
  {
    stroke: "rgba(138, 89, 166, 0.9)",
    pointFill: "#8a59a6",
    areaFill: "rgba(138, 89, 166, 0.24)",
  },
  {
    stroke: "rgba(178, 85, 85, 0.9)",
    pointFill: "#b25555",
    areaFill: "rgba(178, 85, 85, 0.24)",
  },
  {
    stroke: "rgba(78, 137, 128, 0.9)",
    pointFill: "#4e8980",
    areaFill: "rgba(78, 137, 128, 0.24)",
  },
];

const mapLayerStorageKey = "console-calc-map-layer";

const mapLayerOptions = [
  {
    key: "osm-standard",
    label: "OSM Standard",
    createSource: () => new OSM(),
  },
  {
    key: "open-topo",
    label: "OpenTopoMap",
    createSource: () =>
      new XYZ({
        urls: [
          "https://a.tile.opentopomap.org/{z}/{x}/{y}.png",
          "https://b.tile.opentopomap.org/{z}/{x}/{y}.png",
          "https://c.tile.opentopomap.org/{z}/{x}/{y}.png",
        ],
        attributions:
          'Kartendaten: &copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap-Mitwirkende</a>, SRTM | Kartendarstellung: &copy; <a href="https://opentopomap.org/about">OpenTopoMap</a> (CC-BY-SA)',
        maxZoom: 17,
      }),
  },
] as const;

type MapLayerKey = (typeof mapLayerOptions)[number]["key"];

function readStoredMapLayer(): MapLayerKey {
  const rawValue = localStorage.getItem(mapLayerStorageKey);
  return mapLayerOptions.some((option) => option.key === rawValue)
    ? (rawValue as MapLayerKey)
    : "osm-standard";
}

export function createMapPaneView(
  onToggle: (expanded: boolean) => void,
  initialDisplaySettings: DisplaySettings,
): MapPaneView {
  let displaySettings = initialDisplaySettings;
  let latestStack: BindingStackEntry[] = [];
  let latestMapGroup: PlotGroup | null = null;
  let latestMapSignature = "";
  let selectedMapLayerKey = readStoredMapLayer();

  const mapMeta = document.createElement("div");
  mapMeta.className = "map-meta";

  const mapControls = document.createElement("div");
  mapControls.className = "plot-controls map-controls";

  const mapStats = document.createElement("span");
  mapStats.className = "map-stats";

  const mapLayerLabel = document.createElement("label");
  mapLayerLabel.className = "plot-toggle";

  const mapLayerSelect = document.createElement("select");
  mapLayerSelect.className = "map-layer-select";
  mapLayerOptions.forEach((option) => {
    const element = document.createElement("option");
    element.value = option.key;
    element.textContent = option.label;
    mapLayerSelect.append(element);
  });
  mapLayerSelect.value = selectedMapLayerKey;

  const mapLayerText = document.createElement("span");
  mapLayerText.textContent = "Layer";

  mapLayerLabel.append(mapLayerText, mapLayerSelect);

  const mapLineToggleLabel = document.createElement("label");
  mapLineToggleLabel.className = "plot-toggle";

  const connectMapLinesToggle = document.createElement("input");
  connectMapLinesToggle.type = "checkbox";
  connectMapLinesToggle.checked = displaySettings.mapDefaultConnectLines;

  const connectMapLinesLabel = document.createElement("span");
  connectMapLinesLabel.textContent = "Connect lines";

  mapLineToggleLabel.append(connectMapLinesToggle, connectMapLinesLabel);
  mapControls.append(mapLayerLabel, mapLineToggleLabel, mapStats);

  const mapElement = document.createElement("div");
  mapElement.className = "map-canvas";

  const mapResizeHandle = document.createElement("div");
  mapResizeHandle.className = "map-resize-handle";
  mapResizeHandle.setAttribute("role", "separator");
  mapResizeHandle.setAttribute("aria-orientation", "horizontal");
  mapResizeHandle.setAttribute("aria-label", "Resize map height");

  const mapPointSource = new VectorSource();
  const mapLineSource = new VectorSource();
  const mapAreaSource = new VectorSource();

  const mapAreaLayer = new VectorLayer({
    source: mapAreaSource,
    style: (feature) => {
      const index = Number(feature.get("seriesIndex") ?? 0) % mapPalette.length;
      const palette = mapPalette[index];
      return new Style({
        fill: new Fill({ color: palette.areaFill }),
      });
    },
  });

  const mapPointLayer = new VectorLayer({
    source: mapPointSource,
    style: (feature) => {
      const index = Number(feature.get("seriesIndex") ?? 0) % mapPalette.length;
      const palette = mapPalette[index];
      return new Style({
        image: new CircleStyle({
          radius: 4,
          fill: new Fill({ color: palette.pointFill }),
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

  const baseLayer = new TileLayer({
    source:
      mapLayerOptions.find((option) => option.key === selectedMapLayerKey)?.createSource() ??
      new OSM(),
  });

  const map = new OlMap({
    target: mapElement,
    layers: [
      baseLayer,
      mapAreaLayer,
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

  const animateViewTo = (center: [number, number], zoom: number, animate: boolean) => {
    const view = map.getView();
    if (!animate) {
      view.setCenter(center);
      view.setZoom(zoom);
      return;
    }

    view.cancelAnimations();
    view.animate(
      {
        center,
        zoom,
        duration: 260,
        easing: (value) => 1 - Math.pow(1 - value, 3),
      },
    );
  };

  const currentMapLayerLabel = () =>
    mapLayerOptions.find((option) => option.key === selectedMapLayerKey)?.label ?? "OSM Standard";

  const buildMapSignature = (group: PlotGroup | null) => {
    if (group == null || group.kind !== "position") {
      return "";
    }

    return group.items
      .map((series) =>
        series.points
          .map(
            (point) =>
              `${point.latitude_deg.toFixed(6)}:${point.longitude_deg.toFixed(6)}`,
          )
          .join("|"),
      )
      .join("||");
  };

  const applyMapView = (
    group: PlotGroup | null,
    options: ApplyMapViewOptions = {},
  ) => {
    const animate = options.animate ?? false;
    const view = map.getView();
    if (group == null || group.kind !== "position") {
      animateViewTo(fromLonLat([0, 0]), 1.5, animate);
      return;
    }

    const mapCoordinates = toMapCoordinates(
      (group.items as PositionPlotSeries[]).flatMap((series) => series.points),
    );
    if (mapCoordinates.length === 0) {
      animateViewTo(fromLonLat([0, 0]), 1.5, animate);
      return;
    }
    if (mapCoordinates.length === 1) {
      animateViewTo(mapCoordinates[0] as [number, number], 8, animate);
      return;
    }

    view.cancelAnimations();
    view.fit(mapPointSource.getExtent(), {
      size: map.getSize(),
      padding: [48, 48, 48, 48],
      maxZoom: 12,
      duration: animate ? 260 : 0,
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

  const renderMap = (groupList: PlotGroup[], options: ApplyMapViewOptions = {}) => {
    const currentGroup = [...groupList].reverse().find((group) => group.kind === "position");
    const nextSignature = buildMapSignature(currentGroup ?? null);
    const shouldAnimate = (options.animate ?? false) && nextSignature !== latestMapSignature;
    pane.count.textContent = `${currentGroup?.items.length ?? 0}`;
    latestMapGroup = currentGroup ?? null;
    latestMapSignature = nextSignature;

    if (currentGroup == null || currentGroup.kind !== "position") {
      mapMeta.textContent = "";
      mapStats.textContent = "world";
      mapAreaSource.clear();
      mapPointSource.clear();
      mapLineSource.clear();
      requestAnimationFrame(() => {
        map.updateSize();
        applyMapView(null, { animate: shouldAnimate });
      });
      return;
    }

    const seriesList = currentGroup.items as PositionPlotSeries[];
    const totalPositions = seriesList.reduce(
      (sum, series) => sum + series.points.length,
      0,
    );
    mapMeta.textContent = currentGroup.label;
    mapStats.textContent = `${seriesList.length} paths | ${totalPositions} points`;
    mapAreaSource.clear();
    mapPointSource.clear();
    mapLineSource.clear();

    seriesList.forEach((series, seriesIndex) => {
      const mapCoordinates = toMapCoordinates(series.points);
      if (isClosedPositionSeries(series) && mapCoordinates.length >= 4) {
        const feature = new Feature(new Polygon([mapCoordinates]));
        feature.set("seriesIndex", seriesIndex);
        mapAreaSource.addFeature(feature);
      }
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
      applyMapView(currentGroup, { animate: shouldAnimate });
    });
  };

  const rerenderMap = () => {
    renderMap(latestStack.map((entry) => parseMapGroup(entry)).filter(isPlotGroup));
  };

  const refreshLayout = () => {
    requestAnimationFrame(() => {
      map.updateSize();
      if (!pane.body.hidden) {
        applyMapView(latestMapGroup);
      }
    });
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
  mapLayerSelect.addEventListener("change", () => {
    const nextKey = mapLayerSelect.value as MapLayerKey;
    selectedMapLayerKey = nextKey;
    localStorage.setItem(mapLayerStorageKey, selectedMapLayerKey);
    baseLayer.setSource(
      mapLayerOptions.find((option) => option.key === selectedMapLayerKey)?.createSource() ??
        new OSM(),
    );
    rerenderMap();
  });

  pane.body.append(mapMeta, mapControls, mapElement, mapResizeHandle);

  return {
    pane,
    render(stack) {
      latestStack = stack;
      renderMap(latestStack.map((entry) => parseMapGroup(entry)).filter(isPlotGroup), {
        animate: true,
      });
    },
    setDisplaySettings(settings) {
      displaySettings = settings;
      connectMapLinesToggle.checked = displaySettings.mapDefaultConnectLines;
      rerenderMap();
    },
    refreshLayout,
  };
}
