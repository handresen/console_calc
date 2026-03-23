export interface PromptView {
  element: HTMLElement;
  setEnabled(enabled: boolean): void;
  setDepth(depth: number): void;
  setPlaceholder(text: string): void;
  setValue(value: string): void;
  submit(value: string): void;
  focus(): void;
}

export function createPromptView(onSubmit: (input: string) => void): PromptView {
  const history: string[] = [];
  let historyIndex = -1;
  let draftInput = "";

  const submitValue = (value: string) => {
    const trimmedValue = value.trim();
    if (trimmedValue.length === 0) {
      return;
    }

    if (history.at(-1) !== trimmedValue) {
      history.push(trimmedValue);
      if (history.length > 100) {
        history.shift();
      }
    }

    historyIndex = -1;
    draftInput = "";
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
  input.addEventListener("keydown", (event) => {
    if (event.key === "ArrowUp") {
      if (history.length === 0) {
        return;
      }

      event.preventDefault();
      if (historyIndex === -1) {
        draftInput = input.value;
        historyIndex = history.length - 1;
      } else if (historyIndex > 0) {
        historyIndex -= 1;
      }

      input.value = history[historyIndex] ?? "";
      input.setSelectionRange(input.value.length, input.value.length);
      return;
    }

    if (event.key === "ArrowDown" && historyIndex !== -1) {
      event.preventDefault();
      if (historyIndex < history.length - 1) {
        historyIndex += 1;
        input.value = history[historyIndex] ?? "";
      } else {
        historyIndex = -1;
        input.value = draftInput;
      }

      input.setSelectionRange(input.value.length, input.value.length);
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
    setValue(value) {
      input.value = value;
      input.setSelectionRange(input.value.length, input.value.length);
    },
    submit(value) {
      submitValue(value);
    },
    focus() {
      input.focus();
    },
  };
}
