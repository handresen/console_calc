# Web Frontend

This directory contains the active browser frontend for `console_calc`.

Current status:

- Vite + TypeScript
- no framework selected
- consumes the wasm host bridge built from the repository root
- renders:
  - console-style transcript
  - prompt with history support
  - helper panes for stack, definitions, constants, functions, and samples
  - a client-only plot pane for the topmost list on the stack
- shows per-command timing in the transcript
- keeps calculator/session semantics in the wasm host layer rather than in the UI

Relevant root-side artifacts:

- `build/emscripten-host/console_calc.mjs`
- `build/emscripten-host/console_calc.wasm`

Typical flow during development:

1. Build the wasm host artifact from the repository root.
2. Run `npm run dev` inside `web/`.
3. Open the Vite dev server, or the reverse-proxied `/cc/` route if configured.
4. The Vite config serves `build/emscripten-host/*` at `/wasm/*` during dev.

Typical commands:

```bash
source ~/emsdk/emsdk_env.sh
EM_CACHE="$PWD/../build/emscripten-cache" cmake --preset emscripten-host
EM_CACHE="$PWD/../build/emscripten-cache" cmake --build --preset emscripten-host
npm run dev
```

The frontend should consume the host-facing bridge instead of reimplementing
calculator/session behavior locally.
