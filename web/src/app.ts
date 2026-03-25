import { ConsoleWasmBridge } from "./bridge/console-wasm";
import {
  loadDisplaySettings,
  type DisplaySettings,
} from "./ui/display-settings";
import { createDisplaySettingsDialog } from "./ui/display-settings-dialog";
import { createPanesView } from "./ui/panes-view";
import { createPromptView } from "./ui/prompt-view";
import { attachShellSplit } from "./ui/shell-split";
import { formatTranscriptText, renderTranscriptResult } from "./ui/transcript-renderer";
import { createTranscriptView } from "./ui/transcript-view";

export function createApp(root: HTMLElement): void {
  root.replaceChildren();

  const shell = document.createElement("main");
  shell.className = "app-shell";

  const consoleColumn = document.createElement("section");
  consoleColumn.className = "console-column";

  const sideColumn = document.createElement("aside");
  sideColumn.className = "side-column";

  const resizeHandle = document.createElement("div");
  resizeHandle.className = "shell-resize-handle";
  resizeHandle.setAttribute("role", "separator");
  resizeHandle.setAttribute("aria-orientation", "vertical");
  resizeHandle.setAttribute("aria-label", "Resize panels");

  const bridgeBanner = document.createElement("div");
  bridgeBanner.className = "bridge-banner";
  const bridgeBannerText = document.createElement("span");
  bridgeBannerText.className = "bridge-banner-text";
  bridgeBannerText.textContent = "Loading WebAssembly bridge...";

  const settingsButton = document.createElement("button");
  settingsButton.type = "button";
  settingsButton.className = "toolbar-button toolbar-icon-button";
  settingsButton.innerHTML = "&#9881;";
  settingsButton.setAttribute("aria-label", "Open settings");
  settingsButton.title = "Settings";

  bridgeBanner.append(bridgeBannerText, settingsButton);

  const transcript = createTranscriptView();
  const bridge = new ConsoleWasmBridge();
  let displaySettings: DisplaySettings = loadDisplaySettings();
  let latestSnapshot: Awaited<ReturnType<ConsoleWasmBridge["submit"]>>["snapshot"] | null = null;

  const applyDisplaySettings = (settings: DisplaySettings) => {
    displaySettings = settings;
    panes.setDisplaySettings(settings);
    transcript.setDisplaySettings(settings);
    transcript.reformatMessages((text, kind) =>
      kind === "value" || kind === "listing" || kind === "text"
        ? formatTranscriptText(text, settings.transcriptDecimals)
        : text,
    );
    if (latestSnapshot !== null) {
      panes.render(latestSnapshot);
      }
  };

  const settingsDialog = createDisplaySettingsDialog(applyDisplaySettings);

  const executeInput = async (input: string): Promise<void> => {
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
  };

  const renderResult = (
    result: Awaited<ReturnType<ConsoleWasmBridge["submit"]>>,
    input?: string,
    elapsedMs?: number,
  ): void => {
    latestSnapshot = result.snapshot;
    prompt.setDepth(result.snapshot.stack.length);
    renderTranscriptResult(
      transcript,
      result,
      input,
      elapsedMs,
      displaySettings.transcriptDecimals,
    );
    panes.render(result.snapshot);
    transcript.scrollToBottom();
  };

  const prompt = createPromptView(async (input) => {
    await executeInput(input);
  });
  const panes = createPanesView((expression) => {
    prompt.submit(expression);
    prompt.focus();
  }, () => {
    void executeInput("clear");
    prompt.focus();
  }, displaySettings);
  prompt.setEnabled(false);
  transcript.setDisplaySettings(displaySettings);

  settingsButton.addEventListener("click", () => {
    settingsDialog.open(displaySettings);
  });

  consoleColumn.append(bridgeBanner, transcript.element, prompt.element);
  sideColumn.append(panes.element);

  shell.append(consoleColumn, resizeHandle, sideColumn);
  root.append(shell, settingsDialog.element);
  attachShellSplit(shell, resizeHandle);

  void (async () => {
    try {
      const startedAt = performance.now();
      const result = await bridge.initialize();
      bridgeBannerText.textContent = "WebAssembly bridge ready.";
      prompt.setEnabled(true);
      prompt.setPlaceholder("enter expression or command");
      renderResult(result, undefined, performance.now() - startedAt);
      prompt.focus();
    } catch (error) {
      const message =
        error instanceof Error ? error.message : "Unknown wasm bridge failure";
      bridgeBannerText.textContent = `WebAssembly bridge failed: ${message}`;
      transcript.appendMessage(message, "error");
    }
  })();

  globalThis.addEventListener("beforeunload", () => {
    bridge.dispose();
  });
}
