export interface TranscriptView {
  element: HTMLElement;
  appendCommand(input: string): void;
  appendMessage(text: string, kind?: string, metaText?: string): void;
  clear(): void;
  scrollToBottom(): void;
}

function appendLine(
  container: HTMLElement,
  text: string,
  className: string,
  metaText?: string,
): void {
  const line = document.createElement("div");
  line.className = className;

  const content = document.createElement("span");
  content.className = "transcript-line-content";
  content.textContent = text;
  line.append(content);

  if (metaText !== undefined) {
    const meta = document.createElement("span");
    meta.className = "transcript-line-meta";
    meta.textContent = metaText;
    line.append(meta);
  }

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
    appendMessage(text, kind = "text", metaText) {
      appendLine(lines, text, `transcript-line transcript-line-${kind}`, metaText);
    },
    clear() {
      lines.replaceChildren();
    },
    scrollToBottom() {
      view.scrollTop = view.scrollHeight;
    },
  };
}
