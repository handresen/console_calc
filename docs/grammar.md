# Grammar Notes

## First Test Grammar

This initial grammar exists to exercise the parser pipeline with the smallest useful expression language.

Current scope:
- intrinsic integer and floating-point scalar values
- integer and floating-point numeric literals
- unary `-`
- binary `+`, `-`, `*`, `/`, `%`, `^`, `&`, `|`
- parentheses for grouping
- list literals with `{ ... }`
- function calls: `sin`, `cos`, `tan`, `sind`, `cosd`, `tand`, `pow`, `pos`, `lat`, `lon`, `dist`, `bearing`, `br_to_pos`, `sum`, `len`, `product`, `avg`, `min`, `max`, `first`, `drop`, `list_div`, `list_mul`, `guard`, `reduce`, `map`, `range`, `geom`, `repeat`, `linspace`, `powers`
- optional whitespace between tokens

Explicitly out of scope for this first version:
- variables or constants

## Tokens

- `number`
  - an integer or floating-point literal
  - examples: `0`, `7`, `123`, `3.14`, `.5`, `1.`, `1.3e10`, `6E-4`
- `operator`
  - one of `+`, `-`, `*`, `/`, `%`, `^`, `&`, `|`
- `unary operator`
  - `-`
- `grouping`
  - `(` `)` and `{` `}`
- `separator`
  - `,`
- `identifier`
  - used for builtin function names
  - `_` is reserved as the current-element placeholder inside `map(..., expr)`

## Grammar

```ebnf
expression = bitwise_or ;
bitwise_or = bitwise_and , { "|" , bitwise_and } ;
bitwise_and = sum , { "&" , sum } ;
sum        = term , { ( "+" | "-" ) , term } ;
term       = unary , { ( "*" | "/" | "%" ) , unary } ;
unary      = [ "-" ] , power ;
power      = primary , [ "^" , unary ] ;
primary    = number | function_call | map_call | guard_call | list | "(" , expression , ")" ;
function_call = identifier , "(" , expression , { "," , expression } , ")" ;
map_call   = "map" , "(" , expression , "," , expression , ")" ;
guard_call = "guard" , "(" , expression , "," , expression , ")" ;
list       = "{" , expression , { "," , expression } , "}" ;
number     = mantissa , [ exponent ] ;
mantissa   = digits , [ "." , [ digits ] ]
           | "." , digits ;
exponent   = ( "e" | "E" ) , [ "+" | "-" ] , digits ;
digits     = digit , { digit } ;
digit      = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
operator   = "+" | "-" | "*" | "/" | "%" | "^" | "&" | "|" ;
```

Accepted numeric forms include:
- integer literals such as `42`
- hexadecimal integer literals such as `0xff` and `0x10aa10`
- binary integer literals such as `0b1010` and `0b10010110`
- decimal literals such as `3.14`, `.25`, and `10.`
- exponent literals such as `1e6`, `2.5e-3`, and `.8E+2`

## Evaluation Rule

Expressions use these precedence levels, from highest to lowest: function calls, list literals, and parentheses, `^`, unary `-`, `*` `/` `%`, `+` `-`, `&`, `|`. `^` is right-associative. The other operators are left-associative.

`%` uses floating-point modulo via `fmod`. `&` and `|` require integer-valued operands; non-integer operands are rejected. Division by zero, modulo by zero, and non-finite evaluation results are rejected.

Where a scalar is required, a one-element list is accepted and coerced to that element. Multi-element lists are still rejected in scalar positions. List literals themselves remain flat: each list element must evaluate directly to a scalar, so nested lists are still rejected.

Integer-valued decimal literals such as `42`, hexadecimal literals such as `0xff`, and binary literals such as `0b1010` are preserved as intrinsic integer values. Decimal literals with a fractional part or exponent such as `3.14` or `1e3` are evaluated as floating-point values.

