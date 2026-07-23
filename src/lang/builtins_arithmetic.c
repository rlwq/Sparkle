#include "builtins.h"
#include "object.h"
#include "string_view.h"
#include "vm.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

// Numeric, List of Numeric -> Integer | Float   (variadic: args arrive packed)
#define NUMERIC_VARIADIC_BUILTIN(func_name_, operator_)                                            \
    static void func_name_(VM *vm) {                                                               \
        assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));                                                  \
        vm_swap(vm);                                                                               \
                                                                                                   \
        Object *args_ = vm_prev(vm);                                                               \
        OBJ_LIST_FOREACH(curr, args_) vm_push(vm, curr);                                           \
        vm_expect2(vm, TY_NUMERIC, TY_NUMERIC);                                                    \
        vm_to_common_numeric(vm);                                                                  \
                                                                                                   \
        if (vm_peek(vm)->kind == KIND_BOOL)                                                        \
            vm_build_integer(vm, (Integer)OBJ_BOOL(vm_prev(vm)) operator_(Integer)                 \
                                     OBJ_BOOL(vm_peek(vm)));                                       \
        else if (vm_peek(vm)->kind == KIND_INTEGER)                                                \
            vm_build_integer(vm, OBJ_INTEGER(vm_prev(vm)) operator_ OBJ_INTEGER(vm_peek(vm)));     \
        else if (vm_peek(vm)->kind == KIND_FLOAT)                                                  \
            vm_build_float(vm, OBJ_FLOAT(vm_prev(vm)) operator_ OBJ_FLOAT(vm_peek(vm)));           \
                                                                                                   \
        vm_pop_prev_n(vm, 2);                                                                      \
                                                                                                   \
        OBJ_END_LIST_FOREACH vm_pop_prev(vm);                                                      \
    }

NUMERIC_VARIADIC_BUILTIN(spk_add, +)
NUMERIC_VARIADIC_BUILTIN(spk_mul, *)
NUMERIC_VARIADIC_BUILTIN(spk_sub, -)

// Numeric (dividend), Numeric (divisor) -> Float
static void spk_truediv(VM *vm) {
    vm_expect2(vm, TY_NUMERIC, TY_NUMERIC);

    vm_to_common_numeric(vm);

    double v1 = 0;
    double v2 = 0;

    if (OBJ_OFTYPE(vm_peek(vm), TY_INTEGER)) {
        v1 = OBJ_INTEGER(vm_prev(vm));
        v2 = OBJ_INTEGER(vm_peek(vm));
    }

    else if (OBJ_OFTYPE(vm_peek(vm), TY_FLOAT)) {
        v1 = OBJ_FLOAT(vm_prev(vm));
        v2 = OBJ_FLOAT(vm_peek(vm));
    }

    vm_pop_n(vm, 2);
    vm_build_float(vm, v1 / v2);
}

// Integer (dividend), Integer (divisor) -> Integer (quotient)
static void spk_div(VM *vm) {
    vm_expect2(vm, TY_INTEGER, TY_INTEGER);
    VM_RECOVER_IF(vm, OBJ_INTEGER(vm_peek(vm)) == 0, vm->singletons._VALUE_EXCEPTION);

    vm_build_integer(vm, OBJ_INTEGER(vm_prev(vm)) / OBJ_INTEGER(vm_peek(vm)));
    vm_pop_prev_n(vm, 2);
}

// Integer (dividend), Integer (divisor) -> Integer (remainder)
static void spk_mod(VM *vm) {
    vm_expect2(vm, TY_INTEGER, TY_INTEGER);
    VM_RECOVER_IF(vm, OBJ_INTEGER(vm_peek(vm)) == 0, vm->singletons._VALUE_EXCEPTION);

    vm_build_integer(vm, OBJ_INTEGER(vm_prev(vm)) % OBJ_INTEGER(vm_peek(vm)));
    vm_pop_prev_n(vm, 2);
}

