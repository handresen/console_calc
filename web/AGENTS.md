# AGENTS.md

## Scope

This directory is reserved for the future browser frontend that consumes the
WebAssembly/host-facing calculator runtime.

The intended relationship is:

- C++ core and runtime remain outside `web/`
- `web/` consumes the host-facing binding surface
- browser UI concerns stay here rather than leaking back into `apps/`

## Structure Expectations

Keep web-facing code under this directory, for example:

- `src/`
  frontend application source
- `public/`
  static assets

Do not move core parser, evaluator, runtime/session, or terminal-adapter code
into `web/`.

## Design Direction

- Treat the browser app as a consumer of the host-facing bridge, not as a place
  to reimplement calculator semantics.
- Prefer the existing WebAssembly/host binding surface over inventing new
  browser-only behavior in parallel.
- Keep UI state derived from structured session snapshots and command events.
- Avoid parsing terminal transcript strings in the web app when structured data
  is available.

## Near-Term Intent

- Build a simple console-first browser UI.
- Keep side panes optional and driven by structured stack, definition,
  constant, and function data.
- Keep the first web layer thin and pragmatic until the wasm bridge shape is
  better proven.
