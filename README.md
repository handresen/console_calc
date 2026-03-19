# console_calc

`console_calc` is a small command-line calculator with:
- one-shot expression evaluation
- an interactive console mode
- scalar and list values
- late-bound user definitions in console mode

The project is intentionally compact and library-first. Parsing and evaluation live in the core library, while REPL behavior stays in the app layer.

Current build layers:
- `console_calc_lib`
  core parser, evaluator, values, and builtin metadata
- `console_calc_runtime_lib`
  shared runtime/session logic, listings, environment expansion, and provider abstractions
- `console_calc_host_lib`
  host-facing binding facade over the runtime/session layer
- `console_calc`
  terminal application, built only when terminal-app support is enabled

## Quick Start

Evaluate a single expression:

```bash
./build/default/console_calc "2 + 3 * 4"
```

Start interactive console mode:

```bash
./build/default/console_calc
```

Build the host-facing reusable slice without the terminal application:

```bash
cmake --preset host-only
cmake --build --preset host-only
ctest --preset host-only
```

This preset is the current “WASM-ready” build boundary. It keeps the core,
runtime, and host-facing facade while excluding terminal-only code such as the
line editor and console executable.

Build the first Emscripten/WASM host artifact:

```bash
source ~/emsdk/emsdk_env.sh
EM_CACHE="$PWD/build/emscripten-cache" cmake --preset emscripten-host
EM_CACHE="$PWD/build/emscripten-cache" cmake --build --preset emscripten-host
```

This produces:
- `build/emscripten-host/console_calc.mjs`
- `build/emscripten-host/console_calc.wasm`

The current wasm bridge exports a small C-facing session API for creating a
session, submitting commands, and retrieving the last command result as JSON.

## Expression Language

Supported values:
- intrinsic integer and floating-point scalars
- lists like `{1, 2, 3}`

Supported operators:
- unary `-`
- binary `+`, `-`, `*`, `/`, `%`, `^`, `&`, `|`

Grouping:
- parentheses `(...)`

Operator precedence, highest to lowest:
1. function calls, list literals, parentheses
2. `^`
3. unary `-`
4. `*`, `/`, `%`
5. `+`, `-`
6. `&`
7. `|`

Notes:
- `^` is right-associative
- `%` uses floating-point modulo
- `&` and `|` require integer-valued operands
- plain decimal integers such as `42` are kept as integer values
- hexadecimal literals such as `0xff` and binary literals such as `0b1010` are kept as integer values
- decimal values with a fractional part or exponent are floating-point values
- one-element lists are accepted in scalar positions
- list literals are currently flat; nested lists are rejected

Current integer-preserving behavior:
- `+`, `-`, and `*` keep integer results when both inputs are integers and the result fits in 64 bits
- `/` always yields a floating-point result
- `%` yields an integer result when both inputs are integers
- `len(...)` returns an integer
- trig functions return floating-point values

Examples:

```text
2 + 3 * 4           => 14
-2^2                => -4
(-2)^2              => 4
0xff & 0b1010       => 10
10 % 3              => 1
6 & 3 | 8           => 10
first(1, {2, 3})+4  => 6
sum(map({0, 90}, sind(_))) => 1
```

## Builtin Constants

Available in both one-shot mode and console mode:
- `pi`
- `e`
- `tau`

Examples:

```text
sin(pi / 2)
tau / 2
pow(e, 1)
```

## Builtin Functions

### Scalar Functions

- `sin(x)`      sine in radians
- `cos(x)`      cosine in radians
- `tan(x)`      tangent in radians
- `sind(x)`     sine in degrees
- `cosd(x)`     cosine in degrees
- `tand(x)`     tangent in degrees
- `pow(x, y)`   power
- `guard(expr, fallback)` evaluate `fallback` only if `expr` fails

### List Functions

