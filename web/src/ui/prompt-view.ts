export function createPromptView(): HTMLElement {
  const wrapper = document.createElement("section");
  wrapper.className = "prompt-view";

  const input = document.createElement("input");
  input.type = "text";
  input.placeholder = "0> enter expression or command";
  input.autocomplete = "off";
  input.spellcheck = false;

  wrapper.append(input);
  return wrapper;
}
