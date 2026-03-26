import { createPromptHistory } from "./prompt-history";

export interface PromptView {
  element: HTMLElement;
  setEnabled(enabled: boolean): void;
  setDepth(depth: number): void;
  setPlaceholder(text: string): void;
  setValidityState(state: "neutral" | "valid" | "invalid"): void;
  setValue(value: string): void;
  submit(value: string): void;
  focus(): void;
}

export function createPromptView(
  onSubmit: (input: string) => void,
  onInputChanged?: (input: string) => void,
): PromptView {
  const history = createPromptHistory();

  const submitValue = (value: string) => {
    const trimmedValue = value.trim();
    if (trimmedValue.length === 0) {
      return;
    }

    history.push(trimmedValue);
    input.value = "";
    onSubmit(trimmedValue);
  };

  const wrapper = document.createElement("section");
  wrapper.className = "prompt-view";

  const promptLabel = document.createElement("span");
  promptLabel.className = "prompt-label";
  promptLabel.textContent = "0>";

  const input = document.createElement("input");
  input.type = "text";
  input.placeholder = "0> enter expression or command";
  input.autocomplete = "off";
  input.spellcheck = false;
  input.addEventListener("input", () => {
    onInputChanged?.(input.value);
  });
  input.addEventListener("keydown", (event) => {
    if (event.key === "ArrowUp") {
      if (!history.hasEntries()) {
        return;
      }

      event.preventDefault();
      input.value = history.previous(input.value) ?? input.value;
      input.setSelectionRange(input.value.length, input.value.length);
      onInputChanged?.(input.value);
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
      onInputChanged?.(input.value);
      return;
    }

    if (event.key !== "Enter") {
      return;
    }

    submitValue(input.value);
  });

  wrapper.append(promptLabel, input);
  return {
    element: wrapper,
    setEnabled(enabled) {
      input.disabled = !enabled;
    },
    setDepth(depth) {
      promptLabel.textContent = `${depth}>`;
    },
    setPlaceholder(text) {
      input.placeholder = text;
    },
    setValidityState(state) {
      input.dataset.validityState = state;
    },
    setValue(value) {
      input.value = value;
      input.setSelectionRange(input.value.length, input.value.length);
      onInputChanged?.(input.value);
    },
    submit(value) {
      submitValue(value);
    },
    focus() {
      input.focus();
    },
  };
}
