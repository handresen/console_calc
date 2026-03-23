import type { BindingCommandResult } from "../bridge/console-wasm";
import { formatNumericText } from "./display-settings";
import type { TranscriptView } from "./transcript-view";

function expectedFunctionSignature(
  result: BindingCommandResult,
  input?: string,
): string | null {
  for (const event of result.events) {
    if (event.kind === "error" && event.error?.expected_signature) {
      return event.error.expected_signature;
    }
  }

  if (input === undefined) {
    return null;
  }

  const match = input.match(/^\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(/);
  if (match === null) {
    return null;
  }

  const functionName = match[1];
  if (functionName === undefined) {
    return null;
  }

  const functionEntry = result.snapshot.functions.find((entry) => entry.name === functionName);
  return functionEntry?.signature ?? null;
}

export function renderTranscriptResult(
  transcript: TranscriptView,
  result: BindingCommandResult,
  input?: string,
  elapsedMs?: number,
  transcriptDecimals = 8,
): void {
  let renderedEvent = false;
  let timingRendered = false;
  const timingText = elapsedMs === undefined ? undefined : `${elapsedMs.toFixed(1)} ms`;
  const isListValueText = (text: string) => text.trimStart().startsWith("{");

  const appendMessage = (text: string, kind: string, extraClassName?: string): void => {
    transcript.appendMessage(
      formatNumericText(text, transcriptDecimals),
      kind,
      timingRendered ? undefined : timingText,
      extraClassName,
    );
    timingRendered = true;
  };

  for (const event of result.events) {
    renderedEvent = true;
    switch (event.kind) {
      case "value":
        appendMessage(
          input === undefined ? event.text : `${input} = ${event.text}`,
          "value",
          isListValueText(event.text) ? "transcript-line-value-list" : undefined,
        );
        break;
      case "error": {
        appendMessage(`error: ${event.text}`, "error");
        const signature = expectedFunctionSignature(result, input);
        if (signature !== null && !event.text.includes(signature)) {
          appendMessage(`expected: ${signature}`, "listing");
        }
        break;
      }
      case "text":
        appendMessage(event.text, "text");
        break;
      case "stack_listing":
        for (const entry of event.stack) {
          appendMessage(`${entry.level}:${entry.display}`, "listing");
        }
        break;
      case "definition_listing":
        for (const entry of event.definitions) {
          appendMessage(`${entry.name}:${entry.expression}`, "listing");
        }
        break;
      case "constant_listing":
        for (const entry of event.constants) {
          appendMessage(`${entry.name}:${entry.value}`, "listing");
        }
        break;
      case "function_listing":
        for (const entry of event.functions) {
          appendMessage(`${entry.signature} - ${entry.summary}`, "listing");
        }
        break;
      default:
        appendMessage(event.text, "text");
        break;
    }
  }

  if (!renderedEvent && input !== undefined && input.includes(":")) {
    appendMessage(input, "text");
  }
}
