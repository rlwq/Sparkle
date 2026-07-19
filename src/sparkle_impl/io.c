#include "io.h"
#include "dynamic_array.h"
#include "forwards.h"
#include "object.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void write_bytes(CharDA *out, const char *data, size_t size) {
    for (size_t i = 0; i < size; i++)
        da_push(*out, data[i]);
}

static void write_cstr(CharDA *out, const char *s) {
    while (*s)
        da_push(*out, *s++);
}

static void write_list(CharDA *out, Object *expr) {
    assert(expr->kind == KIND_LIST);

    // A (quote x) list prints in its reader form 'x. Compared by name, not by
    // interned pointer, since the printer has no access to the interner.
    if (LIST_SIZE(expr) == 2 && OFTYPE(LIST_AT(expr, 0), TY_SYMBOL) &&
        strcmp(SYMBOL(LIST_AT(expr, 0)), "quote") == 0) {
        write_cstr(out, "'");
        write_expr(out, LIST_AT(expr, 1));
        return;
    }

    write_cstr(out, "(");
    for (size_t i = 0; i < LIST_SIZE(expr); i++) {
        if (i > 0)
            write_cstr(out, " ");
        write_expr(out, LIST_AT(expr, i));
    }
    write_cstr(out, ")");
}

static void write_lambda(CharDA *out, Object *expr) {
    assert(expr->kind == KIND_LAMBDA);

    write_cstr(out, "(lambda (");
    if (expr->as.lambda.args.size > (expr->as.lambda.is_variadic ? 1 : 0))
        write_cstr(out, da_at(expr->as.lambda.args, 0));
    for (size_t i = 1; i < LAMBDA_POS_ARGS_N(expr); i++) {
        write_cstr(out, " ");
        write_cstr(out, da_at(expr->as.lambda.args, i));
    }
    if (expr->as.lambda.is_variadic) {
        write_cstr(out, " Var ");
        write_cstr(out, da_at_end(expr->as.lambda.args, 0));
    }
    write_cstr(out, ") ");
    write_expr(out, expr->as.lambda.subexpr);
    write_cstr(out, ")");
}

void write_expr(CharDA *out, Object *expr) {
    // Large enough for %f of any double (~317 chars for DBL_MAX).
    char buf[512];

    switch (expr->kind) {
    case KIND_NIL:
        write_cstr(out, "Nil");
        break;
    case KIND_BOOL:
        write_cstr(out, BOOL(expr) ? "True" : "False");
        break;
    case KIND_INTEGER:
        snprintf(buf, sizeof(buf), "%lld", INTEGER(expr));
        write_cstr(out, buf);
        break;
    case KIND_FLOAT:
        snprintf(buf, sizeof(buf), "%f", FLOAT(expr));
        write_cstr(out, buf);
        break;
    case KIND_STRING:
        write_bytes(out, STRING_DATA(expr), STRING_SIZE(expr));
        break;
    case KIND_SYMBOL:
        write_cstr(out, SYMBOL(expr));
        break;
    case KIND_LIST:
        write_list(out, expr);
        break;
    case KIND_BUILTIN:
        break;
    case KIND_LAMBDA:
        write_lambda(out, expr);
        break;
    }
}

void print_list(Object *expr) {
    CharDA out;
    da_init(out);
    write_list(&out, expr);
    fwrite(out.data, 1, out.size, stdout);
    da_free(out);
}

void print_expr(Object *expr) {
    CharDA out;
    da_init(out);
    write_expr(&out, expr);
    fwrite(out.data, 1, out.size, stdout);
    da_free(out);
}
