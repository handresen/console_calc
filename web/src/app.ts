import { ConsoleWasmBridge } from "./bridge/console-wasm";
import { createPanesView } from "./ui/panes-view";
import { createPromptView } from "./ui/prompt-view";
import { renderTranscriptResult } from "./ui/transcript-renderer";
import { createTranscriptView } from "./ui/transcript-view";

export function createApp(root: HTMLElement): void {
  root.replaceChildren();

  const shell = document.createElement("main");
  shell.className = "app-shell";

  const consoleColumn = document.createElement("section");
  consoleColumn.className = "console-column";

  const sideColumn = document.createElement("aside");
  sideColumn.className = "side-column";

  const bridgeBanner = document.createElement("div");
  bridgeBanner.className = "bridge-banner";
  bridgeBanner.textContent = "Loading WebAssembly bridge...";

  const transcript = createTranscriptView();
  const bridge = new ConsoleWasmBridge();

  const renderResult = (
    result: Awaited<ReturnType<ConsoleWasmBridge["submit"]>>,
    input?: string,
    elapsedMs?: number,
  ): void => {
    prompt.setDepth(result.snapshot.stack.length);
    renderTranscriptResult(transcript, result, input, elapsedMs);
    panes.render(result.snapshot);
    transcript.scrollToBottom();
  };

  const prompt = createPromptView(async (input) => {
    if (input === "cls") {
      transcript.clear();
      return;
    }

    try {
      const startedAt = performance.now();
      const result = await bridge.submit(input);
      renderResult(result, input, performance.now() - startedAt);
    } catch (error) {
      const message =
        error instanceof Error ? error.message : "Unknown wasm bridge failure";
      transcript.appendMessage(message, "error");
    }
  });
  const panes = createPanesView((expression) => {
    prompt.setValue(expression);
    prompt.focus();
  });
  prompt.setEnabled(false);

  consoleColumn.append(bridgeBanner, transcript.element, prompt.element);
  sideColumn.append(panes.element);

  shell.append(consoleColumn, sideColumn);
  root.append(shell);

  void (async () => {
    try {
      const startedAt = performance.now();
      const result = await bridge.initialize();
      bridgeBanner.textContent = "WebAssembly bridge ready.";
      prompt.setEnabled(true);
      prompt.setPlaceholder("enter expression or command");
      renderResult(result, undefined, performance.now() - startedAt);
      prompt.focus();
    } catch (error) {
      const message =
        error instanceof Error ? error.message : "Unknown wasm bridge failure";
      bridgeBanner.textContent = `WebAssembly bridge failed: ${message}`;
      transcript.appendMessage(message, "error");
    }
  })();

  globalThis.addEventListener("beforeunload", () => {
    bridge.dispose();
  });
}
