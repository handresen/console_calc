import { ConsoleWasmBridge, type BindingCommandResult } from "./bridge/console-wasm";
import { createPanesView } from "./ui/panes-view";
import { createPromptView } from "./ui/prompt-view";
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
  const panes = createPanesView();
  const bridge = new ConsoleWasmBridge();

  const renderResult = (result: BindingCommandResult, input?: string): void => {
    for (const event of result.events) {
      switch (event.kind) {
        case "value":
          transcript.appendMessage(
            input === undefined ? event.text : `${input} = ${event.text}`,
            "value",
          );
          break;
        case "error":
          transcript.appendMessage(`error: ${event.text}`, "error");
          break;
        case "text":
          transcript.appendMessage(event.text, "text");
          break;
        case "stack_listing":
          for (const entry of event.stack) {
            transcript.appendMessage(`${entry.level}:${entry.display}`, "listing");
          }
          break;
        case "definition_listing":
          for (const entry of event.definitions) {
            transcript.appendMessage(`${entry.name}:${entry.expression}`, "listing");
          }
          break;
        case "constant_listing":
          for (const entry of event.constants) {
            transcript.appendMessage(`${entry.name}:${entry.value}`, "listing");
          }
          break;
        case "function_listing":
          for (const entry of event.functions) {
            transcript.appendMessage(
              `${entry.name}/${entry.arity_label} - ${entry.summary}`,
              "listing",
            );
          }
          break;
        default:
          transcript.appendMessage(event.text, "text");
          break;
      }
    }

    panes.render(result.snapshot);
  };

  const prompt = createPromptView(async (input) => {
    try {
      const result = await bridge.submit(input);
      renderResult(result, input);
    } catch (error) {
      const message =
        error instanceof Error ? error.message : "Unknown wasm bridge failure";
      transcript.appendMessage(message, "error");
    }
  });
  prompt.setEnabled(false);

  consoleColumn.append(bridgeBanner, transcript.element, prompt.element);
  sideColumn.append(panes.element);

  shell.append(consoleColumn, sideColumn);
  root.append(shell);

  void (async () => {
    try {
      const result = await bridge.initialize();
      bridgeBanner.textContent = "WebAssembly bridge ready.";
      prompt.setEnabled(true);
      prompt.setPlaceholder("enter expression or command");
      renderResult(result);
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
