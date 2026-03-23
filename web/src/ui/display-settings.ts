export interface DisplaySettings {
  transcriptDecimals: number;
  stackDecimals: number;
  listPreviewLength: number;
  transcriptListClamp: 0 | 2 | 4;
  showTimings: boolean;
  plotDefaultLine: boolean;
  plotDefaultPoints: boolean;
  mapDefaultConnectLines: boolean;
  rememberPaneState: boolean;
}

export const defaultDisplaySettings: DisplaySettings = {
  transcriptDecimals: 8,
  stackDecimals: 6,
  listPreviewLength: 2,
  transcriptListClamp: 2,
  showTimings: true,
  plotDefaultLine: true,
  plotDefaultPoints: false,
  mapDefaultConnectLines: false,
  rememberPaneState: true,
};

const transcriptDecimalsKey = "console-calc-transcript-decimals";
const stackDecimalsKey = "console-calc-stack-decimals";
const listPreviewLengthKey = "console-calc-list-preview-length";
const transcriptListClampKey = "console-calc-transcript-list-clamp";
const showTimingsKey = "console-calc-show-timings";
const plotDefaultLineKey = "console-calc-plot-default-line";
const plotDefaultPointsKey = "console-calc-plot-default-points";
const mapDefaultConnectLinesKey = "console-calc-map-default-connect-lines";
const rememberPaneStateKey = "console-calc-remember-pane-state";

function clampDecimals(value: number, fallback: number): number {
  if (!Number.isFinite(value)) {
    return fallback;
  }

  return Math.max(0, Math.min(12, Math.round(value)));
}

function readStoredNumber(key: string, fallback: number): number {
  const rawValue = localStorage.getItem(key);
  if (rawValue === null) {
    return fallback;
  }

  const parsed = Number.parseInt(rawValue, 10);
  return clampDecimals(parsed, fallback);
}

function clampPreviewLength(value: number, fallback: number): number {
  if (!Number.isFinite(value)) {
    return fallback;
  }

  return Math.max(1, Math.min(8, Math.round(value)));
}

function clampTranscriptListClamp(value: number, fallback: 0 | 2 | 4): 0 | 2 | 4 {
  if (value === 0 || value === 2 || value === 4) {
    return value;
  }

  return fallback;
}

function readStoredBoolean(key: string, fallback: boolean): boolean {
  const rawValue = localStorage.getItem(key);
  if (rawValue === null) {
    return fallback;
  }

  return rawValue === "true";
}

export function loadDisplaySettings(): DisplaySettings {
  return {
    transcriptDecimals: readStoredNumber(
      transcriptDecimalsKey,
      defaultDisplaySettings.transcriptDecimals,
    ),
    stackDecimals: readStoredNumber(stackDecimalsKey, defaultDisplaySettings.stackDecimals),
    listPreviewLength: clampPreviewLength(
      readStoredNumber(listPreviewLengthKey, defaultDisplaySettings.listPreviewLength),
      defaultDisplaySettings.listPreviewLength,
    ),
    transcriptListClamp: clampTranscriptListClamp(
      readStoredNumber(transcriptListClampKey, defaultDisplaySettings.transcriptListClamp),
      defaultDisplaySettings.transcriptListClamp,
    ),
    showTimings: readStoredBoolean(showTimingsKey, defaultDisplaySettings.showTimings),
    plotDefaultLine: readStoredBoolean(plotDefaultLineKey, defaultDisplaySettings.plotDefaultLine),
    plotDefaultPoints: readStoredBoolean(
      plotDefaultPointsKey,
      defaultDisplaySettings.plotDefaultPoints,
    ),
    mapDefaultConnectLines: readStoredBoolean(
      mapDefaultConnectLinesKey,
      defaultDisplaySettings.mapDefaultConnectLines,
    ),
    rememberPaneState: readStoredBoolean(
      rememberPaneStateKey,
      defaultDisplaySettings.rememberPaneState,
    ),
  };
}

export function saveDisplaySettings(settings: DisplaySettings): DisplaySettings {
  const normalized = {
    transcriptDecimals: clampDecimals(
      settings.transcriptDecimals,
      defaultDisplaySettings.transcriptDecimals,
    ),
    stackDecimals: clampDecimals(settings.stackDecimals, defaultDisplaySettings.stackDecimals),
    listPreviewLength: clampPreviewLength(
      settings.listPreviewLength,
      defaultDisplaySettings.listPreviewLength,
    ),
    transcriptListClamp: clampTranscriptListClamp(
      settings.transcriptListClamp,
      defaultDisplaySettings.transcriptListClamp,
    ),
    showTimings: settings.showTimings,
    plotDefaultLine: settings.plotDefaultLine,
    plotDefaultPoints: settings.plotDefaultPoints,
    mapDefaultConnectLines: settings.mapDefaultConnectLines,
    rememberPaneState: settings.rememberPaneState,
  };

  localStorage.setItem(transcriptDecimalsKey, `${normalized.transcriptDecimals}`);
  localStorage.setItem(stackDecimalsKey, `${normalized.stackDecimals}`);
  localStorage.setItem(listPreviewLengthKey, `${normalized.listPreviewLength}`);
  localStorage.setItem(transcriptListClampKey, `${normalized.transcriptListClamp}`);
  localStorage.setItem(showTimingsKey, `${normalized.showTimings}`);
  localStorage.setItem(plotDefaultLineKey, `${normalized.plotDefaultLine}`);
  localStorage.setItem(plotDefaultPointsKey, `${normalized.plotDefaultPoints}`);
  localStorage.setItem(mapDefaultConnectLinesKey, `${normalized.mapDefaultConnectLines}`);
  localStorage.setItem(rememberPaneStateKey, `${normalized.rememberPaneState}`);
  return normalized;
}

function formatDecimal(value: number, decimals: number): string {
  if (!Number.isFinite(value)) {
    return `${value}`;
  }

  const fixed = value.toFixed(decimals);
  const trimmed = fixed
    .replace(/(\.\d*?[1-9])0+$/u, "$1")
    .replace(/\.0+$/u, "")
    .replace(/^-0$/u, "0");
  return trimmed;
}

export function formatNumericText(text: string, decimals: number): string {
  const decimalPattern =
    /(^|[^A-Za-z0-9_])([-+]?(?:\d+\.\d*|\d*\.\d+)(?:[eE][-+]?\d+)?)(?=$|[^A-Za-z0-9_])/gu;

  return text.replace(decimalPattern, (match, prefix: string, numericText: string) => {
    const numericValue = Number(numericText);
    if (!Number.isFinite(numericValue)) {
      return match;
    }

    return `${prefix}${formatDecimal(numericValue, decimals)}`;
  });
}
