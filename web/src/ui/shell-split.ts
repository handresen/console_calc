const SPLIT_STORAGE_KEY = "console-calc-shell-split";
const MIN_CONSOLE_WIDTH = 420;
const MIN_SIDE_WIDTH = 320;

export function attachShellSplit(
  shell: HTMLElement,
  resizeHandle: HTMLElement,
): void {
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

  syncSplitForViewport();
  window.addEventListener("resize", syncSplitForViewport);
}
