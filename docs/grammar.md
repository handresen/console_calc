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
- function calls: `sin`, `cos`, `tan`, `sind`, `cosd`, `tand`, `pow`, `sum`, `len`, `product`, `avg`, `min`, `max`, `first`, `drop`, `map`
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

## Grammar

```ebnf
expression = bitwise_or ;
bitwise_or = bitwise_and , { "|" , bitwise_and } ;
bitwise_and = sum , { "&" , sum } ;
sum        = term , { ( "+" | "-" ) , term } ;
term       = unary , { ( "*" | "/" | "%" ) , unary } ;
unary      = [ "-" ] , power ;
power      = primary , [ "^" , unary ] ;
primary    = number | function_call | map_call | list | "(" , expression , ")" ;
function_call = identifier , "(" , expression , { "," , expression } , ")" ;
map_call   = "map" , "(" , expression , "," , identifier , ")" ;
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
- decimal literals such as `3.14`, `.25`, and `10.`
- exponent literals such as `1e6`, `2.5e-3`, and `.8E+2`

## Evaluation Rule

Expressions use these precedence levels, from highest to lowest: function calls, list literals, and parentheses, `^`, unary `-`, `*` `/` `%`, `+` `-`, `&`, `|`. `^` is right-associative. The other operators are left-associative.

`%` uses floating-point modulo via `fmod`. `&` and `|` require integer-valued operands; non-integer operands are rejected. Division by zero, modulo by zero, and non-finite evaluation results are rejected.

Where a scalar is required, a one-element list is accepted and coerced to that element. Multi-element lists are still rejected in scalar positions. List literals themselves remain flat: each list element must evaluate directly to a scalar, so nested lists are still rejected.

Integer-valued decimal literals such as `42` are preserved as intrinsic integer values. Decimal literals with a fractional part or exponent such as `3.14` or `1e3` are evaluated as floating-point values.

Builtin functions:
- `sin(x)`, `cos(x)`, `tan(x)` use radians
- `sind(x)`, `cosd(x)`, `tand(x)` use degrees
- `pow(x, y)` is equivalent to `x ^ y`
- `sum(list)` sums a list value
- `len(list)` returns list length
- `product(list)` multiplies all list values; `product({})` is `1`
- `avg(list)` returns the arithmetic mean of a non-empty list
- `min(list)` and `max(list)` require non-empty lists
- `first(n, list)` returns the first `n` items as a list
- `drop(n, list)` returns the list without its first `n` items
- `map(list, func)` applies a unary scalar builtin function to each list item and returns a list of the same length

Integer-preserving behavior:
- `+`, `-`, and `*` preserve integer results when both operands are integers and the result fits in 64 bits
- `/` always produces a floating-point result
- `%` produces an integer result when both operands are integers, otherwise floating-point modulo is used
- `len(list)` returns an integer
- `sum(list)` and `product(list)` preserve integer results when all inputs remain integral
- trig functions always return floating-point results

For `first` and `drop`, `n` must be a non-negative integer. If `n` is larger than the list length, the result is clamped naturally to the list bounds. For `map`, `func` must name a unary scalar builtin such as `sin` or `cosd`; list functions and multi-argument functions are rejected.

Examples:
- `2 + 3` => `5`
- `2 + 3 * 4` => `14`
- `-2^2` => `-4`
- `(-2)^2` => `4`
- `2 ^ 3 ^ 2` => `512`
- `sin(0)` => `0`
- `sind(30)` => `0.5`
- `pow(2, 3)` => `8`
- `len({1, 2, 3})` => `3`
- `product({2, 3, 4})` => `24`
- `avg({2, 4, 6})` => `4`
- `min({2, -1, 5})` => `-1`
- `max({2, -1, 5})` => `5`
- `sum({1, 2, 3})` => `6`
- `sum(map({0, 90}, sind))` => `1`
- `first(1, {2, 3}) + 4` => `6`
- `10 % 3` => `1`
- `6 & 3 | 8` => `10`
- `(2 + 3) * 4` => `20`
- `20 / 5 - 1` => `3`
- `2 * 3 + 4 * 5` => `26`

## Valid Examples

- `1+2`
- `10 - 4`
- `8*3/2`
- `7 + 8 - 9 + 10`
- `1.5 + 2.25`
- `-3`
- `2*-3`
- `.5 * 8`
- `1.3e10 / 2`
- `sin(0)`
- `sind(30)`
- `pow(2, 3)`
- `len({1, 2, 3})`
- `product({2, 3, 4})`
- `avg({2, 4, 6})`
- `first(2, {1, 2, 3})`
- `drop(1, {1, 2, 3})`
- `map({0, 90}, sind)`
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
- `map({1, 2}, sum)`
- `map({1, 2}, pow)`
- `first(2, {1, 2, 3}) + 1`
- `1 / 0`
- `1 % 0`
- `0 ^ (1 - 2)`
- `1.5 & 1`
- `.`
- `1e`
- `1e+`