- `sum(list)`        sum list elements
- `len(list)`        list length
- `product(list)`    product of list elements
- `avg(list)`        average of list elements
- `min(list)`        minimum list element
- `max(list)`        maximum list element
- `first(n, list)`   first `n` list elements
- `drop(n, list)`    drop first `n` list elements
- `guard(expr, fallback)` return fallback when expression evaluation fails
- `list_div(a, b)`   divide matching list elements
- `list_mul(a, b)`   multiply matching list elements
- `reduce(list, op)` reduce a list with a binary operator
- `map(list, expr)`  map an inline expression using `_` as the current element

### List Generation Functions

- `range(start, count[, step])` generate `count` values starting at `start`
- `geom(start, count[, ratio])` generate a geometric series
- `repeat(value, count)` repeat a value `count` times
- `linspace(start, stop, count)` generate evenly spaced values over an interval
- `powers(base, count[, start_exp])` generate successive powers of a base

Function notes:
- `product({})` is `1`
- `avg`, `min`, and `max` require a non-empty list
- `first` and `drop` require `n` to be a non-negative integer
- `list_div` requires both inputs to be lists of equal length
- `list_mul` requires both inputs to be lists of equal length
- `reduce` requires a non-empty list
- `reduce` uses existing binary operators such as `+`, `-`, `*`, `/`, `%`, `^`, `&`, `|`
- `map` accepts an inline expression using `_` as the current element
- `map({1, 2}, sum)` and `map({1, 2}, pow)` are invalid
- `map({1, 2}, sin)` is invalid
- `_` is only valid inside `map(..., expr)`
- `guard` evaluates its fallback lazily and can be used inside `map`
- `range` requires `count` to be a non-negative integer
- `range(start, count)` uses a default step of `1`
- `range` preserves integer list elements when `start` and `step` are integers
- `geom` requires `count` to be a non-negative integer
- `geom(start, count)` uses a default ratio of `2`
- `repeat` requires `count` to be a non-negative integer
- `linspace` requires `count` to be a non-negative integer
- `linspace(start, stop, 0)` returns `{}`
- `linspace(start, stop, 1)` returns `{start}`
- `powers(base, count)` starts at exponent `0`

Examples:

```text
sum({1, 2, 3})                => 6
avg({2, 4, 6})                => 4
first(2, {10, 20, 30})        => {10, 20}
drop(1, {10, 20, 30})         => {20, 30}
list_div({8, 9}, {2, 3})      => {4, 3}
list_mul({2, 3}, {4, 5})      => {8, 15}
reduce({2, 3, 4}, *)          => 24
map({0, 90}, sind(_))         => {0, 1}
map({1, 2, 3}, _ + 1)         => {2, 3, 4}
map({1, 2, 3}, sin(_) + _)    => {1.84147..., 2.90929..., 3.14112...}
guard(1 / 0, 0)               => 0
map(range(-2, 5), guard(1 / _, 0))
sum(map({1, 2, 3}, sin(_)))   => 1.89189...
range(10, 4)                  => {10, 11, 12, 13}
range(2, 4, 3)                => {2, 5, 8, 11}
geom(2, 4)                    => {2, 4, 8, 16}
repeat(3, 4)                  => {3, 3, 3, 3}
linspace(0, 1, 5)             => {0, 0.25, 0.5, 0.75, 1}
powers(-1, 4)                 => {1, -1, 1, -1}
```

## Pi Example

You can approximate pi in console mode with the Leibniz series:

```text
0> n:10000
0> denom:range(1, n, 2)
0> alt:powers(-1, n)
0> series:list_div(alt, denom)
0> ppi:sum(series) * 4
0> ppi
3.14149
```

Because console definitions are late-bound, changing `n` recomputes the whole chain:

```text
0> n:100000
0> ppi
3.14158
```

## Console Mode

Starting with no arguments enters interactive mode.

Prompt format:

```text
<stack depth>>
```

The prompt is shown in green. The stack currently keeps up to `100` values by default.

Each successful expression pushes its result onto the stack.

Examples:

```text
0> 1+1
2
1> {1,2,3}
{1, 2, 3}
2> sum(r)
6
3>
```

### Console Commands

