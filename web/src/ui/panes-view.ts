import type {
  BindingConstantEntry,
  BindingDefinitionEntry,
  BindingFunctionEntry,
  BindingSnapshot,
  BindingStackEntry,
} from "../bridge/console-wasm";

export interface PanesView {
  element: HTMLElement;
  render(snapshot: BindingSnapshot): void;
}

function renderTextList(container: HTMLElement, values: string[]): void {
  container.replaceChildren();
  if (values.length === 0) {
    const empty = document.createElement("div");
    empty.className = "pane-empty";
    empty.textContent = "Empty";
    container.append(empty);
    return;
  }

  for (const value of values) {
    const line = document.createElement("div");
    line.className = "pane-line";
    line.textContent = value;
    container.append(line);
  }
}

function stackDisplay(entry: BindingStackEntry): string {
  return `${entry.level}:${entry.display}`;
}

function definitionDisplay(entry: BindingDefinitionEntry): string {
  return `${entry.name}:${entry.expression}`;
}

function constantDisplay(entry: BindingConstantEntry): string {
  return `${entry.name}:${entry.value}`;
}

function functionDisplay(entry: BindingFunctionEntry): string {
  return `${entry.name}/${entry.arity_label} - ${entry.summary}`;
}

interface PaneElements {
  section: HTMLElement;
  body: HTMLElement;
  count: HTMLElement;
}

function createPane(titleText: string): PaneElements {
  const section = document.createElement("section");
  section.className = "pane";

  const title = document.createElement("button");
  title.className = "pane-title";
  title.type = "button";
  title.setAttribute("aria-expanded", "false");

  const titleLabel = document.createElement("span");
  titleLabel.className = "pane-title-label";
  titleLabel.textContent = titleText;

  const count = document.createElement("span");
  count.className = "pane-count";
  count.textContent = "0";

  const marker = document.createElement("span");
  marker.className = "pane-marker";
  marker.textContent = "+";

  const body = document.createElement("div");
  body.className = "pane-body";
  body.hidden = true;

  title.addEventListener("click", () => {
    const expanded = title.getAttribute("aria-expanded") !== "false";
    title.setAttribute("aria-expanded", expanded ? "false" : "true");
    body.hidden = expanded;
    marker.textContent = expanded ? "+" : "−";
  });

  title.append(titleLabel, count, marker);
  section.append(title, body);
  return { section, body, count };
}

export function createPanesView(): PanesView {
  const section = document.createElement("section");
  section.className = "panes-view";

  const status = document.createElement("div");
  status.className = "pane-status";

  const stackPane = createPane("Stack");
  const definitionsPane = createPane("Definitions");
  const constantsPane = createPane("Constants");
  const functionsPane = createPane("Functions");

  section.append(
    status,
    stackPane.section,
    definitionsPane.section,
    constantsPane.section,
    functionsPane.section,
  );

  return {
    element: section,
    render(snapshot) {
      status.textContent = `Mode ${snapshot.display_mode} | stack ${snapshot.stack.length}/${snapshot.max_stack_depth}`;
      stackPane.count.textContent = `${snapshot.stack.length}`;
      definitionsPane.count.textContent = `${snapshot.definitions.length}`;
      constantsPane.count.textContent = `${snapshot.constants.length}`;
      functionsPane.count.textContent = `${snapshot.functions.length}`;
      renderTextList(
        stackPane.body,
        snapshot.stack.map((entry) => stackDisplay(entry)),
      );
      renderTextList(
        definitionsPane.body,
        snapshot.definitions.map((entry) => definitionDisplay(entry)),
      );
      renderTextList(
        constantsPane.body,
        snapshot.constants.map((entry) => constantDisplay(entry)),
      );
      renderTextList(
        functionsPane.body,
        snapshot.functions.map((entry) => functionDisplay(entry)),
      );
    },
  };
}
