# Grammar Notes

## First Test Grammar

This initial grammar exists to exercise the parser pipeline with the smallest useful expression language.

Current scope:
- integer and floating-point numeric literals
- binary `+`, `-`, `*`, `/`
- optional whitespace between tokens

Explicitly out of scope for this first version:
- unary operators
- parentheses
- function calls
- variables or constants

## Tokens

- `number`
  - an integer or floating-point literal
  - examples: `0`, `7`, `123`, `3.14`, `.5`, `1.`, `1.3e10`, `6E-4`
- `operator`
  - one of `+`, `-`, `*`, `/`

## Grammar

```ebnf
expression = term , { ( "+" | "-" ) , term } ;
term       = number , { ( "*" | "/" ) , number } ;
number     = mantissa , [ exponent ] ;
mantissa   = digits , [ "." , [ digits ] ]
           | "." , digits ;
exponent   = ( "e" | "E" ) , [ "+" | "-" ] , digits ;
digits     = digit , { digit } ;
digit      = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
operator   = "+" | "-" | "*" | "/" ;
```

Accepted numeric forms include:
- integer literals such as `42`
- decimal literals such as `3.14`, `.25`, and `10.`
- exponent literals such as `1e6`, `2.5e-3`, and `.8E+2`

## Evaluation Rule

Expressions use standard arithmetic precedence: `*` and `/` bind more tightly than `+` and `-`. Operators of the same precedence group are evaluated from left to right.

Examples:
- `2 + 3` => `5`
- `2 + 3 * 4` => `14`
- `20 / 5 - 1` => `3`
- `2 * 3 + 4 * 5` => `26`

## Valid Examples

- `1+2`
- `10 - 4`
- `8*3/2`
- `7 + 8 - 9 + 10`
- `1.5 + 2.25`
- `.5 * 8`
- `1.3e10 / 2`

## Invalid Examples

- `+1`
- `1+`
- `1++2`
- `-3`
- `(1+2)`
- `2*-3`
- `.`
- `1e`
- `1e+`
