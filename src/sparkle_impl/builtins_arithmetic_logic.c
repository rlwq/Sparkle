#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "builtins.h"
#include "forwards.h"
#include "object.h"
#include "string_view.h"
#include "utils.h"
#include "vm.h"

#define NUMERIC_VARIADIC_BUILTIN(func_name_, operator_)                                            \
    void func_name_(VM *vm) {                                                                      \
        vm_swap(vm);                                                                               \
                                                                                                   \
        Object *args_ = vm_prev(vm);                                                               \
        LIST_FOREACH(curr, args_) vm_push(vm, curr);                                               \
        vm_expect2(vm, TY_NUMERIC, TY_NUMERIC);                                                    \
        vm_to_common_numeric(vm);                                                                  \
                                                                                                   \
        if (vm_peek(vm)->kind == KIND_BOOL)                                                        \
            vm_build_integer(vm, (Integer)BOOL(vm_prev(vm)) operator_(Integer) BOOL(vm_peek(vm))); \
        else if (vm_peek(vm)->kind == KIND_INTEGER)                                                \
            vm_build_integer(vm, INTEGER(vm_prev(vm)) operator_ INTEGER(vm_peek(vm)));             \
        else if (vm_peek(vm)->kind == KIND_FLOAT)                                                  \
            vm_build_float(vm, FLOAT(vm_prev(vm)) operator_ FLOAT(vm_peek(vm)));                   \
                                                                                                   \
        vm_pop_prev_n(vm, 2);                                                                      \
                                                                                                   \
        END_LIST_FOREACH vm_pop_prev(vm);                                                          \
    }

#define NUMERIC_ORDER_BUILTIN(func_name_, operator_)                                               \
    void func_name_(VM *vm) {                                                                      \
        vm_expect2(vm, TY_NUMERIC, TY_NUMERIC);                                                    \
        vm_to_common_numeric(vm);                                                                  \
        switch (TYPEOF(vm_peek(vm))) {                                                             \
        case TY_BOOL:                                                                              \
            vm_build_bool(vm, BOOL(vm_prev(vm)) operator_ BOOL(vm_peek(vm)));                      \
            break;                                                                                 \
        case TY_INTEGER:                                                                           \
            vm_build_bool(vm, INTEGER(vm_prev(vm)) operator_ INTEGER(vm_peek(vm)));                \
            break;                                                                                 \
        case TY_FLOAT:                                                                             \
            vm_build_bool(vm, FLOAT(vm_prev(vm)) operator_ FLOAT(vm_peek(vm)));                    \
            break;                                                                                 \
        default:                                                                                   \
            UNREACHABLE();                                                                         \
        }                                                                                          \
        vm_pop_prev_n(vm, 2);                                                                      \
    }

// Node, Node -> Node
void rkl_eq(VM *vm) {
    bool is_equal = false;

    if (OFTYPE(vm_peek(vm), TY_NUMERIC) && OFTYPE(vm_prev(vm), TY_NUMERIC))
        vm_to_common_numeric(vm);

    if (vm_peek(vm)->kind != vm_prev(vm)->kind) {
        is_equal = false;
        goto end;
    }

    switch (vm_peek(vm)->kind) {
    case KIND_NIL:
        is_equal = true;
        break;
    case KIND_SYMBOL:
        is_equal = SYMBOL(vm_peek(vm)) == SYMBOL(vm_prev(vm));
        break;
    case KIND_INTEGER:
        is_equal = INTEGER(vm_peek(vm)) == INTEGER(vm_prev(vm));
        break;
    case KIND_FLOAT:
        is_equal = FLOAT(vm_peek(vm)) == FLOAT(vm_prev(vm));
        break;
    case KIND_BOOL:
        is_equal = BOOL(vm_peek(vm)) == BOOL(vm_prev(vm));
        break;
    case KIND_STRING:
        is_equal = STRING_SIZE(vm_peek(vm)) == STRING_SIZE(vm_prev(vm)) &&
                   memcmp(STRING_DATA(vm_peek(vm)), STRING_DATA(vm_prev(vm)),
                          STRING_SIZE(vm_peek(vm))) == 0;
        break;
    case KIND_LIST:
    case KIND_LAMBDA:
    case KIND_BUILTIN:
        is_equal = vm_peek(vm) == vm_prev(vm);
        break;
    }

end:
    vm_pop_n(vm, 2);
    vm_build_bool(vm, is_equal);
}

