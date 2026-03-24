export interface PaneElements {
  section: HTMLElement;
  header: HTMLElement;
  title: HTMLButtonElement;
  body: HTMLElement;
  count: HTMLElement;
  actions: HTMLElement;
  setExpanded(expanded: boolean): void;
  isExpanded(): boolean;
}

export function createPane(
  titleText: string,
  onToggle?: (expanded: boolean) => void,
): PaneElements {
  const section = document.createElement("section");
  section.className = "pane";

  const header = document.createElement("div");
  header.className = "pane-header";

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

  const actions = document.createElement("div");
  actions.className = "pane-header-actions";

  const body = document.createElement("div");
  body.className = "pane-body";
  body.hidden = true;

  const setExpanded = (expanded: boolean) => {
    title.setAttribute("aria-expanded", expanded ? "true" : "false");
    body.hidden = !expanded;
    marker.textContent = expanded ? "−" : "+";
    onToggle?.(expanded);
  };

  title.addEventListener("click", () => {
    setExpanded(title.getAttribute("aria-expanded") === "false");
  });

  title.append(titleLabel, marker);
  header.append(title, actions, count);
  section.append(header, body);
  return {
    section,
    header,
    title,
    body,
    count,
    actions,
    setExpanded,
    isExpanded: () => title.getAttribute("aria-expanded") !== "false",
  };
}
