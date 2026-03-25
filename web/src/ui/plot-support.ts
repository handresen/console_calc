import type { BindingPositionEntry, BindingStackEntry } from "../bridge/console-wasm";
import { fromLonLat } from "ol/proj";

export interface PlotSeries {
  kind: "scalar";
  key: string;
  label: string;
  values: number[];
  rawValues: number[];
  truncated: boolean;
}

export interface PositionPlotSeries {
  kind: "position";
  key: string;
  label: string;
  points: BindingPositionEntry[];
  rawPoints: BindingPositionEntry[];
  truncated: boolean;
}

export type PlotItem = PlotSeries | PositionPlotSeries;

export interface PlotGroup {
  key: string;
  label: string;
  kind: "scalar" | "position";
  items: PlotItem[];
}

export type PlotPoint = { x: number; y: number };

const scalarPlotVerticalPadding = 5;

export function isPlotGroup(value: PlotGroup | null): value is PlotGroup {
  return value !== null;
}

function sampleScalarValues(listValues: number[]): { values: number[]; truncated: boolean } {
  if (listValues.length <= 500) {
    return { values: listValues, truncated: false };
  }
  return {
    values: Array.from({ length: 500 }, (_, index) => {
      const sourceIndex = Math.round((index / 499) * (listValues.length - 1));
      return listValues[sourceIndex] ?? 0;
    }),
    truncated: true,
  };
}

function samplePositionValues(
  positionListValues: BindingPositionEntry[],
): { points: BindingPositionEntry[]; truncated: boolean } {
  const maxPositionPoints = 10000;
  if (positionListValues.length <= maxPositionPoints) {
    return { points: positionListValues, truncated: positionListValues.length > 500 };
  }
  return {
    points: Array.from({ length: maxPositionPoints }, (_, index) => {
      const sourceIndex = Math.round(
        (index / (maxPositionPoints - 1)) * (positionListValues.length - 1),
      );
      return positionListValues[sourceIndex] ?? {
        latitude_deg: 0,
        longitude_deg: 0,
      };
    }),
    truncated: true,
  };
}

export function parsePlotGroup(entry: BindingStackEntry): PlotGroup | null {
  const multiPositionListValues = entry.multi_position_list_values ?? [];
  if (multiPositionListValues.length > 0) {
    return {
      key: `stack-${entry.level}`,
      label: `Stack ${entry.level}`,
      kind: "position",
      items: multiPositionListValues.map((points, index) => {
        const sampled = samplePositionValues(points);
        return {
          kind: "position",
          key: `stack-${entry.level}-pos-${index}`,
          label: `Stack ${entry.level}[${index}]`,
          points: sampled.points,
          rawPoints: points,
          truncated: sampled.truncated,
        } satisfies PositionPlotSeries;
      }),
    };
  }

  const positionListValues = entry.position_list_values ?? [];
  if (positionListValues.length > 0 || entry.display.trim().startsWith("{pos(")) {
    const sampled = samplePositionValues(positionListValues);
    return {
      key: `stack-${entry.level}`,
      label: `Stack ${entry.level}`,
      kind: "position",
      items: [
        {
          kind: "position",
          key: `stack-${entry.level}`,
          label: `Stack ${entry.level}`,
          points: sampled.points,
          rawPoints: positionListValues,
          truncated: sampled.truncated,
        },
      ],
    };
  }

  const multiListValues = entry.multi_list_values ?? [];
  if (multiListValues.length > 0) {
    return {
      key: `stack-${entry.level}`,
      label: `Stack ${entry.level}`,
      kind: "scalar",
      items: multiListValues.map((values, index) => {
        const sampled = sampleScalarValues(values);
        return {
          kind: "scalar",
          key: `stack-${entry.level}-list-${index}`,
          label: `Stack ${entry.level}[${index}]`,
          values: sampled.values,
          rawValues: values,
          truncated: sampled.truncated,
        } satisfies PlotSeries;
      }),
    };
  }

  const listValues = entry.list_values ?? [];
  if (listValues.length === 0 && !entry.display.trim().startsWith("{}")) {
    return null;
  }
  const sampled = sampleScalarValues(listValues);
  return {
    key: `stack-${entry.level}`,
    label: `Stack ${entry.level}`,
    kind: "scalar",
    items: [
      {
        kind: "scalar",
        key: `stack-${entry.level}`,
        label: `Stack ${entry.level}`,
        values: sampled.values,
        rawValues: listValues,
        truncated: sampled.truncated,
      },
    ],
  };
}

export function buildScalarBounds(values: number[]): {
  min: number;
  max: number;
  range: number;
} {
  const min = Math.min(...values);
  const max = Math.max(...values);
  return {
    min,
    max,
    range: max - min || 1,
  };
}

export function buildScalarGroupBounds(seriesList: PlotSeries[]): {
  min: number;
  max: number;
  range: number;
} {
  const allValues = seriesList.flatMap((series) => series.rawValues);
  return buildScalarBounds(allValues.length > 0 ? allValues : [0]);
}

export function formatScalarAxisLabel(value: number): string {
  return value.toPrecision(6);
}

export function buildPlotPath(
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

export function buildZeroAxisPath(
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

export function buildPositionBounds(points: BindingPositionEntry[]): {
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

export function buildPositionGroupBounds(seriesList: PositionPlotSeries[]): {
  minX: number;
  maxX: number;
  minY: number;
  maxY: number;
} {
  const allPoints = seriesList.flatMap((series) => series.rawPoints);
  return buildPositionBounds(
    allPoints.length > 0
      ? allPoints
      : [{ latitude_deg: 0, longitude_deg: 0 }],
  );
}

function formatLatitude(value: number): string {
  const suffix = value < 0 ? "S" : "N";
  return `${Math.abs(value).toFixed(1).padStart(4, "0")}${suffix}`;
}

function formatLongitude(value: number): string {
  const suffix = value < 0 ? "W" : "E";
  return `${Math.abs(value).toFixed(1).padStart(5, "0")}${suffix}`;
}

export function formatCornerLabel(latitude: number, longitude: number): string {
  return `${formatLatitude(latitude)}-${formatLongitude(longitude)}`;
}

export function buildPositionPlotPath(
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

export function buildPositionReferenceAxes(
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

export function toMapCoordinates(points: BindingPositionEntry[]): [number, number][] {
  const maxMercatorLatitude = 85.05112878;
  return points.map((point) =>
    fromLonLat([
      point.longitude_deg,
      Math.max(-maxMercatorLatitude, Math.min(maxMercatorLatitude, point.latitude_deg)),
    ]),
  );
}

export function buildScalarPlotPoints(
  values: number[],
  width: number,
  height: number,
  bounds = buildScalarBounds(values),
): PlotPoint[] {
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
}

export function buildPositionPlotPoints(
  points: BindingPositionEntry[],
  width: number,
  height: number,
  bounds = buildPositionBounds(points),
): PlotPoint[] {
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
}
