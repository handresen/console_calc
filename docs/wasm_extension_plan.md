# WebAssembly Extension Plan

## Purpose

This document captures the next architectural steps required to use the current
calculator/runtime as the engine for a browser-based console application,
including a future WebAssembly target.

The goal is not to replace the terminal application. The goal is to separate
session semantics from terminal presentation so the same engine can support:

- the existing terminal console
- a browser UI with a console-like main view
- optional helper panes for stack, definitions, constants, and builtin functions
- a future WebAssembly build

## Current Strengths

The current codebase already has several properties that make this realistic:

- parsing and evaluation are separated from terminal input handling
- most calculator behavior is synchronous, local, and deterministic
- console state already exists as a distinct session-oriented concept
- builtin metadata is centralized
- currency/network behavior is isolated to the app layer

## Main Gap

The remaining problem is that the console runtime is still too stream-oriented.

`ConsoleSession` currently mixes:

- session state
- command execution
- stack and definition mutation
- prompt behavior
- terminal-oriented output
- error printing

A web frontend or WebAssembly host should consume structured state and events,
not terminal transcript strings.

## Target Architecture

The desired architecture is:

1. Core library
   - tokenizer
   - parser
   - evaluator
   - values
   - builtin metadata

2. Session engine
   - stack state
   - user definitions
   - display mode
   - command execution
   - refresh hooks such as currencies
   - structured command results

3. Terminal adapter
   - prompt rendering
   - stream output
   - ANSI colors
   - line editor

4. Web/WASM adapter
   - JS/WASM bindings
   - structured state serialization
   - browser-side rendering and input

## Refactor Plan

### Phase 1: Extract Session Semantics

Create a transport-free session engine from the current console session layer.

Status:

- in progress
- initial extraction is complete on the `users-ha-web_frontend` branch via
  `ConsoleSessionEngine`
- terminal `ConsoleSession` now acts as an adapter over that engine

Target outcomes:

- introduce a `SessionState` type
- expose stack, definitions, display mode, and related session data through an
  explicit state model rather than only ad hoc accessors
- keep command submission in a transport-free method such as:
  - `submit(std::string_view input) -> CommandResult`
- keep direct dependence on `std::istream` / `std::ostream` out of reusable
  session logic

The current `ConsoleSession` should then become a thin adapter over this engine.

### Phase 2: Replace Stream Output With Structured Results

Introduce structured command results and output events.

Status:

- started
- the engine already returns a structured `ConsoleEngineCommandResult`
- results are still somewhat transitional: values are structured, but
  informational output is still line-oriented text

Suggested shapes:

- `CommandResult`
  - optional produced value
  - optional error message
  - emitted output events
  - state dirty flags or snapshots

- `OutputEvent`
  - printed scalar/list value
  - informational text
  - error text

- `SessionSnapshot`
  - stack
  - definitions
  - display mode
  - helper metadata if needed

Terminal mode can still render these as strings. The browser can render the same
data without scraping terminal output.

### Phase 3: Move Formatting Into Adapters

Separate semantic state from transcript formatting.

Status:

- started
- terminal formatting for evaluated values now happens in the terminal adapter
- listing and informational output still need a cleaner split between semantic
  data and adapter rendering

Keep the current formatting helpers, but use them from adapter layers instead of
embedding formatting decisions in the engine.

This applies especially to:

- prompt rendering
- stack display
- `vars`
- `consts`
- `funcs`
- console-only list truncation

### Phase 4: Add State-Oriented Tests

Add tests that validate engine behavior directly rather than through transcript
comparison.

Status:

- started
- `console_session_engine_test.cpp` now covers direct engine behavior
- more state-oriented tests should be added once `SessionState` and richer
  command-result types exist

Priority cases:

- assignment updates definition state
- stack operators mutate stack correctly
- `fx_refresh` updates generated definitions
- display mode changes update state
- invalid commands return structured errors without corrupting state

The existing transcript tests should remain for terminal behavior.

### Phase 5: Make External Providers Fully Host-Injectable

The currency feature already uses a provider interface. Continue that direction.

Requirements:

- the session engine depends only on an abstract provider
- the terminal application can use the libcurl-backed provider
- a browser/WASM host can later provide rates through JS interop or disable the
  feature entirely

This keeps native networking out of the future WASM-compatible engine surface.

### Phase 6: Introduce a WASM-Oriented Build Target

Once the session engine is transport-free:

- keep the terminal application as-is
- add a separate build target for WASM-compatible code
- exclude terminal-only code such as raw line editing from that target

At that point the likely reusable pieces are:

- `console_calc_lib`
- session engine library
- app-layer provider abstractions

The likely non-WASM pieces are:

- terminal line editor
- ANSI prompt rendering
- libcurl-based currency transport

## Suggested Types

These types would make the engine easier to expose to a browser:

- `SessionState`
- `SessionEngine`
- `CommandResult`
- `OutputEvent`
- `DefinitionView`
- `FunctionView`
- `StackEntryView`

The exact names can change. The important part is to expose structured state
instead of raw console text.

## Suggested File Direction

Probable future structure:

- `src/`
  - core evaluator/parser/value logic

- `apps/`
  - terminal adapter
  - provider implementations

- future session files, either in `apps/` or a new shared layer:
  - `session_state.*`
  - `session_engine.*`
  - `session_result.*`

If the session engine becomes broadly reusable, promoting it out of terminal-only
`apps/` code into a more shared library layer may be appropriate.

## Non-Goals For The First WASM Step

These should not be done first:

- porting the terminal line editor to the browser
- binding the current stream-based `ConsoleSession` directly to JS
- adding web-specific syntax to the calculator language
- moving HTTP/curl logic into the parser/evaluator core

## Recommended Order

1. extract transport-free session engine
2. introduce explicit session-state and structured command-result types
3. keep terminal mode as an adapter over that engine/state model
4. add state-oriented tests
5. keep currency/network behavior provider-based
6. add a separate WASM build target

## Readiness Criteria

The project is ready for a WebAssembly proof-of-concept when:

- command execution no longer depends on streams
- terminal formatting is adapter-only
- session state is serializable/inspectable
- currency refresh is optional and provider-based
- terminal-only code is excluded from the reusable session layer

At that point, the browser UI becomes a frontend problem instead of a core
architecture problem.
