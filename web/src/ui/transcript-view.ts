export interface TranscriptView {
  element: HTMLElement;
  appendCommand(input: string): void;
  appendMessage(text: string, kind?: string, metaText?: string, extraClassName?: string): void;
  clear(): void;
  reformatMessages(formatter: (text: string, kind?: string) => string): void;
  scrollToBottom(): void;
}

function appendLine(
  container: HTMLElement,
  text: string,
  className: string,
  metaText?: string,
  extraClassName?: string,
): void {
  const line = document.createElement("div");
  line.className = extraClassName === undefined ? className : `${className} ${extraClassName}`;

  const content = document.createElement("span");
  content.className = "transcript-line-content";
  content.textContent = text;
  content.dataset.rawText = text;
  line.append(content);
  line.dataset.kind = className
    .split(" ")
    .find((token) => token.startsWith("transcript-line-") && token !== "transcript-line")
    ?.replace("transcript-line-", "");

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
    appendMessage(text, kind = "text", metaText, extraClassName) {
      appendLine(
        lines,
        text,
        `transcript-line transcript-line-${kind}`,
        metaText,
        extraClassName,
      );
    },
    clear() {
      lines.replaceChildren();
    },
    reformatMessages(formatter) {
      const contentNodes = lines.querySelectorAll<HTMLElement>(".transcript-line-content");
      for (const contentNode of contentNodes) {
        const rawText = contentNode.dataset.rawText;
        if (rawText === undefined) {
          continue;
        }

        const line = contentNode.parentElement;
        const kind = line?.dataset.kind;
        contentNode.textContent = formatter(rawText, kind);
      }
    },
    scrollToBottom() {
      view.scrollTop = view.scrollHeight;
    },
  };
}
