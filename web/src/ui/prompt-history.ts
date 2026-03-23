export interface PromptHistory {
  push(value: string): void;
  previous(currentDraft: string): string | null;
  next(): string | null;
  reset(): void;
  hasEntries(): boolean;
}

const promptHistoryStorageKey = "console-calc-prompt-history";

function loadEntries(limit: number): string[] {
  const rawValue = localStorage.getItem(promptHistoryStorageKey);
  if (rawValue === null) {
    return [];
  }

  try {
    const parsed = JSON.parse(rawValue);
    if (!Array.isArray(parsed)) {
      return [];
    }

    return parsed
      .filter((entry): entry is string => typeof entry === "string" && entry.length > 0)
      .slice(-limit);
  } catch {
    return [];
  }
}

function saveEntries(entries: string[]): void {
  localStorage.setItem(promptHistoryStorageKey, JSON.stringify(entries));
}

export function createPromptHistory(limit = 100): PromptHistory {
  const entries: string[] = loadEntries(limit);
  let historyIndex = -1;
  let draftInput = "";

  return {
    push(value) {
      if (entries.at(-1) !== value) {
        entries.push(value);
        if (entries.length > limit) {
          entries.shift();
        }
        saveEntries(entries);
      }
      historyIndex = -1;
      draftInput = "";
    },
    previous(currentDraft) {
      if (entries.length === 0) {
        return null;
      }

      if (historyIndex === -1) {
        draftInput = currentDraft;
        historyIndex = entries.length - 1;
      } else if (historyIndex > 0) {
        historyIndex -= 1;
      }

      return entries[historyIndex] ?? "";
    },
    next() {
      if (historyIndex === -1) {
        return null;
      }

      if (historyIndex < entries.length - 1) {
        historyIndex += 1;
        return entries[historyIndex] ?? "";
      }

      historyIndex = -1;
      return draftInput;
    },
    reset() {
      historyIndex = -1;
      draftInput = "";
    },
    hasEntries() {
      return entries.length > 0;
    },
  };
}
