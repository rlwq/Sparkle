#include "io.h"
#include "dynamic_array.h"
#include "forwards.h"
#include "object.h"

#include <assert.h>
#include <stdio.h>

static void write_bytes(CharDA *out, const char *data, size_t size) {
    for (size_t i = 0; i < size; i++)
        da_push(*out, data[i]);
}

static void write_cstr(CharDA *out, const char *s) {
    while (*s)
        da_push(*out, *s++);
}

static void write_list(VM *vm, CharDA *out, Object *expr) {
    assert(expr->kind == KIND_LIST);

    // A (quote x) list prints in its reader form 'x.
    if (OBJ_LIST_SIZE(expr) == 2 && OBJ_OFTYPE(OBJ_LIST_AT(expr, 0), TY_SYMBOL) &&
        OBJ_SYMBOL(OBJ_LIST_AT(expr, 0)) == VM_SYM(vm, quote)) {
        write_cstr(out, "'");
        write_expr(vm, out, OBJ_LIST_AT(expr, 1));
        return;
    }

    write_cstr(out, "(");
    for (size_t i = 0; i < OBJ_LIST_SIZE(expr); i++) {
        if (i > 0)
            write_cstr(out, " ");
        write_expr(vm, out, OBJ_LIST_AT(expr, i));
    }
    write_cstr(out, ")");
}

static void write_lambda(VM *vm, CharDA *out, Object *expr) {
    assert(expr->kind == KIND_LAMBDA);

    write_cstr(out, "(lambda (");
    if (OBJ_LAMBDA_ARGS(expr).size > (OBJ_LAMBDA_IS_VARIADIC(expr) ? 1 : 0))
        write_cstr(out, da_at(OBJ_LAMBDA_ARGS(expr), 0));
    for (size_t i = 1; i < OBJ_LAMBDA_POS_ARGS_N(expr); i++) {
        write_cstr(out, " ");
        write_cstr(out, da_at(OBJ_LAMBDA_ARGS(expr), i));
    }
    if (OBJ_LAMBDA_IS_VARIADIC(expr)) {
        write_cstr(out, " Var ");
        write_cstr(out, da_at_end(OBJ_LAMBDA_ARGS(expr), 0));
    }
    write_cstr(out, ") ");
    write_expr(vm, out, OBJ_LAMBDA_SUBEXPR(expr));
    write_cstr(out, ")");
}

void write_expr(VM *vm, CharDA *out, Object *expr) {
    // Large enough for %f of any double (~317 chars for DBL_MAX).
    char buf[512];

    switch (expr->kind) {
    case KIND_NIL:
        write_cstr(out, "Nil");
        break;
    case KIND_BOOL:
        write_cstr(out, OBJ_BOOL(expr) ? "True" : "False");
        break;
    case KIND_INTEGER:
        snprintf(buf, sizeof(buf), "%lld", OBJ_INTEGER(expr));
        write_cstr(out, buf);
        break;
    case KIND_FLOAT:
        snprintf(buf, sizeof(buf), "%f", OBJ_FLOAT(expr));
        write_cstr(out, buf);
        break;
    case KIND_STRING:
        write_bytes(out, OBJ_STRING_DATA(expr), OBJ_STRING_SIZE(expr));
        break;
    case KIND_SYMBOL:
        write_cstr(out, OBJ_SYMBOL(expr));
        break;
    case KIND_LIST:
        write_list(vm, out, expr);
        break;
    case KIND_BUILTIN:
        break;
    case KIND_LAMBDA:
        write_lambda(vm, out, expr);
        break;
    }
}
