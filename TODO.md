# TODO

## Language Core

* [ ] `bool` type - some kind of unified representation of True/False values
* [ ] Variadic lambdas - `(lambda (x . rest) ...)`
* [ ] Error handling & recovery - runtime errors with catch possibilities
    * [-] Identifing sources
        * [x] Calls of uncallable objects
        * [x] Symbols with no definition
        * [x] Lambda calls with wrong arity
        * [ ] Built-in calls with wrong arity
        * [ ] Special forms with wrong arity
        * [ ] Special forms with garbage with improper arguments
    * [ ] Proper rewinding mechanism
        * [ ] Value stack with spans allocated to aspecific scope
        * [ ] Guarded scopes (`try/catch` blocks)
    * [ ] Error messaging
        * [ ] Error message information with proper error code/name

## Language features

* [ ] `'symbol` - syntactic sugar for `(quote symbol)`
* [ ] `(a . b)` - syntactic sugar for `(cons a b)`
* [ ] `set` special form - variable mutation
* [ ] `begin` special form - sequential expressions in one block

## Built-ins

* [ ] Arithmetic & comparison: `*`, `/`, `%`, `<`, `>`, `<=`, `>=`
* [ ] Lists: `list`, `length`, `append`, `map`, `filter`

## Optimization

* [ ] Tail Call Optimization

## Infrastructure

* [ ] REPL - interactive read-eval-print loop
* [ ] Macro system
* [ ] Module system - `(load "file.rkl")`

## Done
* [x] `print_cons` - crashes on empty list (`NIL`)
* [x] `print_expr` - crashes on an `cons` with its `cdr` not equal to `NIL` 
* [x] Comments - `;` to end of line


