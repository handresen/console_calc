export interface DisplaySettings {
  transcriptDecimals: number;
  stackDecimals: number;
}

export const defaultDisplaySettings: DisplaySettings = {
  transcriptDecimals: 8,
  stackDecimals: 6,
};

const transcriptDecimalsKey = "console-calc-transcript-decimals";
const stackDecimalsKey = "console-calc-stack-decimals";

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

export function loadDisplaySettings(): DisplaySettings {
  return {
    transcriptDecimals: readStoredNumber(
      transcriptDecimalsKey,
      defaultDisplaySettings.transcriptDecimals,
    ),
    stackDecimals: readStoredNumber(stackDecimalsKey, defaultDisplaySettings.stackDecimals),
  };
}

export function saveDisplaySettings(settings: DisplaySettings): DisplaySettings {
  const normalized = {
    transcriptDecimals: clampDecimals(
      settings.transcriptDecimals,
      defaultDisplaySettings.transcriptDecimals,
    ),
    stackDecimals: clampDecimals(settings.stackDecimals, defaultDisplaySettings.stackDecimals),
  };

  localStorage.setItem(transcriptDecimalsKey, `${normalized.transcriptDecimals}`);
  localStorage.setItem(stackDecimalsKey, `${normalized.stackDecimals}`);
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