void rkl_ne(VM *vm) {
    rkl_eq(vm);
    vm_build_bool(vm, !BOOL(vm_peek(vm)));
    vm_pop_prev(vm);
}

NUMERIC_VARIADIC_BUILTIN(rkl_add, +)
NUMERIC_VARIADIC_BUILTIN(rkl_mul, *)
NUMERIC_VARIADIC_BUILTIN(rkl_sub, -)

NUMERIC_ORDER_BUILTIN(rkl_gt, >)
NUMERIC_ORDER_BUILTIN(rkl_ge, >=)
NUMERIC_ORDER_BUILTIN(rkl_lt, <)
NUMERIC_ORDER_BUILTIN(rkl_le, <=)

void rkl_truediv(VM *vm) {
    vm_expect2(vm, TY_NUMERIC, TY_NUMERIC);

    vm_to_common_numeric(vm);

    double v1 = 0;
    double v2 = 0;

    if (OFTYPE(vm_peek(vm), TY_INTEGER)) {
        v1 = INTEGER(vm_prev(vm));
        v2 = INTEGER(vm_peek(vm));
    }

    else if (OFTYPE(vm_peek(vm), TY_FLOAT)) {
        v1 = FLOAT(vm_prev(vm));
        v2 = FLOAT(vm_peek(vm));
    }

    vm_pop_n(vm, 2);
    vm_build_float(vm, v1 / v2);
}

void rkl_div(VM *vm) {
    vm_expect2(vm, TY_INTEGER, TY_INTEGER);
    VM_RECOVER_IF(vm, INTEGER(vm_peek(vm)) == 0, vm->singletons._VALUE_EXCEPTION);

    vm_build_integer(vm, INTEGER(vm_prev(vm)) / INTEGER(vm_peek(vm)));
    vm_pop_prev_n(vm, 2);
}

void rkl_mod(VM *vm) {
    vm_expect2(vm, TY_INTEGER, TY_INTEGER);
    VM_RECOVER_IF(vm, INTEGER(vm_peek(vm)) == 0, vm->singletons._VALUE_EXCEPTION);

    vm_build_integer(vm, INTEGER(vm_prev(vm)) % INTEGER(vm_peek(vm)));
    vm_pop_prev_n(vm, 2);
}

void rkl_eval(VM *vm) {
    vm_eval_node(vm);
}

void rkl_is_nil(VM *vm) {
    bool is_nil = vm_peek(vm)->kind == KIND_NIL;
    vm_pop(vm);
    vm_build_bool(vm, is_nil);
}

void rkl_cast_to_bool(VM *vm) {
    vm_cast_to_bool(vm);
}

// Recognises exactly the numeric literal the reader does - [+-]? digit+ with an
// optional '.' and more digits - so (int "12") accepts what the source 12
// accepts and nothing else. svtolli and svtod assert well-formed input rather
// than validating, and those asserts are gone in a release build, so nothing
// may reach them unscanned.
static bool scan_number(Object *s, bool *is_decimal) {
    const char *data = STRING_DATA(s);
    size_t size = STRING_SIZE(s);
    size_t i = 0;

    if (i < size && (data[i] == '+' || data[i] == '-'))
        i++;

    size_t digits = i;
    while (i < size && isdigit((unsigned char)data[i]))
        i++;
    if (i == digits)
        return false;

    *is_decimal = false;
    if (i < size && data[i] == '.') {
        *is_decimal = true;
        i++;
        while (i < size && isdigit((unsigned char)data[i]))
            i++;
    }

    return i == size;
}

