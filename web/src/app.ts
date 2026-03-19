import { createBridgePlaceholder } from "./bridge/console-wasm";
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

  consoleColumn.append(
    createBridgePlaceholder(),
    createTranscriptView(),
    createPromptView(),
  );
  sideColumn.append(createPanesView());

  shell.append(consoleColumn, sideColumn);
  root.append(shell);
}
