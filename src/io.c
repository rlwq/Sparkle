#include "io.h"
#include "string_view.h"
#include "vm.h"
#include "write.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

Io *io_alloc_std(void) {
    Io *io = malloc(sizeof(Io));
    assert(io);

    io->in = stdin;
    io->out = stdout;
    io->err = stderr;
    io->color = true;

    return io;
}

void io_free(Io *io) {
    free(io);
}

void io_write(Io *io, const char *data, size_t size) {
    fwrite(data, 1, size, io->out);
}

bool io_read_line(Io *io, CharDA *line) {
    line->size = 0;

    int c;
    while ((c = fgetc(io->in)) != EOF && c != '\n')
        da_push(*line, (char)c);

    return c != EOF || line->size > 0;
}

void io_err_write(Io *io, const char *data, size_t size) {
    fflush(io->out);
    fwrite(data, 1, size, io->err);
}

static void io_err_vprintf(Io *io, const char *fmt, va_list args) {
    fflush(io->out);
    vfprintf(io->err, fmt, args);
}

// color is an ANSI sequence to wrap the message in, or NULL for plain.
static void io_report_v(Io *io, const char *color, const char *fmt, va_list args) {
    if (color && io->color)
        fputs(color, io->err);

    io_err_vprintf(io, fmt, args);

    if (color && io->color)
        fputs(RESET, io->err);
}

#define IO_REPORT_FUNC(func_name_, color_)                                                         \
    void func_name_(Io *io, const char *fmt, ...) {                                                \
        va_list args;                                                                              \
        va_start(args, fmt);                                                                       \
        io_report_v(io, (color_), fmt, args);                                                      \
        va_end(args);                                                                              \
    }

IO_REPORT_FUNC(io_report, NULL)
IO_REPORT_FUNC(io_report_error, RED)
IO_REPORT_FUNC(io_report_warning, YELLOW)

void io_report_lexer(Io *io, const char *path, Lexer *lexer) {
    assert(lexer->is_err);

    io_report_error(io, "%s:%zu:%zu: [PARSE ERROR] Unexpected character: " SV_FMT "\n", path,
                    lexer->pos.line + 1, lexer->pos.column + 1,
                    SV_ARGS(sv_take(lexer->src, sv_find(lexer->src, '\n'))));
}

void io_report_parser(Io *io, const char *path, Parser *parser) {
    (void)parser;
    assert(parser->is_err);

    io_report_error(io, "%s: [PARSE ERROR] Invalid or incomplete expression.\n", path);
}

void io_report_vm(Io *io, const char *path, VM *vm) {
    assert(vm->is_err);

    // A value-carrying exception is an Exception object; its whole point is the
    // detail it holds, so render it as the Kind: Value that write_expr gives and
    // let it stand in for any fixed message.
    if (OBJ_OFTYPE(vm->exception, TY_EXCEPTION)) {
        CharDA out;
        da_init(out);
        write_expr(vm, &out, vm->exception);
        io_report_error(io, "%s: [RUNTIME ERROR] %.*s\n", path, (int)out.size, out.data);
        da_free(out);
        return;
    }

    // A value-less exception is a bare kind Symbol. Compared by interned name
    // rather than by object: a kind that reached here through throw is a fresh
    // symbol, since capitalized symbols self-evaluate into one, and would match
    // no singleton by identity.
    StringName kind = OBJ_SYMBOL(vm_exception_kind(vm));
    const char *message = NULL;
#define X(name_, msg_)                                                                             \
    if (kind == OBJ_SYMBOL(vm->singletons._##name_))                                               \
        message = (msg_);
    X_RUNTIME_EXCEPTIONS
#undef X

    // A kind the language does not define is one the program invented, so the
    // name it chose is the whole of what can be said about it.
    if (message)
        io_report_error(io, "%s: [RUNTIME ERROR] %s\n", path, message);
    else
        io_report_error(io, "%s: [RUNTIME ERROR] %s raised.\n", path, kind);
}
