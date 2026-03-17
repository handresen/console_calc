# AGENTS.md

## Project Overview
- This repository contains a C++ command-line tool for parsing and evaluating mathematical expressions.
- The project should start small, remain easy to reason about, and grow by adding parser features in controlled steps.

## Stack Decisions
- Build system: `CMake`
- Dependency source: `vcpkg` in manifest mode
- Language preference: `C++20` or later
- Test framework: add through `vcpkg` when tests are introduced; do not hand-roll a custom test runner unless explicitly requested

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
- `tests/`
  - unit and integration tests
- `docs/`
  - grammar notes, roadmap, and design decisions

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

## Working Rules For Agents
- Follow the existing CMake and `vcpkg` structure unless the user asks to replace it.
- Prefer small, reviewable commits and isolated edits.
- When adding dependencies, declare them in `vcpkg.json` and wire them through CMake targets.
- Keep include paths explicit and target-scoped in CMake.
- Add tests alongside behavior changes when practical.
- Avoid parser generators unless explicitly requested.
- Keep implementations compact, but do not trade away readability for terseness.
- Do not outsource tokenizing, parsing, or other core expression-processing logic to external libraries unless explicitly requested.
- Keep platform-specific code out of the core library unless there is no reasonable alternative.
- Treat the console application as a thin adapter over the core library.
- Document grammar and operator-precedence assumptions as they become concrete.

## Near-Term Priorities
1. Finalize the initial project scaffold.
2. Define the first supported grammar.
3. Implement tokenization.
4. Implement parsing.
5. Implement evaluation and user-facing error reporting.
