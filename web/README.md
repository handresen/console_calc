# Web Frontend

This directory is reserved for the future browser frontend for `console_calc`.

Planned role:

- load the WebAssembly/host-facing calculator bridge
- provide a console-style main interaction surface
- optionally render helper panes for stack, definitions, constants, and
  builtin functions

Current status:

- Vite + TypeScript scaffolded
- no framework selected
- the current wasm artifact is produced from the root build via the
  `emscripten-host` preset
- the first browser round-trip now loads the wasm bridge, creates a session,
  initializes it, submits commands, and renders transcript + helper panes

Relevant root-side artifacts:

- `build/emscripten-host/console_calc.mjs`
- `build/emscripten-host/console_calc.wasm`

Typical flow during development:

1. Build the wasm host artifact from the repository root.
2. Run `npm run dev` inside `web/`.
3. The Vite config serves `build/emscripten-host/*` at `/wasm/*` during dev.

The frontend should consume the host-facing bridge instead of reimplementing
calculator/session behavior locally.
