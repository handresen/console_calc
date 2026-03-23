import { ConsoleWasmBridge } from "./bridge/console-wasm";
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
  bridgeBanner.textContent = "Loading WebAssembly bridge...";

  const transcript = createTranscriptView();
  const bridge = new ConsoleWasmBridge();

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
    prompt.setDepth(result.snapshot.stack.length);
    renderTranscriptResult(transcript, result, input, elapsedMs);
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
  });
  prompt.setEnabled(false);

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
  root.append(shell);

  syncSplitForViewport();
  window.addEventListener("resize", syncSplitForViewport);

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