Builtin functions:
- `sin(x)`, `cos(x)`, `tan(x)` use radians
- `sind(x)`, `cosd(x)`, `tand(x)` use degrees
- `pow(x, y)` is equivalent to `x ^ y`
- `rand([min, max])` returns a random floating-point value in a half-open interval
- `pos(lat, lon)` constructs a WGS84 position from latitude/longitude in degrees
- `lat(pos)` and `lon(pos)` extract latitude and longitude in degrees
- `to_list(poslist)` expands positions into scalar values using `(lat, lon)` order
- `to_poslist(list)` pairs scalar list values into positions using `(lat, lon)` order
- `dist(pos1, pos2)` returns WGS84 ellipsoid distance in meters
- `bearing(pos1, pos2)` returns initial WGS84 bearing in degrees
- `br_to_pos(pos, bearing_deg, range_m)` returns a destination position from a start position, bearing, and range in meters
- `sum(list)` sums a list value
- `len(list)` returns list length
- `product(list)` multiplies all list values; `product({})` is `1`
- `avg(list)` returns the arithmetic mean of a non-empty list
- `min(list)` and `max(list)` require non-empty lists
- `first(n, list)` returns the first `n` items as a list
- `drop(n, list)` returns the list without its first `n` items
- `list_div(list_a, list_b)` divides list elements pairwise and requires equal list lengths
- `list_mul(list_a, list_b)` multiplies list elements pairwise and requires equal list lengths
- `guard(expr, fallback)` returns `expr` when it evaluates successfully, otherwise evaluates and returns `fallback`
- `timed_loop(expr, count)` evaluates `expr` `count` times and returns elapsed seconds as a floating-point value
- `fill(expr, count)` evaluates `expr` `count` times and returns the collected results as a list
- `reduce(list, op)` reduces a non-empty list left-to-right using a binary operator such as `+` or `*`
- `map(list, expr[, start[, step[, count]]])` evaluates `expr` over a list slice with `_` bound to the current element
- `map_at(list, expr[, start[, step[, count]]])` evaluates `expr` at selected list positions and preserves list length
- `range(start, count[, step])` generates a list beginning at `start`, with `count` elements, incrementing by `step` or by `1` when omitted
- `geom(start, count[, ratio])` generates a geometric series beginning at `start`, multiplying by `ratio` or by `2` when omitted
- `repeat(value, count)` repeats `value` `count` times
- `fill(expr, count)` repeatedly evaluates `expr` `count` times and returns the results as a list
- `linspace(start, stop, count)` generates `count` evenly spaced values from `start` to `stop`
- `powers(base, count[, start_exp])` generates successive powers of `base`, starting at exponent `start_exp` or `0`

Integer-preserving behavior:
- `+`, `-`, and `*` preserve integer results when both operands are integers and the result fits in 64 bits
- `/` always produces a floating-point result
- `%` produces an integer result when both operands are integers, otherwise floating-point modulo is used
- `len(list)` returns an integer
- `sum(list)` and `product(list)` preserve integer results when all inputs remain integral
- trig functions always return floating-point results

For `first` and `drop`, `n` must be a non-negative integer. If `n` is larger than the list length, the result is clamped naturally to the list bounds. For `map` and `map_at`, the second argument is an expression that uses `_` as the current element placeholder. `_` is only valid inside these mapping forms. Optional `start`, `step`, and `count` arguments use zero-based `start`; `step = 1` by default; and when `count` is omitted, mapping continues over all remaining matching elements. `step` must be a positive integer. `map` returns only mapped elements, while `map_at` preserves original list length and copies untouched elements through. For `timed_loop` and `fill`, `count` must be a non-negative integer. For `rand`, `rand()` uses `[0, 1)`, `rand(max)` uses `[0, max)`, and `rand(min, max)` uses `[min, max)` with finite bounds and `min < max`.

Geo positions are a dedicated value type, separate from scalars and lists. They use the `(lat, lon)` convention in degrees. Only the geo-specific functions accept position values.

Position lists are also supported as a separate homogeneous collection type. A
literal such as `{pos(60, 10), pos(61, 11)}` produces a position list. Scalar
lists remain scalar-only, and mixed scalar/position list literals are invalid.
`to_list(poslist)` converts a position list into a scalar list by expanding each
position as `lat, lon`.
`to_poslist(list)` converts an even-length scalar list into a position list by
pairing values as `(lat, lon)`. An odd number of values is invalid, and an
empty list returns an empty position list.

