#ifndef REPL_H
#define REPL_H

#include "interpreter.h"
// For CharDA. DA(char) written out again would be a distinct anonymous struct,
// not the same type.
#include "io.h"

// An interactive session over one Interpreter: read a line, evaluate it, print
// what it came to, repeat until end of input. Bindings accumulate, because the
// interpreter's root scope outlives any single line.
//
// One line is one chunk of source, so a line holding several expressions runs
// all of them and an unbalanced line is an ordinary parse error - the same one
// a file would get. There is no multi-line continuation.
//
// The program's standard input is the session's standard input, so `(input)`
// consumes the next line the user was about to type. That is the honest
// consequence of having one stream, and it is left as is: reading commands from
// the terminal separately would mean /dev/tty, and the language is meant to
// build with nothing but ISO C11.
typedef struct {
    Interpreter *interp;

    // The current line's bytes. Tokens are StringViews into this, so it has to
    // outlive interp_eval - which it does, being reused rather than freed.
    CharDA line;
} Repl;

Repl *repl_alloc(void);
void repl_free(Repl *repl);

// Runs until end of input. Prompts go to stderr, so stdout carries only what
// the session produced and a piped session stays machine-readable.
void repl_run(Repl *repl);

#endif