- `q` or `Q`
  Exit console mode
- `s`
  List stack as `level:value`
- `vars`
  List user definitions
- `consts`
  List builtin constants
- `funcs`
  List builtin functions with grouped help text
- `fx_refresh`
  Refresh console-only NOK-relative currency conversion variables
- `dup`
  Duplicate the top stack value
- `drop`
  Remove the top stack value
- `swap`
  Swap the top two stack values
- `clear`
  Clear the stack
- `dec`
  Display integers in decimal
- `hex`
  Display integers in hexadecimal
- `bin`
  Display integers in binary
- single operator line: `+`, `-`, `*`, `/`, `%`, `^`, `&`, `|`
  Apply that operator to the top two scalar stack values

### Result Reference

`r` expands to the current top-of-stack value inside console expressions.

Examples:

```text
0> pi
3.14159
1> r*2
6.28319
2> sum({1, r})
7.28319
```

If the top of stack is a list, `r` expands to that full list value:

```text
0> {1,2,3}
{1, 2, 3}
1> sum(r)
6
```

Long list results are truncated in console output after the first 10 entries:

```text
0> range(1, 12)
{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, <hiding 2 entries>}
```

### User Definitions

Console mode supports late-bound named definitions:

```text
x:pi+1
sx:sin(x)
sx
```

Definitions are resolved when used, not when declared.

That means reassignment affects later evaluations:

```text
0> x:pi+1
0> sx:sin(x)
0> sx
-0.841471
1> x:0
1> sx
0
```

Definitions can also hold list expressions:

```text
vals:1,2,3,4
sum(vals)
map(vals, sin(_))
```

Rules:
- definition syntax is `name:expression`
- redefining a builtin constant name is rejected
- redefining an existing user definition replaces it
- circular references are rejected

## Currency Conversions

Console mode can load a small built-in set of NOK-relative currency conversions on launch.
This is best-effort with a short timeout. If the fetch fails, console mode still starts and no
currency variables are added.

You can refresh the rates explicitly with:
- `fx_refresh`

Current generated variables include:
- `nok2usd`, `usd2nok`
- `nok2cny`, `cny2nok`
- `nok2eur`, `eur2nok`
- `nok2gbp`, `gbp2nok`
- `nok2sek`, `sek2nok`
- `nok2dkk`, `dkk2nok`

Example:

```text
0> nok2usd
0.0951
1> usd2nok
10.5152
2> fx_refresh
2> nok2usd
0.0953
```

## Integer Display Modes

Console mode keeps decimal as the default display format.

You can switch integer display with:
- `dec`
- `hex`
- `bin`

Only intrinsic integer values change representation. Floating-point values continue to display in decimal.

Example:

```text
0> 255
255
1> hex
1> s
0:0xff
1> bin
1> s
0:0b11111111
1> 1.5
1.5
2> s
0:0b11111111
1:1.5
```

## Keyboard Support In Console Mode

Interactive console mode supports:
- up arrow for command history
- left/right arrows for cursor movement

The console keeps the last 10 submitted commands.

## Errors

Invalid expressions and invalid console operations print:

```text
error: <message>
```

Examples:
- division by zero
- modulo by zero
- using multi-element lists where a scalar is required
- invalid bitwise operands
- unknown identifiers
- stack underflow

## Project Layout

- `include/console_calc/`
  Public library headers
- `src/`
  Core parser, evaluator, value formatting, builtin metadata
- `apps/`
  Console app, REPL session logic, definition expansion
- `tests/`
  Conformance, console app, and command/listing tests
- `docs/`
  Grammar and development notes

For implementation details, see [docs/grammar.md](docs/grammar.md) and [docs/development.md](docs/development.md).

## Build

Configure and build with CMake:

```bash
cmake --preset default
cmake --build --preset default
```

Run tests:

```bash
ctest --preset default
```

Release build:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Host-only build:

```bash
cmake --preset host-only
cmake --build --preset host-only
ctest --preset host-only
```

Executable path:

```bash
./build/default/console_calc
```
