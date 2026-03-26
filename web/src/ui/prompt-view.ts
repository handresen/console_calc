import { createPromptHistory } from "./prompt-history";

export interface PromptView {
  element: HTMLElement;
  setEnabled(enabled: boolean): void;
  setDepth(depth: number): void;
  setPlaceholder(text: string): void;
  setValidityState(
    state: "neutral" | "valid" | "invalid",
    invalidStart?: number,
  ): void;
  setValue(value: string): void;
  submit(value: string): void;
  focus(): void;
}

type ValidityState = "neutral" | "valid" | "invalid";

interface BracePair {
  openIndex: number;
  closeIndex: number;
}

function escapeHtml(text: string): string {
  return text
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");
}

function isBraceCharacter(character: string): boolean {
  return character === "(" || character === ")" || character === "{" || character === "}" ||
    character === "[" || character === "]";
}

function matchingBrace(character: string): string | null {
  switch (character) {
    case "(":
      return ")";
    case ")":
      return "(";
    case "{":
      return "}";
    case "}":
      return "{";
    case "[":
      return "]";
    case "]":
      return "[";
    default:
      return null;
  }
}

function findMatchingBracePair(text: string, cursorStart: number, cursorEnd: number): BracePair | null {
  if (cursorStart !== cursorEnd) {
    return null;
  }

  const candidateIndices: number[] = [];
  if (cursorStart > 0 && isBraceCharacter(text[cursorStart - 1]!)) {
    candidateIndices.push(cursorStart - 1);
  }
  if (cursorStart < text.length && isBraceCharacter(text[cursorStart]!)) {
    candidateIndices.push(cursorStart);
  }

  for (const index of candidateIndices) {
    const character = text[index]!;
    const counterpart = matchingBrace(character);
    if (counterpart === null) {
      continue;
    }

    if (character === "(" || character === "{" || character === "[") {
      let depth = 0;
      for (let position = index; position < text.length; position += 1) {
        const current = text[position]!;
        if (current === character) {
          depth += 1;
        } else if (current === counterpart) {
          depth -= 1;
          if (depth === 0) {
            return { openIndex: index, closeIndex: position };
          }
        }
      }
      continue;
    }

    let depth = 0;
    for (let position = index; position >= 0; position -= 1) {
      const current = text[position]!;
      if (current === character) {
        depth += 1;
      } else if (current === counterpart) {
        depth -= 1;
        if (depth === 0) {
          return { openIndex: position, closeIndex: index };
        }
      }
    }
  }

  return null;
}

function renderHighlightedText(
  text: string,
  pair: BracePair | null,
  validityState: ValidityState,
  invalidStart: number | null,
): string {
  if (text.length === 0) {
    return "";
  }

  let html = "";
  for (let index = 0; index < text.length; index += 1) {
    const character = escapeHtml(text[index]!);
    const classes: string[] = [];
    if (pair !== null && (index === pair.openIndex || index === pair.closeIndex)) {
      classes.push("prompt-brace-match");
    }
    if (validityState === "invalid" && invalidStart !== null && index >= invalidStart) {
      classes.push("prompt-invalid-suffix");
    }

    if (classes.length === 0) {
      html += character;
      continue;
    }

    html += `<span class="${classes.join(" ")}">${character}</span>`;
  }
  return html;
}

export function createPromptView(
  onSubmit: (input: string) => void,
  onInputChanged?: (input: string) => void,
): PromptView {
  const history = createPromptHistory();
  let validityState: ValidityState = "neutral";
  let invalidStart: number | null = null;

  const wrapper = document.createElement("section");
  wrapper.className = "prompt-view";

  const promptLabel = document.createElement("span");
  promptLabel.className = "prompt-label";
  promptLabel.textContent = "0>";

  const editorShell = document.createElement("div");
  editorShell.className = "prompt-editor-shell";
  editorShell.dataset.validityState = validityState;

  const placeholder = document.createElement("div");
  placeholder.className = "prompt-placeholder";
  placeholder.textContent = "0> enter expression or command";

  const mirror = document.createElement("div");
  mirror.className = "prompt-mirror";

  const input = document.createElement("textarea");
  input.className = "prompt-input";
  input.rows = 1;
  input.wrap = "off";
  input.autocomplete = "off";
  input.spellcheck = false;

  const syncMirror = () => {
    const pair = findMatchingBracePair(input.value, input.selectionStart, input.selectionEnd);
    mirror.innerHTML = `${renderHighlightedText(input.value, pair, validityState, invalidStart)}<span class="prompt-mirror-trailing-space"> </span>`;
    mirror.style.transform = `translateX(${-input.scrollLeft}px)`;
    placeholder.hidden = input.value.length !== 0;
  };

  const notifyInputChanged = () => {
    syncMirror();
    onInputChanged?.(input.value);
  };

  const submitValue = (value: string) => {
    const trimmedValue = value.trim();
    if (trimmedValue.length === 0) {
      return;
    }

    history.push(trimmedValue);
    input.value = "";
    syncMirror();
    onInputChanged?.(input.value);
    onSubmit(trimmedValue);
  };

  input.addEventListener("input", () => {
    notifyInputChanged();
  });
  input.addEventListener("scroll", () => {
    syncMirror();
  });
  input.addEventListener("click", syncMirror);
  input.addEventListener("keyup", syncMirror);
  input.addEventListener("select", syncMirror);
  input.addEventListener("keydown", (event) => {
    if (event.key === "ArrowUp") {
      if (!history.hasEntries()) {
        return;
      }

      event.preventDefault();
      input.value = history.previous(input.value) ?? input.value;
      input.setSelectionRange(input.value.length, input.value.length);
      notifyInputChanged();
      return;
    }

    if (event.key === "ArrowDown") {
      const nextValue = history.next();
      if (nextValue === null) {
        return;
      }

      event.preventDefault();
      input.value = nextValue;
      input.setSelectionRange(input.value.length, input.value.length);
      notifyInputChanged();
      return;
    }

    if (event.key !== "Enter") {
      return;
    }

    event.preventDefault();
    submitValue(input.value);
  });

  editorShell.append(placeholder, mirror, input);
  wrapper.append(promptLabel, editorShell);
  syncMirror();

  return {
    element: wrapper,
    setEnabled(enabled) {
      input.disabled = !enabled;
    },
    setDepth(depth) {
      promptLabel.textContent = `${depth}>`;
    },
    setPlaceholder(text) {
      placeholder.textContent = text;
    },
    setValidityState(state, nextInvalidStart) {
      validityState = state;
      invalidStart =
        state === "invalid" && nextInvalidStart !== undefined ? nextInvalidStart : null;
      editorShell.dataset.validityState = validityState;
      syncMirror();
    },
    setValue(value) {
      input.value = value;
      input.setSelectionRange(input.value.length, input.value.length);
      notifyInputChanged();
    },
    submit(value) {
      submitValue(value);
    },
    focus() {
      input.focus();
    },
  };
}
