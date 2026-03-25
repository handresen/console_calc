# Development Notes

## Target Layers

The project currently builds in four layers:

- `console_calc_lib`
  Core parser/evaluator/value logic
- `console_calc_runtime_lib`
  Shared runtime/session logic, listings, identifier expansion, and provider abstractions
- `console_calc_host_lib`
  Host-facing binding facade intended as the reusable bridge for future web/WASM work
- `console_calc`
  Terminal executable and terminal-only adapter code

Implementation is currently organized across:

- `src/`
  core library implementation
- `apps/`
  runtime/session and terminal adapter code
- `bindings/`
  binding-facing implementation such as the facade, C API, and wasm entrypoint

See [architecture.md](./architecture.md) for the current intended layer boundaries.

For host-facing or future WebAssembly preparation work, prefer depending on
`console_calc_host_lib` rather than the terminal application target.
If behavior is reusable by both terminal and web hosts, it should usually land
below those adapters unless it is clearly presentation-specific.

## Presets

Available CMake presets:

- `default`
  Debug build with the full terminal application and tests
- `release`
  Release build with the full terminal application and tests
- `host-only`
  Builds the reusable core/runtime/host-facing slice with `CONSOLE_CALC_BUILD_TERMINAL_APP=OFF`
- `emscripten-host`
  Builds the host-facing slice with the Emscripten toolchain, terminal app disabled, tests disabled, and native currency transport disabled
- `clang-tidy`
  Debug-style build with `clang-tidy` enabled

The `host-only` preset is the current build-level guardrail for “WASM-ready”
work. If a new dependency or source change accidentally pulls terminal-only code
into the host layer, this preset should catch it.

## Workflow Guardrails

Before larger feature or refactor passes, do a quick worktree scan so it is
clear which changes belong to the current task and which were already present.
That matters in this repo because native core work, wasm host work, and web UI
work often overlap in one checkout.

When verifying changes, wait for the command to finish rather than reading
partial output as success. In practice that means not reporting green status
until the relevant native build, `ctest`, wasm build, or `web/` build command
has exited successfully.

## Feature Workflow

For normal feature work, prefer this order when it fits the task:

1. implement the feature
2. run the relevant checks
3. update tests, docs, and nearby comments
4. summarize the resulting state briefly
5. optionally perform narrow adjacent cleanup
6. run the relevant checks again
7. commit

This keeps verification and documentation close to the behavior change while
still leaving room for small local improvements.

### Local Cleanup Versus Structural Refactor

Local cleanup is appropriate during feature work when it stays close to the
change and does not alter subsystem boundaries. Typical examples:

- rename a confusing local variable
- extract a tiny helper
- remove obvious duplication introduced by the feature
- tidy directly touched docs, comments, or tests

Structural refactor should usually be treated as a separate decision. Typical
examples:

- changing subsystem boundaries
- introducing a new abstraction layer
- consolidating concepts across multiple modules
- reworking public API shapes
- moving ownership or responsibility between components

This distinction matters in this repo because small grammar changes can easily
tempt broader parser, runtime, or web-UI restructuring. Those larger moves are
often valid, but they should be chosen deliberately rather than justified only
because the code was already open.

## Major-Change Review

When a task starts to suggest broader cleanup, do a short explicit review of
the changed subsystem before proceeding:

1. identify issues, complexity growth, or boundary problems
2. list possible simplifications
3. rank the highest-value refactors
4. decide whether to defer, schedule, or execute the refactor separately

This keeps feature work from quietly turning into architecture work and makes
later rollback or follow-up planning much easier.

## Dependencies

The project uses `vcpkg` in manifest mode. Current non-test dependencies include:

- `curl`
  for native console-side currency refresh
- `geographiclib`
  for WGS84 geodesic calculations behind the intrinsic position functions

Native presets and the `emscripten-host` preset are configured to use the
manifest through `~/vcpkg/scripts/buildsystems/vcpkg.cmake`.

## Emscripten Host Build

The first browser-oriented build target is `emscripten-host`. It produces a
minimal `.mjs` + `.wasm` artifact around the host-facing binding facade and C
bridge.

Prerequisites:

- `emsdk` installed
- `source ~/emsdk/emsdk_env.sh` available in the shell used for configure/build
- `vcpkg` installed at `~/vcpkg`

Build with a repo-local Emscripten cache to avoid writing under the global
`emsdk` cache during sandboxed or restricted runs:

```bash
source ~/emsdk/emsdk_env.sh
EM_CACHE="$PWD/build/emscripten-cache" cmake --preset emscripten-host
EM_CACHE="$PWD/build/emscripten-cache" cmake --build --preset emscripten-host
```

Outputs:

- `build/emscripten-host/console_calc.mjs`
- `build/emscripten-host/console_calc.wasm`

The Emscripten preset uses `vcpkg` with `VCPKG_CHAINLOAD_TOOLCHAIN_FILE` so the
wasm build consumes the same manifest dependencies as the native builds.

## Formatting

This repository uses `clang-format` with the root configuration in `.clang-format`.

Format the codebase with:

```bash
clang-format -i \
  apps/*.cpp \
  bindings/**/*.cpp \
  src/*.cpp \
  tests/*.cpp \
  include/console_calc/*.h
```

## Linting

This repository supports `clang-tidy` through CMake, but the integration is opt-in.

Configure and build with `clang-tidy` enabled:

```bash
cmake --preset clang-tidy
cmake --build --preset clang-tidy
```

Direct invocation is also possible once `compile_commands.json` has been generated:

```bash
clang-tidy -p build/default \
  apps/console_calc_main.cpp \
  bindings/c_api/console_binding_c_api.cpp \
  bindings/facade/console_binding_facade.cpp \
  src/expression_parser.cpp \
  src/expression_ast_parser.cpp \
  src/expression_evaluator.cpp \
  src/expression_tokenizer.cpp \
  src/geodesy.cpp \
  tests/expression_conformance_test.cpp \
  tests/expression_case_loader.cpp
```

## Host-Only Verification

To verify the reusable host/runtime slice independently of the terminal app:

```bash
cmake --preset host-only
cmake --build --preset host-only
ctest --preset host-only
```

## Tool Installation

On macOS with Homebrew:

```bash
brew install clang-format clang-tidy
```

## Web Frontend

The browser frontend lives in `web/` and consumes the wasm host bridge rather
than reimplementing calculator semantics locally.

Typical development flow:

```bash
source ~/emsdk/emsdk_env.sh
EM_CACHE="$PWD/build/emscripten-cache" cmake --preset emscripten-host
EM_CACHE="$PWD/build/emscripten-cache" cmake --build --preset emscripten-host
cd web
npm install
npm run dev
```

The frontend expects the wasm artifacts from `build/emscripten-host/` and the
Vite config serves them during development.
