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
const userMapLayersStorageKey = "console-calc-user-map-layers";
const manageMapLayersKey = "__manage__";

const builtinMapLayerOptions = [
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
  {
    key: "carto-voyager",
    label: "CARTO Voyager",
    createSource: () =>
      new XYZ({
        urls: [
          "https://a.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}.png",
          "https://b.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}.png",
          "https://c.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}.png",
          "https://d.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}.png",
        ],
        attributions:
          '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors, &copy; <a href="https://carto.com/attributions">CARTO</a>',
        maxZoom: 20,
      }),
  },
  {
    key: "carto-dark-matter",
    label: "CARTO Dark Matter",
    createSource: () =>
      new XYZ({
        urls: [
          "https://a.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png",
          "https://b.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png",
          "https://c.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png",
          "https://d.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png",
        ],
        attributions:
          '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors, &copy; <a href="https://carto.com/attributions">CARTO</a>',
        maxZoom: 20,
      }),
  },
];

type UserMapLayer = {
  key: string;
  label: string;
  urlTemplate: string;
};

type MapLayerOption = {
  key: string;
  label: string;
  createSource: () => OSM | XYZ;
};

function readStoredMapLayer(): string {
  return localStorage.getItem(mapLayerStorageKey) ?? "osm-standard";
}

function readStoredUserMapLayers(): UserMapLayer[] {
  const rawValue = localStorage.getItem(userMapLayersStorageKey);
  if (rawValue == null) {
    return [];
  }

  try {
    const parsed = JSON.parse(rawValue) as UserMapLayer[];
    return parsed.filter(
      (entry) =>
        typeof entry.key === "string" &&
        typeof entry.label === "string" &&
        typeof entry.urlTemplate === "string",
    );
  } catch {
    return [];
  }
}

function saveUserMapLayers(entries: UserMapLayer[]) {
  localStorage.setItem(userMapLayersStorageKey, JSON.stringify(entries));
}