// Reuses the lexer's scanner and requires it to consume the whole string, so
// trailing characters are rejected.
static bool scan_number(Object *s, bool *is_decimal) {
    size_t length = 0;
    SvNumber kind = sv_scan_number(sv(OBJ_STRING_DATA(s), OBJ_STRING_SIZE(s)), &length);

    *is_decimal = kind == SV_NUMBER_FLOAT;
    return kind != SV_NUMBER_NONE && length == OBJ_STRING_SIZE(s);
}

// Value -> Integer | Float
static void cast_numeric(VM *vm, ObjectKind want) {
    Object *value = vm_peek(vm);

    if (OBJ_OFTYPE(value, TY_STRING)) {
        bool is_decimal = false;
        VM_RECOVER_IF(vm, !scan_number(value, &is_decimal), vm->singletons._VALUE_EXCEPTION);

        // svtod scans for a '.' without a bound, so an integer literal has to go
        // through svtolli rather than being parsed as a float and cast.
        StringView text = sv(OBJ_STRING_DATA(value), OBJ_STRING_SIZE(value));
        double number = is_decimal ? svtod(text) : (double)svtolli(text);

        if (want == KIND_FLOAT)
            vm_build_float(vm, number);
        else
            vm_build_integer(vm, is_decimal ? (Integer)number : svtolli(text));

        vm_pop_prev(vm);
        return;
    }

    VM_RECOVER_IF(vm, !OBJ_OFTYPE(value, TY_NUMERIC), vm->singletons._TYPE_EXCEPTION);

    if (value->kind == want)
        return;

    double number = 0;
    switch (value->kind) {
    case KIND_BOOL:
        number = OBJ_BOOL(value);
        break;
    case KIND_INTEGER:
        number = (double)OBJ_INTEGER(value);
        break;
    case KIND_FLOAT:
        number = OBJ_FLOAT(value);
        break;
    case KIND_NIL:
    case KIND_LIST:
    case KIND_SYMBOL:
    case KIND_STRING:
    case KIND_BUILTIN:
    case KIND_LAMBDA:
    case KIND_EXCEPTION:
        assert(false && "UNREACHABLE");
        break;
    }

    if (want == KIND_FLOAT)
        vm_build_float(vm, number);
    else
        vm_build_integer(vm, (Integer)number);

    vm_pop_prev(vm);
}

// Reads a numeric argument as a double, for the operations whose result is a
// Float no matter what came in.
static double numeric_as_double(Object *value) {
    switch (value->kind) {
    case KIND_BOOL:
        return OBJ_BOOL(value);
    case KIND_INTEGER:
        return (double)OBJ_INTEGER(value);
    case KIND_FLOAT:
        return OBJ_FLOAT(value);
    case KIND_NIL:
    case KIND_LIST:
    case KIND_SYMBOL:
    case KIND_STRING:
    case KIND_BUILTIN:
    case KIND_LAMBDA:
    case KIND_EXCEPTION:
        assert(false && "UNREACHABLE");
    }
    return 0;
}

// Numeric -> Integer | Float
static void spk_neg(VM *vm) {
    vm_expect(vm, TY_NUMERIC);

    Object *value = vm_peek(vm);
    if (OBJ_OFTYPE(value, TY_FLOAT))
        vm_build_float(vm, -OBJ_FLOAT(value));
    else if (OBJ_OFTYPE(value, TY_INTEGER))
        vm_build_integer(vm, -OBJ_INTEGER(value));
    else
        vm_build_integer(vm, -(Integer)OBJ_BOOL(value));

    vm_pop_prev(vm);
}

// Numeric -> Integer | Float
static void spk_abs(VM *vm) {
    vm_expect(vm, TY_NUMERIC);

    Object *value = vm_peek(vm);
    if (OBJ_OFTYPE(value, TY_FLOAT))
        vm_build_float(vm, fabs(OBJ_FLOAT(value)));
    else if (OBJ_OFTYPE(value, TY_INTEGER))
        vm_build_integer(vm, OBJ_INTEGER(value) < 0 ? -OBJ_INTEGER(value) : OBJ_INTEGER(value));
    else
        vm_build_integer(vm, OBJ_BOOL(value));

    vm_pop_prev(vm);
}

