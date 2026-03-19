# Architecture

## Layering

The project is intentionally split into a small set of layers with different
responsibilities:

1. `console_calc_lib`
   - Core language and value system
   - Tokenization, parsing, AST, evaluation, builtin metadata, scalar/list math
   - No terminal, browser, or transport-specific behavior

2. `console_calc_runtime_lib`
   - Session/runtime behavior on top of the core library
   - Console command classification and execution
   - Definition expansion, listing views, currency provider abstractions
   - Structured command events and session snapshots

3. `console_calc_host_lib`
   - Host-facing bridge over the runtime layer
   - Binding facade types intended for non-terminal consumers
   - C API export surface and WebAssembly entrypoint support

4. Terminal application
   - REPL loop, prompt rendering, line editing, ANSI behavior
   - Thin adapter over the runtime/session layer

5. `web/`
   - Browser UI consuming the WebAssembly host bridge
   - No calculator semantics should be reimplemented here when structured host
     data is already available

## Directory Intent

- `include/console_calc/`
  Public headers for the core library and host-facing bridge
- `src/`
  Core library implementation
- `apps/`
  Runtime/session logic and terminal adapters
- `bindings/`
  Binding-facing implementation details such as the facade, C API, and wasm entry
- `web/`
  Browser frontend

## Error Flow

Errors should originate as structured data as early as practical:

- Core and runtime code may throw exceptions internally
- The session engine converts user-facing failures into structured error events
- Host bindings preserve that structured error information in snapshots/events
- Terminal and web adapters format the error for their own presentation needs

This avoids duplicating ad hoc error interpretation in each frontend.

## Practical Rule

If a feature is reusable by both terminal and browser hosts, it should usually
live in the core, runtime, or host layers rather than in `apps/console_session.cpp`
or `web/src/`.
