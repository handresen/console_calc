# AGENTS.md

## Project Overview
- This repository contains a C++ calculator engine with both terminal and web hosts for parsing and evaluating mathematical expressions.
- The project should start small, remain easy to reason about, and grow by adding parser features in controlled steps.
- Treat the project as a focused personal tool rather than a general-purpose framework; preserve simplicity and avoid platform-style abstraction unless it clearly supports the existing use case.
- The web frontend on `main` is active and first-class; treat terminal and web as real hosts when making boundary and layering decisions.

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
- Scalar values are intrinsic `int64` or floating-point values. Scalar lists are first-class and currently flat. Homogeneous position lists are also supported as a separate value type. Geographic positions are a first-class intrinsic value type using the `(lat, lon)` convention in degrees.
- Supported unary operators are `-` and `~`.
- Supported binary operators are `+`, `-`, `*`, `/`, `%`, `^`, `=`, `<`, `<=`, `>`, `>=`, `&`, and `|`.
- Parentheses, list literals, and postfix list indexing are supported for grouping and collection access.
- Operator precedence is currently: function calls / postfix indexing / list literals / parentheses, `^`, unary `-` `~`, `*` `/` `%`, `+` `-`, comparisons, `&`, `|`.
- `^` is right-associative. The other binary operators are left-associative.
- `/` always yields floating-point results.
- `%` preserves integer results for integer operands and otherwise uses floating-point modulo semantics.
- Comparisons return integer `1` or `0`.
- `&`, `|`, and `~` require integer-valued operands and should reject non-integer inputs.
- Builtin constants include root math aliases `pi`, `e`, and `tau`, plus namespaced builtin constants such as `m.pi`, `c.deg`, and `ph.c`.
- Builtin scalar functions include arithmetic/trig helpers, `pow`, `rand([min, max])`, and explicit bitwise helpers such as `and`, `or`, `xor`, `nand`, `nor`, `shl`, and `shr`.
- Builtin position functions include `pos`, `lat`, `lon`, `dist`, `bearing`, `br_to_pos`, `to_list`, and `to_poslist`.
- Builtin list functions include aggregation (`sum`, `product`, `avg`, `min`, `max`, `len`), slicing (`first`, `drop`), pairwise operations (`list_add`, `list_sub`, `list_mul`, `list_div`), and list generation (`range`, `geom`, `repeat`, `linspace`, `powers`).
- Special forms currently include `map(list, expr[, start[, step[, count]]])`, `map_at(list, expr[, start[, step[, count]]])`, `list_where(list, expr)`, `reduce(list, op)`, `guard(expr, fallback)`, `timed_loop(expr, count)`, and `fill(expr, count)`.
- `_` is reserved as the current-element placeholder inside `map`, `map_at`, and `list_where`.
- The console supports late-bound value definitions and fixed-arity user-defined functions, result-stack semantics, integer display modes (`dec`, `hex`, `bin`), and console-native commands such as `s`, `vars`, `consts`, and `funcs`.
- Console mode also supports best-effort currency-rate refresh in the app layer.
- The app layer is responsible for console-only syntax and identifier expansion, including `r` result references and late-bound definitions.
- The web frontend is active on `main`, consumes the wasm host bridge, and provides transcript, helper panes, samples, and plotting without reimplementing calculator semantics in TypeScript.

## Working Rules For Agents
- Follow the existing CMake and `vcpkg` structure unless the user asks to replace it.
- Prefer small, reviewable commits and isolated edits.
- Do not parallelize git index-mutating commands such as `git add`, `git commit`, or similar staging/commit operations; run them sequentially to avoid stale index locks.
- For normal feature work, follow this order when practical: implement the feature, run the relevant checks, update docs/tests/comments, summarize the resulting state briefly, optionally perform narrow adjacent cleanup, run the relevant checks again, then commit.
- Narrow adjacent cleanup is allowed during feature work when it stays local to the touched code: rename a confusing local variable, extract a tiny helper, remove obvious duplication introduced by the feature, or tidy directly touched docs/comments/tests.
- Treat structural refactors as a separate decision unless the user explicitly asks for them or the current task cannot be completed cleanly without them. Structural refactors include changing subsystem boundaries, introducing new abstractions, consolidating concepts across modules, reworking public API shapes, or moving ownership between components.
- When a change feels large enough to justify structural cleanup, pause and evaluate the changed subsystem explicitly: identify complexity growth or boundary problems, list simpler alternatives, rank high-value refactors, and decide whether to defer, schedule, or execute the refactor separately.
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
- Before substantial feature or refactor passes, check whether the worktree is already dirty and distinguish new task changes from pre-existing local changes.
- Do not report verification complete until the relevant build or test command has fully exited successfully; avoid claiming green status from partial or still-running output.
- After refactor-focused passes, include a short churn summary in the close-out:
  - net lines added/deleted
  - count of files touched
  - count of new files
  - count of deleted files
  - whether the refactor was useful or should be reverted
- If a requested change appears unreasonable, internally inconsistent, or likely to degrade project cohesion, notify the user before proceeding instead of silently implementing it.
- When terms, naming, or category labels are inconsistent or can be improved materially, call that out explicitly before or alongside implementation so the vocabulary stays coherent.

## Near-Term Priorities
1. Preserve correctness while extending the grammar in small, test-backed steps.
2. Keep the core value model, builtin-function metadata, and evaluator semantics cohesive as list and integer features expand.
3. Refine console-mode behavior and future result-stack semantics without coupling them to the parser core.
4. Keep grammar notes, README examples, command/listing tests, and external test data synchronized with behavior.

## Forward Plan
- The current forward-looking extension plan for the project is documented in `docs/wasm_extension_plan.md`.
- Treat that document as the canonical status-and-direction note for the calculator/runtime's browser and WebAssembly host path.
- Follow that plan by separating session semantics from terminal presentation before attempting any web UI or WASM binding work.
- That separation is now largely in place on `main`: transport-free session handling, structured snapshots/events, a binding-facing facade, a C API, and a working wasm/web frontend already exist. Future work should extend that direction rather than reintroducing terminal-shaped APIs into reusable layers.
