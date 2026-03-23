export interface PromptHistory {
  push(value: string): void;
  previous(currentDraft: string): string | null;
  next(): string | null;
  reset(): void;
  hasEntries(): boolean;
}

export function createPromptHistory(limit = 100): PromptHistory {
  const entries: string[] = [];
  let historyIndex = -1;
  let draftInput = "";

  return {
    push(value) {
      if (entries.at(-1) !== value) {
        entries.push(value);
        if (entries.length > limit) {
          entries.shift();
        }
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
