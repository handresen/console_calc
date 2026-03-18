# Development Notes

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

## Tool Installation

On macOS with Homebrew:

```bash
brew install clang-format clang-tidy
```
