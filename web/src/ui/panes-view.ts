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
  renderSampleList,
  renderTextList,
  sampleGroups,
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
  const paneOverlayHost = document.createElement("div");
  paneOverlayHost.className = "pane-overlay-host";

  const infoPanel = document.createElement("section");
  infoPanel.className = "info-panel";

  const plotPanel = document.createElement("section");
  plotPanel.className = "plot-panel";

  const mapPanel = document.createElement("section");
  mapPanel.className = "map-panel";
  let displaySettings = initialDisplaySettings;
  let latestSnapshot: BindingSnapshot | null = null;
  const paneStateStorageKey = "console-calc-pane-state";
  let expandedPaneState: {
    key: string;
    panel: HTMLElement;
    pane: { setExpanded(expanded: boolean): void; isExpanded(): boolean };
    previousPaneState: boolean;
    anchor: Comment;
    refreshLayout: () => void;
    button: HTMLButtonElement;
    buttonCollapsedIcon: string;
    buttonCollapsedLabel: string;
  } | null = null;

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

  const setExpandedButtonState = (
    button: HTMLButtonElement,
    expanded: boolean,
    collapsedIcon: string,
    collapsedLabel: string,
  ) => {
    button.textContent = expanded ? "⤡" : collapsedIcon;
    button.setAttribute("aria-label", expanded ? "Collapse" : collapsedLabel);
    button.title = expanded ? "Collapse" : collapsedLabel;
  };

  const setOverlayExpanded = (
    key: string,
    panel: HTMLElement,
    pane: { setExpanded(expanded: boolean): void; isExpanded(): boolean },
    refreshLayout: () => void,
    button: HTMLButtonElement,
    buttonCollapsedIcon: string,
    buttonCollapsedLabel: string,
    expanded: boolean,
  ) => {
    const isCurrent = expandedPaneState?.key === key;
    if ((expanded && isCurrent) || (!expanded && !isCurrent)) {
      return;
    }

    if (!expanded && expandedPaneState !== null) {
      const current = expandedPaneState;
      current.panel.classList.remove("pane-panel-expanded");
      if (paneOverlayHost.contains(current.panel)) {
        current.anchor.replaceWith(current.panel);
      }
      current.pane.setExpanded(current.previousPaneState);
      setExpandedButtonState(
        current.button,
        false,
        current.buttonCollapsedIcon,
        current.buttonCollapsedLabel,
      );
      current.refreshLayout();
      expandedPaneState = null;
    }

    if (!expanded) {
      return;
    }

    const appShell =
      section.closest(".app-shell") ?? document.querySelector<HTMLElement>(".app-shell");
    if (appShell !== null && !paneOverlayHost.isConnected) {
      appShell.append(paneOverlayHost);
    }
    const anchor = document.createComment(`${key}-overlay-anchor`);
    panel.before(anchor);
    paneOverlayHost.append(panel);
    panel.classList.add("pane-panel-expanded");
    expandedPaneState = {
      key,
      panel,
      pane,
      previousPaneState: pane.isExpanded(),
      anchor,
      refreshLayout,
      button,
      buttonCollapsedIcon,
      buttonCollapsedLabel,
    };
    pane.setExpanded(true);
    setExpandedButtonState(button, true, buttonCollapsedIcon, buttonCollapsedLabel);
    refreshLayout();
  };

  const addExpandButton = (
    key: string,
    panel: HTMLElement,
    pane: { setExpanded(expanded: boolean): void; isExpanded(): boolean; actions: HTMLElement },
    refreshLayout: () => void,
    collapsedLabel: string,
    collapsedIcon = "⤢",
  ) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "pane-icon-button";
    setExpandedButtonState(button, false, collapsedIcon, collapsedLabel);
    button.addEventListener("click", () => {
      setOverlayExpanded(
        key,
        panel,
        pane,
        refreshLayout,
        button,
        collapsedIcon,
        collapsedLabel,
        expandedPaneState?.key !== key,
      );
    });
    pane.actions.append(button);
  };

  addExpandButton(
    "plot",
    plotPanel,
    plotPaneView.pane,
    () => plotPaneView.refreshLayout(),
    "Expand plot",
  );
  addExpandButton(
    "map",
    mapPanel,
    mapPaneView.pane,
    () => mapPaneView.refreshLayout(),
    "Expand map",
  );

  section.append(infoPanel, plotPanel, mapPanel);
  restorePaneState();

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && expandedPaneState !== null) {
      event.preventDefault();
      setOverlayExpanded(
        expandedPaneState.key,
        expandedPaneState.panel,
        expandedPaneState.pane,
        expandedPaneState.refreshLayout,
        expandedPaneState.button,
        expandedPaneState.buttonCollapsedIcon,
        expandedPaneState.buttonCollapsedLabel,
        false,
      );
    }
  });

  return {
    element: section,
    render(snapshot) {
      latestSnapshot = snapshot;
      status.textContent = `Mode ${snapshot.display_mode} | stack ${snapshot.stack.length}/${snapshot.max_stack_depth}`;
      stackPaneView.render(snapshot);
      definitionsPane.count.textContent = `${snapshot.definitions.length}`;
      constantsPane.count.textContent = `${snapshot.constants.length}`;
      functionsPane.count.textContent = `${snapshot.functions.length}`;
      samplesPane.count.textContent = `${sampleGroups.reduce((total, group) => total + group.entries.length, 0)}`;
      renderTextList(
        definitionsPane.body,
        snapshot.definitions.map((entry) => definitionDisplay(entry)),
      );
      renderConstantList(constantsPane.body, snapshot.constants);
      renderFunctionTable(functionTableContainer, snapshot.functions);
      renderSampleList(samplesList, sampleGroups, onSampleSelected);
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
