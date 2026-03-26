import type {
  BindingConstantEntry,
  BindingDefinitionEntry,
  BindingFunctionEntry,
  BindingSnapshot,
  BindingStackEntry,
} from "../bridge/console-wasm";
import type { DisplaySettings } from "./display-settings";
import { defaultDisplaySettings } from "./display-settings";
import { createPane } from "./pane-controls";
import { createMapPaneView } from "./map-pane-view";
import {
  definitionDisplay,
  renderConstantList,
  renderFunctionTable,
  renderTextList,
  sampleExpressions,
} from "./pane-renderers";
import { createPlotPaneView } from "./plot-pane-view";
import { createStackPaneView } from "./stack-pane-view";

export interface PanesView {
  element: HTMLElement;
  render(snapshot: BindingSnapshot): void;
  setDisplaySettings(settings: DisplaySettings): void;
}

export function createPanesView(
  onSampleSelected?: (expression: string) => void,
  onClearStack?: () => void,
  initialDisplaySettings: DisplaySettings = defaultDisplaySettings,
): PanesView {
  const section = document.createElement("section");
  section.className = "panes-view";

  const infoPanel = document.createElement("section");
  infoPanel.className = "info-panel";

  const plotPanel = document.createElement("section");
  plotPanel.className = "plot-panel";

  const mapPanel = document.createElement("section");
  mapPanel.className = "map-panel";
  let displaySettings = initialDisplaySettings;
  let latestSnapshot: BindingSnapshot | null = null;
  const paneStateStorageKey = "console-calc-pane-state";

  const status = document.createElement("div");
  status.className = "pane-status";

  const handlePaneToggle = (expanded: boolean) => {
    if (displaySettings.rememberPaneState) {
      persistPaneState();
    }
  };

  const stackPaneView = createStackPaneView(
    handlePaneToggle,
    onClearStack,
    displaySettings,
  );
  const stackPane = stackPaneView.pane;
  const definitionsPane = createPane("Definitions", handlePaneToggle);
  const constantsPane = createPane("Constants", handlePaneToggle);
  const functionsPane = createPane("Functions", handlePaneToggle);
  const samplesPane = createPane("Samples", handlePaneToggle);
  const plotPaneView = createPlotPaneView(handlePaneToggle, displaySettings);
  const mapPaneView = createMapPaneView(handlePaneToggle, displaySettings);
  const panes = [
    { key: "stack", pane: stackPane },
    { key: "definitions", pane: definitionsPane },
    { key: "constants", pane: constantsPane },
    { key: "functions", pane: functionsPane },
    { key: "samples", pane: samplesPane },
    { key: "plot", pane: plotPaneView.pane },
    { key: "map", pane: mapPaneView.pane },
  ];

  function persistPaneState() {
    const state = Object.fromEntries(panes.map(({ key, pane }) => [key, pane.isExpanded()]));
    localStorage.setItem(paneStateStorageKey, JSON.stringify(state));
  }

  function restorePaneState() {
    if (!displaySettings.rememberPaneState) {
      panes.forEach(({ pane }) => pane.setExpanded(false));
      return;
    }

    const rawState = localStorage.getItem(paneStateStorageKey);
    if (rawState === null) {
      return;
    }

    try {
      const parsed = JSON.parse(rawState) as Record<string, boolean>;
      for (const { key, pane } of panes) {
        pane.setExpanded(parsed[key] === true);
      }
    } catch {
      panes.forEach(({ pane }) => pane.setExpanded(false));
    }
  }

  functionsPane.body.classList.add("functions-pane-body");

  const functionTableContainer = document.createElement("div");
  functionTableContainer.className = "function-table-container";

  const samplesList = document.createElement("div");
  samplesList.className = "sample-list";

  for (const expression of sampleExpressions) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "sample-button";
    button.textContent = expression;
    button.addEventListener("click", () => {
      onSampleSelected?.(expression);
    });
    samplesList.append(button);
  }

  functionsPane.body.append(functionTableContainer);
  samplesPane.body.append(samplesList);

  infoPanel.append(
    status,
    stackPane.section,
    definitionsPane.section,
    constantsPane.section,
    functionsPane.section,
    samplesPane.section,
  );

  plotPanel.append(
    plotPaneView.pane.section,
  );

  mapPanel.append(mapPaneView.pane.section);

  section.append(infoPanel, plotPanel, mapPanel);
  restorePaneState();

  return {
    element: section,
    render(snapshot) {
      latestSnapshot = snapshot;
      status.textContent = `Mode ${snapshot.display_mode} | stack ${snapshot.stack.length}/${snapshot.max_stack_depth}`;
      stackPaneView.render(snapshot);
      definitionsPane.count.textContent = `${snapshot.definitions.length}`;
      constantsPane.count.textContent = `${snapshot.constants.length}`;
      functionsPane.count.textContent = `${snapshot.functions.length}`;
      samplesPane.count.textContent = `${sampleExpressions.length}`;
      renderTextList(
        definitionsPane.body,
        snapshot.definitions.map((entry) => definitionDisplay(entry)),
      );
      renderConstantList(constantsPane.body, snapshot.constants);
      renderFunctionTable(functionTableContainer, snapshot.functions);
      plotPaneView.render(snapshot.stack);
      mapPaneView.render(snapshot.stack);
    },
    setDisplaySettings(settings) {
      displaySettings = settings;
      stackPaneView.setDisplaySettings(displaySettings);
      if (displaySettings.rememberPaneState) {
        persistPaneState();
      }
      restorePaneState();
      if (latestSnapshot !== null) {
        stackPaneView.render(latestSnapshot);
        plotPaneView.setDisplaySettings(displaySettings);
        mapPaneView.setDisplaySettings(displaySettings);
      }
    },
  };
}
