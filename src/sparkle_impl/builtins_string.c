#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "io.h"
#include "object.h"
#include "vm.h"

// Node -> String
static void cast_to_string(VM *vm) {
    // Strings are immutable, so an existing string is shared as-is.
    if (vm_peek(vm)->kind == KIND_STRING)
        return;

    CharDA repr;
    da_init(repr);
    write_expr(&repr, vm_peek(vm));

    // The DA grows before size reaches capacity, so the buffer is always at
    // least repr.size + 1 bytes — hand it over instead of copying.
    vm_pop(vm);
    vm_build_string_own(vm, repr.data, repr.size);
}

// Any -> String
void rkl_str(VM *vm) {
    cast_to_string(vm);
}

// String -> Integer
void rkl_str_len(VM *vm) {
    vm_expect(vm, TY_STRING);

    Integer n = (Integer)STRING_SIZE(vm_peek(vm));
    vm_pop(vm);
    vm_build_integer(vm, n);
}

// String, Integer -> String
void rkl_str_get(VM *vm) {
    vm_expect2(vm, TY_STRING, TY_INTEGER);

    Object *s = vm_prev(vm);
    Integer idx = INTEGER(vm_peek(vm));
    VM_RECOVER_IF(vm, idx < 0 || (size_t)idx >= STRING_SIZE(s), vm->singletons._VALUE_EXCEPTION);

    vm_build_string(vm, STRING_DATA(s) + idx, 1);
    vm_pop_prev_n(vm, 2);
}

// String, Integer (start), Integer (length) -> String
void rkl_str_sub(VM *vm) {
    Object *s = vm_peek_at(vm, 2);
    Object *start_obj = vm_prev(vm);
    Object *len_obj = vm_peek(vm);

    VM_RECOVER_IF(vm, !OFTYPE(s, TY_STRING), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, !OFTYPE(start_obj, TY_INTEGER), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, !OFTYPE(len_obj, TY_INTEGER), vm->singletons._TYPE_EXCEPTION);

    Integer start = INTEGER(start_obj);
    Integer len = INTEGER(len_obj);
    VM_RECOVER_IF(vm,
                  start < 0 || len < 0 || (size_t)start > STRING_SIZE(s) ||
                      (size_t)len > STRING_SIZE(s) - (size_t)start,
                  vm->singletons._VALUE_EXCEPTION);

    vm_build_string(vm, STRING_DATA(s) + start, (size_t)len);
    vm_pop_prev_n(vm, 3);
}

// List of String -> String
void rkl_str_cat(VM *vm) {
    // Variadic: the args always arrive packed in a list.
    assert(OFTYPE(vm_peek(vm), TY_LIST));

    Object *args = vm_peek(vm);
    size_t total = 0;
    LIST_FOREACH(arg, args)
        VM_RECOVER_IF(vm, !OFTYPE(arg, TY_STRING), vm->singletons._TYPE_EXCEPTION);
        total += STRING_SIZE(arg);
    END_LIST_FOREACH

    char *data = malloc(total + 1);
    assert(data);
    size_t offset = 0;
    LIST_FOREACH(arg, args)
        memcpy(data + offset, STRING_DATA(arg), STRING_SIZE(arg));
        offset += STRING_SIZE(arg);
    END_LIST_FOREACH

    vm_pop(vm);
    vm_build_string_own(vm, data, total);
}

// String (haystack), String (needle) -> Integer (index of first occurrence, or -1)
void rkl_str_find(VM *vm) {
    vm_expect2(vm, TY_STRING, TY_STRING);

    Object *haystack = vm_prev(vm);
    Object *needle = vm_peek(vm);

    Integer found = -1;
    for (size_t i = 0; i + STRING_SIZE(needle) <= STRING_SIZE(haystack); i++) {
        if (memcmp(STRING_DATA(haystack) + i, STRING_DATA(needle), STRING_SIZE(needle)) == 0) {
            found = (Integer)i;
            break;
        }
    }

    vm_pop_n(vm, 2);
    vm_build_integer(vm, found);
}

// String -> Integer
void rkl_str_ord(VM *vm) {
    vm_expect(vm, TY_STRING);

    Object *s = vm_peek(vm);
    VM_RECOVER_IF(vm, STRING_SIZE(s) == 0, vm->singletons._VALUE_EXCEPTION);

    Integer code = (Integer)(unsigned char)STRING_DATA(s)[0];
    vm_pop(vm);
    vm_build_integer(vm, code);
}

// Integer -> String
void rkl_str_chr(VM *vm) {
    vm_expect(vm, TY_INTEGER);

    Integer code = INTEGER(vm_peek(vm));
    VM_RECOVER_IF(vm, code < 0 || code > 127, vm->singletons._VALUE_EXCEPTION);

    char c = (char)code;
    vm_pop(vm);
    vm_build_string(vm, &c, 1);
}

DEFINE_MODULE(STRING) = {
    {"str", rkl_str, 1, false},         {"str-len", rkl_str_len, 1, false},
    {"str-get", rkl_str_get, 2, false}, {"str-sub", rkl_str_sub, 3, false},
    {"str-cat", rkl_str_cat, 0, true},  {"str-find", rkl_str_find, 2, false},
    {"str-ord", rkl_str_ord, 1, false}, {"str-chr", rkl_str_chr, 1, false},
};

DEFINE_MODULE_SIZE(STRING);