// Node -> Integer or Float, per want. A String is read as a numeric literal, a
// Bool counts as 1 or 0, and Float to Integer truncates toward zero.
static void cast_numeric(VM *vm, ObjectKind want) {
    Object *value = vm_peek(vm);

    if (OFTYPE(value, TY_STRING)) {
        bool is_decimal = false;
        VM_RECOVER_IF(vm, !scan_number(value, &is_decimal), vm->singletons._VALUE_EXCEPTION);

        // Which parser to use follows the decimal point, exactly as the lexer
        // dispatches on TK_DECIMAL against TK_INTEGER: svtod scans for a '.'
        // without a bound and runs off the end of a view that has none.
        StringView text = sv(STRING_DATA(value), STRING_SIZE(value));
        double number = is_decimal ? svtod(text) : (double)svtolli(text);

        if (want == KIND_FLOAT)
            vm_build_float(vm, number);
        else
            vm_build_integer(vm, is_decimal ? (Integer)number : svtolli(text));

        vm_pop_prev(vm);
        return;
    }

    VM_RECOVER_IF(vm, !OFTYPE(value, TY_NUMERIC), vm->singletons._TYPE_EXCEPTION);

    if (value->kind == want)
        return;

    double number = 0;
    switch (value->kind) {
    case KIND_BOOL:
        number = BOOL(value);
        break;
    case KIND_INTEGER:
        number = (double)INTEGER(value);
        break;
    case KIND_FLOAT:
        number = FLOAT(value);
        break;
    case KIND_NIL:
    case KIND_LIST:
    case KIND_SYMBOL:
    case KIND_STRING:
    case KIND_BUILTIN:
    case KIND_LAMBDA:
        UNREACHABLE();
        break;
    }

    if (want == KIND_FLOAT)
        vm_build_float(vm, number);
    else
        vm_build_integer(vm, (Integer)number);

    vm_pop_prev(vm);
}

// Node -> Integer
void rkl_cast_to_integer(VM *vm) {
    cast_numeric(vm, KIND_INTEGER);
}

// Node -> Float
void rkl_cast_to_float(VM *vm) {
    cast_numeric(vm, KIND_FLOAT);
}

void rkl_logical_and(VM *vm) {
    bool result = true;

    Object *args_ = vm_peek(vm);
    LIST_FOREACH(curr, args_)
        vm_push(vm, curr);
        vm_cast_to_bool(vm);
        result = result && BOOL(vm_peek(vm));
        vm_pop(vm);
    END_LIST_FOREACH

    vm_pop(vm);
    vm_build_bool(vm, result);
}

void rkl_logical_or(VM *vm) {
    bool result = false;

    Object *args_ = vm_peek(vm);
    LIST_FOREACH(curr, args_)
        vm_push(vm, curr);
        vm_cast_to_bool(vm);
        result = result || BOOL(vm_peek(vm));
        vm_pop(vm);
    END_LIST_FOREACH

    vm_pop(vm);
    vm_build_bool(vm, result);
}

void rkl_logical_not(VM *vm) {
    vm_cast_to_bool(vm);
    vm_build_bool(vm, !BOOL(vm_peek(vm)));
    vm_pop_prev(vm);
}

DEFINE_MODULE(ARITHMETIC_LOGIC) = {
    {"+", rkl_add, 1, true},
    {"*", rkl_mul, 1, true},
    {"-", rkl_sub, 1, true},
    {"/", rkl_truediv, 2, false},
    {"=", rkl_eq, 2, false},
    {"!=", rkl_ne, 2, false},
    {">", rkl_gt, 2, false},
    {">=", rkl_ge, 2, false},
    {"<", rkl_lt, 2, false},
    {"<=", rkl_le, 2, false},
    {"div", rkl_div, 2, false},
    {"mod", rkl_mod, 2, false},
    {"eval", rkl_eval, 1, false},
    {"nil?", rkl_is_nil, 1, false},
    {"?", rkl_cast_to_bool, 1, false},
    {"not", rkl_logical_not, 1, false},
    {"int", rkl_cast_to_integer, 1, false},
    {"float", rkl_cast_to_float, 1, false},
    {"&&", rkl_logical_and, 0, true},
    {"||", rkl_logical_or, 0, true},
};

DEFINE_MODULE_SIZE(ARITHMETIC_LOGIC);
