export function createTranscriptView(): HTMLElement {
  const view = document.createElement("section");
  view.className = "transcript-view";
  view.textContent = "Console transcript will render here.";
  return view;
}