Examples:
- `2 + 3` => `5`
- `2 + 3 * 4` => `14`
- `-2^2` => `-4`
- `(-2)^2` => `4`
- `2 ^ 3 ^ 2` => `512`
- `sin(0)` => `0`
- `sind(30)` => `0.5`
- `pow(2, 3)` => `8`
- `lat(pos(60, 10))` => `60`
- `lon(pos(60, 10))` => `10`
- `to_list({pos(60, 10), pos(61, 11)})` => `{60, 10, 61, 11}`
- `to_poslist({60, 10, 61, 11})` => `{pos(60, 10), pos(61, 11)}`
- `dist(pos(0, 0), pos(0, 1))` => `111319.4907932264`
- `bearing(pos(0, 0), pos(0, 1))` => `90`
- `lon(br_to_pos(pos(0, 0), 90, 111319.4907932264))` => `1`
- `len({1, 2, 3})` => `3`
- `product({2, 3, 4})` => `24`
- `avg({2, 4, 6})` => `4`
- `min({2, -1, 5})` => `-1`
- `max({2, -1, 5})` => `5`
- `sum({1, 2, 3})` => `6`
- `sum(map({0, 90}, sind(_)))` => `1`
- `map({10, 20, 30, 40, 50}, _ + 1, 1, 2, 2)` => `{21, 41}`
- `map_at({10, 20, 30, 40, 50}, _ + 1, 1, 2, 2)` => `{10, 21, 30, 41, 50}`
- `sum(map({1, 2, 3}, _ + 1))` => `9`
- `sum(map({1, 2, 3}, sin(_) + _))` => `7.8918884196934453`
- `guard(1 / 0, 0)` => `0`
- `timed_loop(sin(pi / 3), 1000)` => a non-negative elapsed time in seconds
- `fill(1 + 2, 3)` => `{3, 3, 3}`
- `{pos(60, 10), pos(61, 11)}` => a position list
- `rand()` => a value in `[0, 1)`
- `sum(map(range(-2, 5), guard(1 / _, 0)))` => `0`
- `sum(list_div(powers(-1, 4), range(1, 4, 2)))` => `0.72380952380952379`
- `sum(list_mul({2, 3, 4}, {5, 6, 7}))` => `56`
- `reduce({2, 3, 4}, *)` => `24`
- `sum(range(2, 4, 3))` => `26`
- `sum(geom(3, 4, 3))` => `120`
- `sum(repeat(2, 4))` => `8`
- `sum(linspace(1, 4, 4))` => `10`
- `first(1, {2, 3}) + 4` => `6`
- `10 % 3` => `1`
- `6 & 3 | 8` => `10`
- `(2 + 3) * 4` => `20`
- `20 / 5 - 1` => `3`
- `2 * 3 + 4 * 5` => `26`
- `0xff & 0b1010` => `10`

## Valid Examples

- `1+2`
- `10 - 4`
- `8*3/2`
- `7 + 8 - 9 + 10`
- `1.5 + 2.25`
- `-3`
- `2*-3`
- `.5 * 8`
- `0xff`
- `0b1010`
- `1.3e10 / 2`
- `sin(0)`
- `sind(30)`
- `pow(2, 3)`
- `pos(60, 10)`
- `lat(pos(60, 10))`
- `dist(pos(0, 0), pos(0, 1))`
- `len({1, 2, 3})`
- `product({2, 3, 4})`
- `avg({2, 4, 6})`
- `first(2, {1, 2, 3})`
- `drop(1, {1, 2, 3})`
- `list_div({8, 9}, {2, 3})`
- `list_mul({1, 2}, {3, 4})`
- `reduce({2, 3, 4}, +)`
- `map({0, 90}, sind(_))`
- `map({1, 2, 3}, _ + 1)`
- `map({1, 2, 3}, sin(_) + _)`
- `guard(1 / 0, 0)`
- `timed_loop(1 + 2, 3)`
- `fill(rand(), 3)`
- `{pos(60, 10), pos(61, 11)}`
- `rand(10, 20)`
- `range(10, 4)`
- `range(1.5, 3, 0.5)`
- `geom(2, 4)`
- `repeat(3, 4)`
- `linspace(0, 1, 5)`
- `powers(-1, 4)`
- `sin(first(1, {0}))`
- `sum({1, 2, 3})`
- `2 ^ 3`
- `10 % 3`
- `6 & 3 | 8`
- `(2 + 3) * 4`
- `8 / (2 + 2)`

## Invalid Examples

- `+1`
- `1+`
- `1++2`
- `(1+2`
-  `()`
- `sin()`
- `pow(2)`
- `sum(1)`
- `avg({})`
- `first(1.5, {1, 2})`
- `drop(1, 2)`
- `list_div({1, 2}, {3})`
- `list_mul({1, 2}, {3})`
- `reduce({}, +)`
- `map({1, 2}, sum)`
- `map({1, 2}, pow)`
- `map({1, 2}, sin)`
- `pos(100, 0)`
- `lat(1)`
- `to_poslist({60, 10, 61})`
- `dist(pos(0, 0), 1)`
- `br_to_pos(pos(0, 0), 90, -1)`
- `map({1, 2}, _ + foo)`
- `guard(1 / 0)`
- `timed_loop(1 + 2)`
- `fill(1)`
- `rand(1, 2, 3)`
- `{1, pos(60, 10)}`
- `_`
- `range(1)`
- `range(1, 2, 3, 4)`
- `range(1, -1)`
- `geom(1)`
- `repeat(1, -1)`
- `linspace(1, 2)`
- `powers(2)`
- `first(2, {1, 2, 3}) + 1`
- `1 / 0`
- `1 % 0`
- `0 ^ (1 - 2)`
- `1.5 & 1`
- `.`
- `0x`
- `0b`
- `0xg`
- `0b102`
- `1e`
- `1e+`
