export interface PromptView {
  element: HTMLElement;
  setEnabled(enabled: boolean): void;
  setPlaceholder(text: string): void;
  focus(): void;
}

export function createPromptView(onSubmit: (input: string) => void): PromptView {
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
    if (event.key !== "Enter") {
      return;
    }

    const value = input.value.trim();
    if (value.length === 0) {
      return;
    }

    input.value = "";
    onSubmit(value);
  });

  wrapper.append(promptLabel, input);
  return {
    element: wrapper,
    setEnabled(enabled) {
      input.disabled = !enabled;
    },
    setPlaceholder(text) {
      input.placeholder = text;
    },
    focus() {
      input.focus();
    },
  };
}
