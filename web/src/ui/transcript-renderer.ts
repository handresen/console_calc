import type { BindingCommandResult } from "../bridge/console-wasm";
import type { TranscriptView } from "./transcript-view";

function expectedFunctionSignature(
  result: BindingCommandResult,
  input?: string,
): string | null {
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
): void {
  let renderedEvent = false;

  for (const event of result.events) {
    renderedEvent = true;
    switch (event.kind) {
      case "value":
        transcript.appendMessage(
          input === undefined ? event.text : `${input} = ${event.text}`,
          "value",
        );
        break;
      case "error": {
        transcript.appendMessage(`error: ${event.text}`, "error");
        const signature = expectedFunctionSignature(result, input);
        if (signature !== null && !event.text.includes(signature)) {
          transcript.appendMessage(`expected: ${signature}`, "listing");
        }
        break;
      }
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
          transcript.appendMessage(`${entry.signature} - ${entry.summary}`, "listing");
        }
        break;
      default:
        transcript.appendMessage(event.text, "text");
        break;
    }
  }

  if (!renderedEvent && input !== undefined && input.includes(":")) {
    transcript.appendMessage(input, "text");
  }
}
