#ifndef IO_H
#define IO_H

#include "dynamic_array.h"
#include "forwards.h"
#include "lexer.h"
#include "parser.h"

#include <stdbool.h>
#include <stdio.h>

// The process's streams plus reporting over them. Every write to err flushes
// out first, or a report outruns the output it is about once a stream is
// redirected. io_report_* read Lexer/Parser/VM - hence src/ root, not core/.
struct Io {
    FILE *in;
    FILE *out;
    FILE *err;

    // Reports wrap in ANSI color when set; deciding is the host's business.
    bool color;
};

// stdin/stdout/stderr, color on. io_free releases the struct alone - the
// standard streams are not this module's to close.
Io *io_alloc_std(void);
void io_free(Io *io);

void io_write(Io *io, const char *data, size_t size);

// One line from in into line (cleared first), newline dropped. False only at
// end of input with nothing read.
bool io_read_line(Io *io, CharDA *line);

void io_err_write(Io *io, const char *data, size_t size);

// To err; error and warning color when io->color is set, io_report is plain.
void io_report(Io *io, const char *fmt, ...);
void io_report_error(Io *io, const char *fmt, ...);
void io_report_warning(Io *io, const char *fmt, ...);

void io_report_lexer(Io *io, const char *path, Lexer *lexer);
void io_report_parser(Io *io, const char *path, Parser *parser);
void io_report_vm(Io *io, const char *path, VM *vm);

#endif
