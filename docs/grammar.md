# Grammar Notes

## First Test Grammar

This initial grammar exists to exercise the parser pipeline with the smallest useful expression language.

Current scope:
- integer and floating-point numeric literals
- unary `-`
- binary `+`, `-`, `*`, `/`, `%`, `^`, `&`, `|`
- parentheses for grouping
- optional whitespace between tokens

Explicitly out of scope for this first version:
- function calls
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
  - `(` and `)`

## Grammar

```ebnf
expression = bitwise_or ;
bitwise_or = bitwise_and , { "|" , bitwise_and } ;
bitwise_and = sum , { "&" , sum } ;
sum        = term , { ( "+" | "-" ) , term } ;
term       = unary , { ( "*" | "/" | "%" ) , unary } ;
unary      = [ "-" ] , power ;
power      = primary , [ "^" , unary ] ;
primary    = number | "(" , expression , ")" ;
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

Expressions use these precedence levels, from highest to lowest: parentheses, `^`, unary `-`, `*` `/` `%`, `+` `-`, `&`, `|`. `^` is right-associative. The other operators are left-associative.

`%` uses floating-point modulo via `fmod`. `&` and `|` require integer-valued operands; non-integer operands are rejected. Division by zero, modulo by zero, and non-finite evaluation results are rejected.

Examples:
- `2 + 3` => `5`
- `2 + 3 * 4` => `14`
- `-2^2` => `-4`
- `(-2)^2` => `4`
- `2 ^ 3 ^ 2` => `512`
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
- `1 / 0`
- `1 % 0`
- `0 ^ (1 - 2)`
- `1.5 & 1`
- `.`
- `1e`
- `1e+`
