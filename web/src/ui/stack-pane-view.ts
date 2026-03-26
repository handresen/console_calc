import type { BindingSnapshot } from "../bridge/console-wasm";
import type { DisplaySettings } from "./display-settings";
import type { PaneElements } from "./pane-controls";
import { createPane } from "./pane-controls";
import { renderStackList } from "./stack-pane-renderers";

export interface StackPaneView {
  pane: PaneElements;
  render(snapshot: BindingSnapshot): void;
  setDisplaySettings(settings: DisplaySettings): void;
}

export function createStackPaneView(
  onToggle: (expanded: boolean) => void,
  onClearStack: (() => void) | undefined,
  initialDisplaySettings: DisplaySettings,
): StackPaneView {
  const pane = createPane("Stack", onToggle);
  const titleLabel = pane.title.querySelector<HTMLElement>(".pane-title-label");
  const list = document.createElement("div");
  list.className = "stack-list";

  const clearButton = document.createElement("button");
  clearButton.type = "button";
  clearButton.className = "pane-icon-button";
  clearButton.textContent = "×";
  clearButton.setAttribute("aria-label", "Clear stack");
  clearButton.title = "Clear stack";
  clearButton.addEventListener("click", () => {
    onClearStack?.();
  });

  pane.actions.append(clearButton);
  pane.body.append(list);

  let displaySettings = initialDisplaySettings;
  updateTitle();

  function updateTitle(): void {
    if (titleLabel !== null) {
      titleLabel.textContent = `Stack d${displaySettings.stackDecimals}`;
    }
  }

  return {
    pane,
    render(snapshot) {
      updateTitle();
      pane.count.textContent = `${snapshot.stack.length}`;
      clearButton.disabled = snapshot.stack.length === 0;
      renderStackList(list, snapshot.stack, displaySettings);
      list.scrollTop = list.scrollHeight;
    },
    setDisplaySettings(settings) {
      displaySettings = settings;
      updateTitle();
    },
  };
}
