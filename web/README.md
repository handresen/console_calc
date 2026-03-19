# Web Frontend

This directory is reserved for the future browser frontend for `console_calc`.

Planned role:

- load the WebAssembly/host-facing calculator bridge
- provide a console-style main interaction surface
- optionally render helper panes for stack, definitions, constants, and
  builtin functions

Current status:

- directory scaffold only
- no JS toolchain chosen yet
- the current wasm artifact is produced from the root build via the
  `emscripten-host` preset

Relevant root-side artifacts:

- `build/emscripten-host/console_calc.mjs`
- `build/emscripten-host/console_calc.wasm`

The frontend should consume the host-facing bridge instead of reimplementing
calculator/session behavior locally.
