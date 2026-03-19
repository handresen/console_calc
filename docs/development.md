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

For host-facing or future WebAssembly preparation work, prefer depending on
`console_calc_host_lib` rather than the terminal application target.

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

## Emscripten Host Build

The first browser-oriented build target is `emscripten-host`. It produces a
minimal `.mjs` + `.wasm` artifact around the host-facing binding facade and C
bridge.

Prerequisites:

- `emsdk` installed
- `source ~/emsdk/emsdk_env.sh` available in the shell used for configure/build

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

## Formatting

This repository uses `clang-format` with the root configuration in `.clang-format`.

Format the codebase with:

```bash
clang-format -i apps/*.cpp src/*.cpp tests/*.cpp include/console_calc/*.h
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
  src/expression_parser.cpp \
  src/expression_ast_parser.cpp \
  src/expression_evaluator.cpp \
  src/expression_tokenizer.cpp \
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
