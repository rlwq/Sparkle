#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "forwards.h"
#include "lexer.h"
#include "string_interner.h"
#include "string_view.h"

#include <stdbool.h>

// One evaluation session: the interner, the heap and the VM that outlive any
// single chunk of source, plus the buffers a run reuses. Source goes in through
// interp_eval as many times as you like - whatever the root scope reaches
// survives from one call to the next, which is what a REPL needs.
//
// This is pipeline wiring and nothing more. The lexer and the parser are not
// here on purpose: their state lives exactly one run, so interp_eval builds and
// drops them itself. Anything about the language rather than about running it
// belongs in lang/.
typedef struct {
    StringInterner *si;
    GC *gc;
    VM *vm;

    TokenDA tokens;
    ObjectPtrDA exprs;
} Interpreter;

// Ready to evaluate on return: the root scope exists, builtins are bound in it
// and the special forms are registered.
Interpreter *interp_alloc(void);
void interp_free(Interpreter *interp);

// Runs one chunk of source to completion - lex, parse, load, evaluate -
// reporting the first stage that fails to stderr under `name`, which is a path
// for a file and a placeholder for anything else. Returns false if any stage
// failed; the interpreter stays usable either way.
//
// src must stay alive for the duration of the call, since tokens are
// StringViews into it, but not past it: parsed objects own their own copies.
bool interp_eval(Interpreter *interp, StringView src, const char *name);

#endif
