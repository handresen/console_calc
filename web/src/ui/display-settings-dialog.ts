import {
  defaultDisplaySettings,
  saveDisplaySettings,
  type DisplaySettings,
} from "./display-settings";

export interface DisplaySettingsDialog {
  element: HTMLDialogElement;
  open(settings: DisplaySettings): void;
}

export function createDisplaySettingsDialog(
  onSave: (settings: DisplaySettings) => void,
): DisplaySettingsDialog {
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

  const syncInputs = (settings: DisplaySettings) => {
    transcriptInput.value = `${settings.transcriptDecimals}`;
    stackInput.value = `${settings.stackDecimals}`;
    previewInput.value = `${settings.listPreviewLength}`;
    clampSelect.value = `${settings.transcriptListClamp}`;
    showTimingsInput.checked = settings.showTimings;
    rememberPanesInput.checked = settings.rememberPaneState;
    plotLineInput.checked = settings.plotDefaultLine;
    plotPointsInput.checked = settings.plotDefaultPoints;
    mapLinesInput.checked = settings.mapDefaultConnectLines;
  };

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
    onSave(nextSettings);
    settingsDialog.close();
  });

  return {
    element: settingsDialog,
    open(settings) {
      syncInputs(settings);
      settingsDialog.showModal();
    },
  };
}
