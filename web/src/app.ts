import { ConsoleWasmBridge } from "./bridge/console-wasm";
import {
  defaultDisplaySettings,
  formatNumericText,
  loadDisplaySettings,
  saveDisplaySettings,
  type DisplaySettings,
} from "./ui/display-settings";
import { createPanesView } from "./ui/panes-view";
import { createPromptView } from "./ui/prompt-view";
import { renderTranscriptResult } from "./ui/transcript-renderer";
import { createTranscriptView } from "./ui/transcript-view";

const SPLIT_STORAGE_KEY = "console-calc-shell-split";
const MIN_CONSOLE_WIDTH = 420;
const MIN_SIDE_WIDTH = 320;

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

  const settingsDialog = document.createElement("dialog");
  settingsDialog.className = "settings-dialog";

  const settingsForm = document.createElement("form");
  settingsForm.method = "dialog";
  settingsForm.className = "settings-form";

  const settingsHeading = document.createElement("h2");
  settingsHeading.className = "settings-heading";
  settingsHeading.textContent = "Display settings";

  const transcriptField = document.createElement("label");
  transcriptField.className = "settings-field";
  const transcriptLabel = document.createElement("span");
  transcriptLabel.textContent = "Transcript decimals";
  const transcriptInput = document.createElement("input");
  transcriptInput.type = "number";
  transcriptInput.min = "0";
  transcriptInput.max = "12";
  transcriptInput.step = "1";
  transcriptField.append(transcriptLabel, transcriptInput);

  const stackField = document.createElement("label");
  stackField.className = "settings-field";
  const stackLabel = document.createElement("span");
  stackLabel.textContent = "Stack decimals";
  const stackInput = document.createElement("input");
  stackInput.type = "number";
  stackInput.min = "0";
  stackInput.max = "12";
  stackInput.step = "1";
  stackField.append(stackLabel, stackInput);

  const previewField = document.createElement("label");
  previewField.className = "settings-field";
  const previewLabel = document.createElement("span");
  previewLabel.textContent = "List preview length";
  const previewInput = document.createElement("input");
  previewInput.type = "number";
  previewInput.min = "1";
  previewInput.max = "8";
  previewInput.step = "1";
  previewField.append(previewLabel, previewInput);

  const clampField = document.createElement("label");
  clampField.className = "settings-field";
  const clampLabel = document.createElement("span");
  clampLabel.textContent = "Transcript list clamp";
  const clampSelect = document.createElement("select");
  for (const [value, label] of [
    ["0", "Off"],
    ["2", "2 lines"],
    ["4", "4 lines"],
  ] as const) {
    const option = document.createElement("option");
    option.value = value;
    option.textContent = label;
    clampSelect.append(option);
  }
  clampField.append(clampLabel, clampSelect);

  const showTimingsField = document.createElement("label");
  showTimingsField.className = "settings-check";
  const showTimingsInput = document.createElement("input");
  showTimingsInput.type = "checkbox";
  const showTimingsLabel = document.createElement("span");
  showTimingsLabel.textContent = "Show transcript timings";
  showTimingsField.append(showTimingsInput, showTimingsLabel);

  const rememberPanesField = document.createElement("label");
  rememberPanesField.className = "settings-check";
  const rememberPanesInput = document.createElement("input");
  rememberPanesInput.type = "checkbox";
  const rememberPanesLabel = document.createElement("span");
  rememberPanesLabel.textContent = "Remember pane expansion";
  rememberPanesField.append(rememberPanesInput, rememberPanesLabel);

  const plotDefaultsGroup = document.createElement("div");
  plotDefaultsGroup.className = "settings-group";
  const plotDefaultsTitle = document.createElement("div");
  plotDefaultsTitle.className = "settings-group-title";
  plotDefaultsTitle.textContent = "Plot defaults";
  const plotLineField = document.createElement("label");
  plotLineField.className = "settings-check";
  const plotLineInput = document.createElement("input");
  plotLineInput.type = "checkbox";
  const plotLineLabel = document.createElement("span");
  plotLineLabel.textContent = "Show line";
  plotLineField.append(plotLineInput, plotLineLabel);
  const plotPointsField = document.createElement("label");
  plotPointsField.className = "settings-check";
  const plotPointsInput = document.createElement("input");
  plotPointsInput.type = "checkbox";
  const plotPointsLabel = document.createElement("span");
  plotPointsLabel.textContent = "Show points";
  plotPointsField.append(plotPointsInput, plotPointsLabel);
  const mapLinesField = document.createElement("label");
  mapLinesField.className = "settings-check";
  const mapLinesInput = document.createElement("input");
  mapLinesInput.type = "checkbox";
  const mapLinesLabel = document.createElement("span");
  mapLinesLabel.textContent = "Connect map lines";
  mapLinesField.append(mapLinesInput, mapLinesLabel);
  plotDefaultsGroup.append(
    plotDefaultsTitle,
    plotLineField,
    plotPointsField,
    mapLinesField,
  );

  const settingsActions = document.createElement("div");
  settingsActions.className = "settings-actions";
  const cancelButton = document.createElement("button");
  cancelButton.type = "button";
  cancelButton.className = "toolbar-button toolbar-button-secondary";
  cancelButton.textContent = "Cancel";
  const saveButton = document.createElement("button");
  saveButton.type = "submit";
  saveButton.className = "toolbar-button";
  saveButton.textContent = "Save";
  settingsActions.append(cancelButton, saveButton);

  settingsForm.append(
    settingsHeading,
    transcriptField,
    stackField,
    previewField,
    clampField,
    showTimingsField,
    rememberPanesField,
    plotDefaultsGroup,
    settingsActions,
  );
  settingsDialog.append(settingsForm);

  const syncSettingsInputs = () => {
    transcriptInput.value = `${displaySettings.transcriptDecimals}`;
    stackInput.value = `${displaySettings.stackDecimals}`;
    previewInput.value = `${displaySettings.listPreviewLength}`;
    clampSelect.value = `${displaySettings.transcriptListClamp}`;
    showTimingsInput.checked = displaySettings.showTimings;
    rememberPanesInput.checked = displaySettings.rememberPaneState;
    plotLineInput.checked = displaySettings.plotDefaultLine;
    plotPointsInput.checked = displaySettings.plotDefaultPoints;
    mapLinesInput.checked = displaySettings.mapDefaultConnectLines;
  };

  const applyDisplaySettings = (settings: DisplaySettings) => {
    displaySettings = settings;
    panes.setDisplaySettings(settings);
    transcript.setDisplaySettings(settings);
    transcript.reformatMessages((text, kind) =>
      kind === "value" || kind === "listing" || kind === "text"
        ? formatNumericText(text, settings.transcriptDecimals)
        : text,
    );
    if (latestSnapshot !== null) {
      panes.render(latestSnapshot);
    }
  };

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
    syncSettingsInputs();
    settingsDialog.showModal();
  });

  cancelButton.addEventListener("click", () => {
    settingsDialog.close();
  });

  settingsForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const transcriptDecimals = Number.parseInt(transcriptInput.value, 10);
    const stackDecimals = Number.parseInt(stackInput.value, 10);
    const listPreviewLength = Number.parseInt(previewInput.value, 10);
    const transcriptListClamp = Number.parseInt(clampSelect.value, 10) as 0 | 2 | 4;
    const nextSettings = saveDisplaySettings({
      transcriptDecimals: Number.isNaN(transcriptDecimals)
        ? defaultDisplaySettings.transcriptDecimals
        : transcriptDecimals,
      stackDecimals: Number.isNaN(stackDecimals)
        ? defaultDisplaySettings.stackDecimals
        : stackDecimals,
      listPreviewLength: Number.isNaN(listPreviewLength)
        ? defaultDisplaySettings.listPreviewLength
        : listPreviewLength,
      transcriptListClamp,
      showTimings: showTimingsInput.checked,
      plotDefaultLine: plotLineInput.checked,
      plotDefaultPoints: plotPointsInput.checked,
      mapDefaultConnectLines: mapLinesInput.checked,
      rememberPaneState: rememberPanesInput.checked,
    });
    applyDisplaySettings(nextSettings);
    settingsDialog.close();
  });

  const applySplit = (consoleWidth: number) => {
    const shellWidth = shell.getBoundingClientRect().width;
    const maxConsoleWidth = Math.max(
      MIN_CONSOLE_WIDTH,
      shellWidth - MIN_SIDE_WIDTH - 12,
    );
    const clampedConsoleWidth = Math.max(
      MIN_CONSOLE_WIDTH,
      Math.min(consoleWidth, maxConsoleWidth),
    );
    shell.style.gridTemplateColumns = `${clampedConsoleWidth}px 12px minmax(${MIN_SIDE_WIDTH}px, 1fr)`;
    localStorage.setItem(SPLIT_STORAGE_KEY, `${Math.round(clampedConsoleWidth)}`);
  };

  const resetResponsiveLayout = () => {
    shell.style.removeProperty("grid-template-columns");
  };

  const syncSplitForViewport = () => {
    if (window.innerWidth <= 900) {
      resetResponsiveLayout();
      return;
    }

    const storedWidth = localStorage.getItem(SPLIT_STORAGE_KEY);
    if (storedWidth !== null) {
      const parsedWidth = Number.parseFloat(storedWidth);
      if (Number.isFinite(parsedWidth)) {
        applySplit(parsedWidth);
        return;
      }
    }

    const shellWidth = shell.getBoundingClientRect().width;
    applySplit(shellWidth * 0.59);
  };

  resizeHandle.addEventListener("pointerdown", (event) => {
    if (window.innerWidth <= 900) {
      return;
    }

    event.preventDefault();
    resizeHandle.setPointerCapture(event.pointerId);
    document.body.classList.add("is-resizing-shell");

    const onPointerMove = (moveEvent: PointerEvent) => {
      const shellBounds = shell.getBoundingClientRect();
      applySplit(moveEvent.clientX - shellBounds.left);
    };

    const stopDragging = () => {
      document.body.classList.remove("is-resizing-shell");
      resizeHandle.removeEventListener("pointermove", onPointerMove);
      resizeHandle.removeEventListener("pointerup", stopDragging);
      resizeHandle.removeEventListener("pointercancel", stopDragging);
    };

    resizeHandle.addEventListener("pointermove", onPointerMove);
    resizeHandle.addEventListener("pointerup", stopDragging);
    resizeHandle.addEventListener("pointercancel", stopDragging);
  });

  resizeHandle.addEventListener("dblclick", () => {
    localStorage.removeItem(SPLIT_STORAGE_KEY);
    syncSplitForViewport();
  });

  consoleColumn.append(bridgeBanner, transcript.element, prompt.element);
  sideColumn.append(panes.element);

  shell.append(consoleColumn, resizeHandle, sideColumn);
  root.append(shell, settingsDialog);

  syncSplitForViewport();
  window.addEventListener("resize", syncSplitForViewport);

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
