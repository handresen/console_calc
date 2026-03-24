# Basic User Functions Plan

## Goal

Status on `main`: fixed-arity user-defined functions are now implemented.
This document remains as the design note for the current behavior and the next
extensions.

Add explicit user-defined functions with calculator-friendly syntax such as:

```text
f(x): x + 1
p(x): x % 3 = 1
pair_sum(x, y): x + y
```

Initial usage should work in normal expressions and inside existing list forms:

```text
f(3)
pair_sum(2, 5)
map(vals, f(_))
list_where(vals, p(_))
```

This extends the current late-bound definition model without introducing full
lambda syntax or function-valued expressions yet.

## Scope For First Version

Keep the first implementation intentionally narrow:

- fixed-arity user-defined functions
- syntax `name(param[, param...]): expr`
- function calls in ordinary expressions, `map(...)`, `map_at(...)`, and `list_where(...)`
- late-bound resolution, matching current definition behavior
- no recursive function bodies
- no overloading
- no anonymous functions / `=>` syntax
- no function values as first-class data

This gives most of the practical value while avoiding a large parser/runtime
jump.

## Design Direction

Use two explicit definition kinds in the runtime layer:

- value definitions: `x: pi + 1`
- function definitions: `f(x): x + 1`, `pair_sum(x, y): x + y`

Do not overload `name: expr` to implicitly become a function because the body
contains `_`. That would be ambiguous and would mix two different concepts
under one syntax.

Keep `_` reserved for existing list-special-form placeholder usage.
User-defined functions should use named parameters such as `x`.

## Expected Semantics

- function definitions are stored late-bound, like existing value definitions
- calling `f(expr)` or `pair_sum(a, b)` substitutes or binds the arguments into the stored body
  at evaluation time
- redefining a function replaces the previous definition of that function name
- builtin function names and builtin constants remain reserved
- recursive function bodies should be rejected in v1 to keep expansion and
  diagnostics simple
- nested calls in arguments such as `f(f(3))` should remain allowed
- calling an unknown user function remains an error

## Implementation Outline

1. Extend assignment parsing in `apps/` to recognize `name(param[, param...]): expr`
   alongside `name: expr`.
2. Refactor runtime definition storage to represent typed definitions rather
   than only raw expression aliases.
3. Extend identifier and call expansion so user function calls are
   recognized in a token-aware way.
4. Add safe parameter substitution or binding for fixed-arity user-defined calls.
5. Keep cycle detection for value definitions and recursive function bodies,
   while still allowing nested function calls in arguments.
6. Update listings so `vars` and related definition views can show both value
   and function definitions clearly.
7. Add console, conformance, and README examples for the new syntax.

## Refactoring Notes

This feature benefits from a small refactor first:

- separate value-definition storage from future function-definition storage
- generalize app-layer expansion terminology from map-only placeholders to bound
  argument contexts
- keep substitution token-aware rather than doing naive text replacement

That refactor is useful and should not be reverted if user functions move
forward.

## Deferred Ideas

These are deliberately out of scope for the first pass:

- anonymous lambdas such as `x => x + 1`
- recursive function bodies
- higher-order functions or passing functions as values
- closures or captured locals
- general program control constructs

Those can be revisited later if the unary-function model proves useful.