// List of Numeric -> Numeric   (variadic: args arrive packed)
static void extremum(VM *vm, bool want_greater) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));

    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);
    VM_RECOVER_IF(vm, n == 0, vm->singletons._ARITY_EXCEPTION);

    OBJ_LIST_FOREACH(arg, args)
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(arg, TY_NUMERIC), vm->singletons._TYPE_EXCEPTION);
    OBJ_END_LIST_FOREACH

    // The winner stays the original object, so its kind survives instead of widening.
    size_t best = 0;
    for (size_t i = 1; i < n; i++) {
        double candidate = numeric_as_double(OBJ_LIST_AT(args, i));
        double current = numeric_as_double(OBJ_LIST_AT(args, best));
        if (want_greater ? candidate > current : candidate < current)
            best = i;
    }

    vm_push(vm, OBJ_LIST_AT(args, best));
    vm_pop_prev(vm);
}

// List of Numeric -> Numeric
static void spk_min(VM *vm) {
    extremum(vm, false);
}

// List of Numeric -> Numeric
static void spk_max(VM *vm) {
    extremum(vm, true);
}

// Numeric -> Float
#define FLOAT_RESULT_BUILTIN(func_name_, expr_)                                                    \
    static void func_name_(VM *vm) {                                                               \
        vm_expect(vm, TY_NUMERIC);                                                                 \
        double x = numeric_as_double(vm_peek(vm));                                                 \
        (void)x;                                                                                   \
        vm_build_float(vm, (expr_));                                                               \
        vm_pop_prev(vm);                                                                           \
    }

FLOAT_RESULT_BUILTIN(spk_sqrt_impl, sqrt(x))

// Numeric -> Float
static void spk_sqrt(VM *vm) {
    vm_expect(vm, TY_NUMERIC);
    VM_RECOVER_IF(vm, numeric_as_double(vm_peek(vm)) < 0, vm->singletons._VALUE_EXCEPTION);
    spk_sqrt_impl(vm);
}

// Numeric (base), Numeric (exponent) -> Float
static void spk_pow(VM *vm) {
    vm_expect2(vm, TY_NUMERIC, TY_NUMERIC);

    double base = numeric_as_double(vm_prev(vm));
    double exponent = numeric_as_double(vm_peek(vm));

    vm_build_float(vm, pow(base, exponent));
    vm_pop_prev_n(vm, 2);
}

// Numeric -> Integer
#define ROUNDING_BUILTIN(func_name_, round_)                                                       \
    static void func_name_(VM *vm) {                                                               \
        vm_expect(vm, TY_NUMERIC);                                                                 \
        vm_build_integer(vm, (Integer)round_(numeric_as_double(vm_peek(vm))));                     \
        vm_pop_prev(vm);                                                                           \
    }

ROUNDING_BUILTIN(spk_floor, floor)
ROUNDING_BUILTIN(spk_ceil, ceil)
ROUNDING_BUILTIN(spk_round, round)

// Value -> Integer
static void spk_cast_to_integer(VM *vm) {
    cast_numeric(vm, KIND_INTEGER);
}

// Value -> Float
static void spk_cast_to_float(VM *vm) {
    cast_numeric(vm, KIND_FLOAT);
}

DEFINE_MODULE(ARITHMETIC) = {
    {"+", spk_add, 1, true},
    {"*", spk_mul, 1, true},
    {"-", spk_sub, 1, true},
    {"/", spk_truediv, 2, false},
    {"div", spk_div, 2, false},
    {"mod", spk_mod, 2, false},
    {"int", spk_cast_to_integer, 1, false},
    {"float", spk_cast_to_float, 1, false},
    {"neg", spk_neg, 1, false},
    {"abs", spk_abs, 1, false},
    {"min", spk_min, 0, true},
    {"max", spk_max, 0, true},
    {"pow", spk_pow, 2, false},
    {"sqrt", spk_sqrt, 1, false},
    {"floor", spk_floor, 1, false},
    {"ceil", spk_ceil, 1, false},
    {"round", spk_round, 1, false},
};

DEFINE_MODULE_SIZE(ARITHMETIC);
