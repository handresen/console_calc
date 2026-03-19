export function createPanesView(): HTMLElement {
  const section = document.createElement("section");
  section.className = "panes-view";

  const title = document.createElement("h2");
  title.className = "pane-title";
  title.textContent = "Helper Panes";

  const list = document.createElement("ul");
  list.className = "pane-list";

  for (const item of ["Stack", "Definitions", "Constants", "Functions"]) {
    const entry = document.createElement("li");
    entry.textContent = item;
    list.append(entry);
  }

  section.append(title, list);
  return section;
}
