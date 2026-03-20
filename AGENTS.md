# AGENTS.md

## Project Overview
- This repository contains a C++ command-line tool for parsing and evaluating mathematical expressions.
- The project should start small, remain easy to reason about, and grow by adding parser features in controlled steps.

## Stack Decisions
- Build system: `CMake`
- Dependency source: `vcpkg` in manifest mode
- Language preference: `C++20` or later
- Test framework: add through `vcpkg` when tests are introduced; do not hand-roll a custom test runner unless explicitly requested
- Linting and formatting: `clang-tidy` and `clang-format`

## Repository Layout
- `CMakeLists.txt`
  - root build configuration
- `vcpkg.json`
  - dependency manifest
- `include/console_calc/`
  - public headers for reusable parsing and evaluation components
- `src/`
  - implementation files
- `apps/`
  - command-line executable entrypoints
- `bindings/`
  - binding-facing bridge code such as the host facade, C API, and wasm entrypoint
- `tests/`
  - unit and integration tests
- `docs/`
  - grammar notes, roadmap, and design decisions
  - developer workflow notes

## Design Direction
- Keep CLI concerns separate from parsing and evaluation logic.
- Split the codebase into a portable core library and a console runner.
- Prefer a library-first structure even if the repository currently exposes only one executable.
- Keep the core library portable across Windows, Linux, and Android.
- Do not optimize for cross-compiling yet; portability should come from clean platform-neutral code and build structure, not early toolchain complexity.
- Code should be concise but still easy for a human to read and modify.
- Implement core logic in this repository, including tokenization, parsing, evaluation flow, and related error handling.
- Favor standard library facilities over external dependencies for core calculator behavior.
- Use third-party packages for heavier capabilities that are out of scope for the core parser, such as symbolic manipulation, advanced numeric methods, or similar specialized features.
- Introduce external dependencies only when they clearly improve correctness, maintainability, test quality, or provide substantial heavy-lifting functionality.

## Current Language Scope
- The CLI supports both one-shot evaluation from command-line arguments and an interactive console mode when started with no arguments.
- Scalar values are intrinsic `int64` or floating-point values. List values are first-class and currently flat. Geographic positions are a first-class intrinsic value type using the `(lat, lon)` convention in degrees.
- Supported binary operators are `+`, `-`, `*`, `/`, `%`, `^`, `&`, and `|`.
- Unary `-` is supported.
- Parentheses and list literals are supported for grouping and list construction.
- Operator precedence is currently: function calls / list literals / parentheses, `^`, unary `-`, `*` `/` `%`, `+` `-`, `&`, `|`.
- `^` is right-associative. The other binary operators are left-associative.
- `/` always yields floating-point results.
- `%` preserves integer results for integer operands and otherwise uses floating-point modulo semantics.
- `&` and `|` require integer-valued operands and should reject non-integer inputs.
- Builtin constants include `pi`, `e`, and `tau`.
- Builtin scalar functions include `abs`, `sqrt`, trig functions, `pow`, `rand([min, max])`, and `timed_loop(expr, count)`.
- Builtin position functions include `pos`, `lat`, `lon`, `dist`, `bearing`, and `br_to_pos`.
- Builtin list functions include aggregation (`sum`, `product`, `avg`, `min`, `max`, `len`), slicing (`first`, `drop`), pairwise operations (`list_add`, `list_sub`, `list_mul`, `list_div`), `map`, and `reduce`.
- Builtin list generation functions include `range`, `geom`, `repeat`, `linspace`, and `powers`.
- Special forms currently include `map(list, expr)`, `reduce(list, op)`, `guard(expr, fallback)`, and `timed_loop(expr, count)`.
- The console supports late-bound user definitions, result-stack semantics, integer display modes (`dec`, `hex`, `bin`), and console-native commands such as `s`, `vars`, `consts`, and `funcs`.
- Console mode also supports best-effort currency-rate refresh in the app layer.
- The app layer is responsible for console-only syntax and identifier expansion, including `r` result references and late-bound definitions.
- The web frontend is active on `main`, consumes the wasm host bridge, and provides transcript, helper panes, samples, and plotting without reimplementing calculator semantics in TypeScript.

## Working Rules For Agents
- Follow the existing CMake and `vcpkg` structure unless the user asks to replace it.
- Prefer small, reviewable commits and isolated edits.
- When adding dependencies, declare them in `vcpkg.json` and wire them through CMake targets.
- Keep include paths explicit and target-scoped in CMake.
- Add tests alongside behavior changes when practical.
- Avoid parser generators unless explicitly requested.
- Keep `.clang-format` and `.clang-tidy` in sync with the project's actual conventions.
- Keep implementations compact, but do not trade away readability for terseness.
- Do not outsource tokenizing, parsing, or other core expression-processing logic to external libraries unless explicitly requested.
- Keep platform-specific code out of the core library unless there is no reasonable alternative.
- Treat the console application as a thin adapter over the core library.
- Keep binding-facing bridge code in `bindings/` rather than mixing it back into the core library or terminal app layers.
- Document grammar and operator-precedence assumptions as they become concrete.
- Keep the interactive console behavior in the app layer rather than mixing REPL concerns into the parser library.
- Keep console-only syntax expansion token-aware and localized in the app layer rather than letting it become a second ad hoc parser.
- Keep the expression test corpus in `tests/expression_cases.txt` using quoted expressions, a comma, and an expected result token per line, with `#` comments for section headers.
- When parser behavior changes, update both `docs/grammar.md` and the externalized expression test data.
- When console commands or builtin-function metadata change, update the focused command/listing tests in `tests/console_command_test.cpp` and the user-facing `README.md` when appropriate.
- When adding or refactoring focused unit tests, prefer small reusable failure-diagnostics helpers over ad hoc temporary `printf` debugging. Leave targeted instrumentation in place when it materially improves future failure diagnosis without cluttering the tests.
- After refactor-focused passes, include a short churn summary in the close-out:
  - net lines added/deleted
  - count of files touched
  - count of new files
  - count of deleted files
  - whether the refactor was useful or should be reverted

## Near-Term Priorities
1. Preserve correctness while extending the grammar in small, test-backed steps.
2. Keep the core value model, builtin-function metadata, and evaluator semantics cohesive as list and integer features expand.
3. Refine console-mode behavior and future result-stack semantics without coupling them to the parser core.
4. Keep grammar notes, README examples, command/listing tests, and external test data synchronized with behavior.

## Forward Plan
- The current forward-looking extension plan for the project is documented in `docs/wasm_extension_plan.md`.
- Treat that document as the canonical next-steps plan for preparing the calculator/runtime for a future browser and WebAssembly frontend.
- Follow that plan by separating session semantics from terminal presentation before attempting any web UI or WASM binding work.
- That separation is now largely in place on `main`: transport-free session handling, structured snapshots/events, a binding-facing facade, a C API, and a working wasm/web frontend already exist. Future work should extend that direction rather than reintroducing terminal-shaped APIs into reusable layers.
