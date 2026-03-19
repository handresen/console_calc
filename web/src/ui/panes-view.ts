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

function createPane(titleText: string): { section: HTMLElement; body: HTMLElement } {
  const section = document.createElement("section");
  section.className = "pane";

  const title = document.createElement("h2");
  title.className = "pane-title";
  title.textContent = titleText;

  const body = document.createElement("div");
  body.className = "pane-body";

  section.append(title, body);
  return { section, body };
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