function toUserMapLayerOption(entry: UserMapLayer): MapLayerOption {
  return {
    key: entry.key,
    label: entry.label,
    createSource: () =>
      new XYZ({
        url: entry.urlTemplate,
        maxZoom: 20,
      }),
  };
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
  let userMapLayers = readStoredUserMapLayers();

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

  const mapLayersDialog = document.createElement("dialog");
  mapLayersDialog.className = "settings-dialog map-layers-dialog";

  const mapLayersForm = document.createElement("form");
  mapLayersForm.method = "dialog";
  mapLayersForm.className = "settings-form";

  const mapLayersHeading = document.createElement("h2");
  mapLayersHeading.className = "settings-heading";
  mapLayersHeading.textContent = "Manage basemaps";

  const mapLayersIntro = document.createElement("p");
  mapLayersIntro.className = "map-layers-dialog-copy";
  mapLayersIntro.textContent =
    "User basemap setup will live here. This first step only adds the local management shell.";

  const mapLayersPlaceholder = document.createElement("div");
  mapLayersPlaceholder.className = "map-layers-placeholder";
  const mapLayersList = document.createElement("div");
  mapLayersList.className = "map-layers-list";

  const mapLayerNameField = document.createElement("label");
  mapLayerNameField.className = "settings-field";
  const mapLayerNameLabel = document.createElement("span");
  mapLayerNameLabel.textContent = "Name";
  const mapLayerNameInput = document.createElement("input");
  mapLayerNameInput.type = "text";
  mapLayerNameInput.placeholder = "My basemap";
  mapLayerNameField.append(mapLayerNameLabel, mapLayerNameInput);

  const mapLayerUrlField = document.createElement("label");
  mapLayerUrlField.className = "settings-field";
  const mapLayerUrlLabel = document.createElement("span");
  mapLayerUrlLabel.textContent = "URL template";
  const mapLayerUrlInput = document.createElement("input");
  mapLayerUrlInput.type = "text";
  mapLayerUrlInput.placeholder =
    "https://localhost:5103/...&lvl={z}&col={x}&row={y}";
  mapLayerUrlField.append(mapLayerUrlLabel, mapLayerUrlInput);

  const mapLayersHint = document.createElement("p");
  mapLayersHint.className = "map-layers-dialog-copy";
  mapLayersHint.textContent = "Use {z}, {x}, and {y} placeholders in the tile URL template.";

  const mapLayersAddButton = document.createElement("button");
  mapLayersAddButton.type = "button";
  mapLayersAddButton.className = "toolbar-button";
  mapLayersAddButton.textContent = "Add basemap";

  const mapLayersActions = document.createElement("div");
  mapLayersActions.className = "settings-actions";

  const mapLayersCloseButton = document.createElement("button");
  mapLayersCloseButton.type = "submit";
  mapLayersCloseButton.className = "toolbar-button";
  mapLayersCloseButton.textContent = "Close";

  mapLayersActions.append(mapLayersCloseButton);
  mapLayersForm.append(
    mapLayersHeading,
    mapLayersIntro,
    mapLayerNameField,
    mapLayerUrlField,
    mapLayersHint,
    mapLayersAddButton,
    mapLayersPlaceholder,
    mapLayersList,
    mapLayersActions,
  );
  mapLayersDialog.append(mapLayersForm);

  const allMapLayerOptions = (): MapLayerOption[] => [
    ...builtinMapLayerOptions,
    ...userMapLayers.map(toUserMapLayerOption),
  ];

  const findMapLayerOption = (key: string): MapLayerOption | undefined =>
    allMapLayerOptions().find((option) => option.key === key);

  const syncMapLayerSelect = () => {
    mapLayerSelect.replaceChildren();
    allMapLayerOptions().forEach((option) => {
      const element = document.createElement("option");
      element.value = option.key;
      element.textContent = option.label;
      mapLayerSelect.append(element);
    });
    const manageMapLayersOption = document.createElement("option");
    manageMapLayersOption.value = manageMapLayersKey;
    manageMapLayersOption.textContent = "<Manage>";
    mapLayerSelect.append(manageMapLayersOption);

    if (findMapLayerOption(selectedMapLayerKey) == null) {
      selectedMapLayerKey = "osm-standard";
    }
    mapLayerSelect.value = selectedMapLayerKey;
  };

  const renderUserMapLayers = () => {
    mapLayersList.replaceChildren();
    if (userMapLayers.length === 0) {
      mapLayersPlaceholder.hidden = false;
      mapLayersPlaceholder.textContent = "No user basemaps yet.";
      return;
    }

    mapLayersPlaceholder.hidden = true;
    userMapLayers.forEach((entry) => {
      const row = document.createElement("div");
      row.className = "map-layer-entry";

      const text = document.createElement("div");
      text.className = "map-layer-entry-text";

      const label = document.createElement("div");
      label.className = "map-layer-entry-label";
      label.textContent = entry.label;

      const url = document.createElement("div");
      url.className = "map-layer-entry-url";
      url.textContent = entry.urlTemplate;
      url.title = entry.urlTemplate;

      const removeButton = document.createElement("button");
      removeButton.type = "button";
      removeButton.className = "pane-icon-button";
      removeButton.textContent = "−";
      removeButton.title = "Remove basemap";
      removeButton.setAttribute("aria-label", "Remove basemap");
      removeButton.addEventListener("click", () => {
        userMapLayers = userMapLayers.filter((item) => item.key !== entry.key);
        saveUserMapLayers(userMapLayers);
        if (selectedMapLayerKey === entry.key) {
          selectedMapLayerKey = "osm-standard";
          localStorage.setItem(mapLayerStorageKey, selectedMapLayerKey);
          baseLayer.setSource(new OSM());
        }
        syncMapLayerSelect();
        renderUserMapLayers();
        rerenderMap();
      });

      text.append(label, url);
      row.append(text, removeButton);
      mapLayersList.append(row);
    });
  };

  const isValidMapLayerUrlTemplate = (value: string) =>
    value.includes("{z}") && value.includes("{x}") && value.includes("{y}");

  syncMapLayerSelect();
  renderUserMapLayers();

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
    source: findMapLayerOption(selectedMapLayerKey)?.createSource() ?? new OSM(),
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
    findMapLayerOption(selectedMapLayerKey)?.label ?? "OSM Standard";

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
  mapLayersAddButton.addEventListener("click", () => {
    const label = mapLayerNameInput.value.trim();
    const urlTemplate = mapLayerUrlInput.value.trim();
    if (label === "" || !isValidMapLayerUrlTemplate(urlTemplate)) {
      mapLayersPlaceholder.hidden = false;
      mapLayersPlaceholder.textContent =
        "Enter a name and a URL template containing {z}, {x}, and {y}.";
      return;
    }

    const entry: UserMapLayer = {
      key: `user-${Date.now()}`,
      label,
      urlTemplate,
    };
    userMapLayers = [...userMapLayers, entry];
    saveUserMapLayers(userMapLayers);
    selectedMapLayerKey = entry.key;
    localStorage.setItem(mapLayerStorageKey, selectedMapLayerKey);
    syncMapLayerSelect();
    renderUserMapLayers();
    mapLayerNameInput.value = "";
    mapLayerUrlInput.value = "";
    baseLayer.setSource(toUserMapLayerOption(entry).createSource());
    rerenderMap();
  });
  mapLayerSelect.addEventListener("change", () => {
    if (mapLayerSelect.value === manageMapLayersKey) {
      mapLayerSelect.value = selectedMapLayerKey;
      renderUserMapLayers();
      mapLayersDialog.showModal();
      return;
    }

    const nextKey = mapLayerSelect.value;
    selectedMapLayerKey = nextKey;
    localStorage.setItem(mapLayerStorageKey, selectedMapLayerKey);
    baseLayer.setSource(
      findMapLayerOption(selectedMapLayerKey)?.createSource() ??
        new OSM(),
    );
    rerenderMap();
  });

  pane.body.append(mapMeta, mapControls, mapElement, mapResizeHandle, mapLayersDialog);

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
