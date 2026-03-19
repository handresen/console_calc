export interface TranscriptView {
  element: HTMLElement;
  appendCommand(input: string): void;
  appendMessage(text: string, kind?: string): void;
}

function appendLine(container: HTMLElement, text: string, className: string): void {
  const line = document.createElement("div");
  line.className = className;
  line.textContent = text;
  container.append(line);
  container.scrollTop = container.scrollHeight;
}

export function createTranscriptView(): TranscriptView {
  const view = document.createElement("section");
  view.className = "transcript-view";

  const lines = document.createElement("div");
  lines.className = "transcript-lines";
  view.append(lines);

  return {
    element: view,
    appendCommand(input) {
      appendLine(lines, input, "transcript-line transcript-line-command");
    },
    appendMessage(text, kind = "text") {
      appendLine(lines, text, `transcript-line transcript-line-${kind}`);
    },
  };
}
