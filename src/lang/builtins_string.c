#include "builtins.h"
#include "io.h"
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Value -> String
static void cast_to_string(VM *vm) {
    // Strings are immutable, so an existing one is shared as-is.
    if (vm_peek(vm)->kind == KIND_STRING)
        return;

    CharDA repr;
    da_init(repr);
    write_expr(vm, &repr, vm_peek(vm));

    // Handed over, not copied. da_init allocates before the first push, so the
    // buffer is malloc'd and never zero-sized, as gc_alloc_string_own requires.
    vm_pop(vm);
    vm_build_string_own(vm, repr.data, repr.size);
}

// Value -> String
static void spk_str(VM *vm) {
    cast_to_string(vm);
}

// String -> Integer
static void spk_str_len(VM *vm) {
    vm_expect(vm, TY_STRING);

    Integer n = (Integer)OBJ_STRING_SIZE(vm_peek(vm));
    vm_pop(vm);
    vm_build_integer(vm, n);
}

// String, Integer (index) -> String
static void spk_str_get(VM *vm) {
    vm_expect2(vm, TY_STRING, TY_INTEGER);

    Object *s = vm_prev(vm);
    Integer idx = OBJ_INTEGER(vm_peek(vm));
    VM_RECOVER_IF(vm, idx < 0 || (size_t)idx >= OBJ_STRING_SIZE(s),
                  vm->singletons._VALUE_EXCEPTION);

    vm_build_string(vm, OBJ_STRING_DATA(s) + idx, 1);
    vm_pop_prev_n(vm, 2);
}

// String, Integer (start), Integer (length) -> String
static void spk_str_sub(VM *vm) {
    Object *s = vm_peek_at(vm, 2);
    Object *start_obj = vm_prev(vm);
    Object *len_obj = vm_peek(vm);

    VM_RECOVER_IF(vm, !OBJ_OFTYPE(s, TY_STRING), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(start_obj, TY_INTEGER), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(len_obj, TY_INTEGER), vm->singletons._TYPE_EXCEPTION);

    Integer start = OBJ_INTEGER(start_obj);
    Integer len = OBJ_INTEGER(len_obj);
    VM_RECOVER_IF(vm,
                  start < 0 || len < 0 || (size_t)start > OBJ_STRING_SIZE(s) ||
                      (size_t)len > OBJ_STRING_SIZE(s) - (size_t)start,
                  vm->singletons._VALUE_EXCEPTION);

    vm_build_string(vm, OBJ_STRING_DATA(s) + start, (size_t)len);
    vm_pop_prev_n(vm, 3);
}

// List of String -> String   (variadic: args arrive packed)
static void spk_str_cat(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));

    Object *args = vm_peek(vm);
    size_t total = 0;
    OBJ_LIST_FOREACH(arg, args)
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(arg, TY_STRING), vm->singletons._TYPE_EXCEPTION);
        total += OBJ_STRING_SIZE(arg);
    OBJ_END_LIST_FOREACH

    char *data = malloc(total + 1);
    assert(data);
    size_t offset = 0;
    OBJ_LIST_FOREACH(arg, args)
        memcpy(data + offset, OBJ_STRING_DATA(arg), OBJ_STRING_SIZE(arg));
        offset += OBJ_STRING_SIZE(arg);
    OBJ_END_LIST_FOREACH

    vm_pop(vm);
    vm_build_string_own(vm, data, total);
}

// First occurrence of needle in haystack at or after `from`, index into *at.
// Success is a separate bool since an empty needle matches at the end too,
// leaving no in-range index free to mean "no match". Shared by str-find,
// str-split and str-replace.
static bool find_from(Object *haystack, Object *needle, size_t from, size_t *at) {
    size_t size = OBJ_STRING_SIZE(haystack);
    size_t needle_size = OBJ_STRING_SIZE(needle);

    for (size_t i = from; i + needle_size <= size; i++) {
        if (memcmp(OBJ_STRING_DATA(haystack) + i, OBJ_STRING_DATA(needle), needle_size) == 0) {
            *at = i;
            return true;
        }
    }

    return false;
}

// String (haystack), String (needle) -> Integer (index, or -1)
static void spk_str_find(VM *vm) {
    vm_expect2(vm, TY_STRING, TY_STRING);

    size_t at = 0;
    bool found = find_from(vm_prev(vm), vm_peek(vm), 0, &at);

    vm_pop_n(vm, 2);
    vm_build_integer(vm, found ? (Integer)at : -1);
}

// String -> Integer
static void spk_str_ord(VM *vm) {
    vm_expect(vm, TY_STRING);

    Object *s = vm_peek(vm);
    VM_RECOVER_IF(vm, OBJ_STRING_SIZE(s) == 0, vm->singletons._VALUE_EXCEPTION);

    Integer code = (Integer)(unsigned char)OBJ_STRING_DATA(s)[0];
    vm_pop(vm);
    vm_build_integer(vm, code);
}

// Integer -> String
static void spk_str_chr(VM *vm) {
    vm_expect(vm, TY_INTEGER);

    Integer code = OBJ_INTEGER(vm_peek(vm));
    VM_RECOVER_IF(vm, code < 0 || code > 127, vm->singletons._VALUE_EXCEPTION);

    char c = (char)code;
    vm_pop(vm);
    vm_build_string(vm, &c, 1);
}

// String, String (separator) -> List of String
static void spk_str_split(VM *vm) {
    vm_expect2(vm, TY_STRING, TY_STRING);
    VM_RECOVER_IF(vm, OBJ_STRING_SIZE(vm_peek(vm)) == 0, vm->singletons._VALUE_EXCEPTION);

    Object *s = vm_prev(vm);
    Object *sep = vm_peek(vm);

    // s and sep stay below the pieces on the stack, so they are rooted while
    // building each one collects.
    size_t pieces = 0;
    size_t from = 0;
    size_t at = 0;

    while (find_from(s, sep, from, &at)) {
        vm_build_string(vm, OBJ_STRING_DATA(s) + from, at - from);
        pieces++;
        from = at + OBJ_STRING_SIZE(sep);
    }

    vm_build_string(vm, OBJ_STRING_DATA(s) + from, OBJ_STRING_SIZE(s) - from);
    pieces++;

    vm_pack_list(vm, pieces);
    vm_pop_prev_n(vm, 2);
}

// List of String, String (separator) -> String
static void spk_str_join(VM *vm) {
    vm_expect2(vm, TY_LIST, TY_STRING);

    Object *parts = vm_prev(vm);
    Object *sep = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(parts);

    size_t total = 0;
    OBJ_LIST_FOREACH(part, parts)
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(part, TY_STRING), vm->singletons._TYPE_EXCEPTION);
        total += OBJ_STRING_SIZE(part);
    OBJ_END_LIST_FOREACH

    if (n > 1)
        total += (n - 1) * OBJ_STRING_SIZE(sep);

    char *data = malloc(total + 1);
    assert(data);

    size_t offset = 0;
    for (size_t i = 0; i < n; i++) {
        if (i > 0) {
            memcpy(data + offset, OBJ_STRING_DATA(sep), OBJ_STRING_SIZE(sep));
            offset += OBJ_STRING_SIZE(sep);
        }
        Object *part = OBJ_LIST_AT(parts, i);
        memcpy(data + offset, OBJ_STRING_DATA(part), OBJ_STRING_SIZE(part));
        offset += OBJ_STRING_SIZE(part);
    }

    vm_pop_n(vm, 2);
    vm_build_string_own(vm, data, total);
}

// String, String (target), String (replacement) -> String
static void spk_str_replace(VM *vm) {
    Object *s = vm_peek_at(vm, 2);
    Object *target = vm_prev(vm);
    Object *replacement = vm_peek(vm);

    VM_RECOVER_IF(vm, !OBJ_OFTYPE(s, TY_STRING), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(target, TY_STRING), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(replacement, TY_STRING), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, OBJ_STRING_SIZE(target) == 0, vm->singletons._VALUE_EXCEPTION);

    // Counted first so the buffer is sized exactly, since the replacement may
    // be longer or shorter than what it stands in for.
    size_t hits = 0;
    size_t from = 0;
    size_t at = 0;
    while (find_from(s, target, from, &at)) {
        hits++;
        from = at + OBJ_STRING_SIZE(target);
    }

    size_t total =
        OBJ_STRING_SIZE(s) + hits * OBJ_STRING_SIZE(replacement) - hits * OBJ_STRING_SIZE(target);
    char *data = malloc(total + 1);
    assert(data);

    size_t offset = 0;
    from = 0;
    while (find_from(s, target, from, &at)) {
        memcpy(data + offset, OBJ_STRING_DATA(s) + from, at - from);
        offset += at - from;
        memcpy(data + offset, OBJ_STRING_DATA(replacement), OBJ_STRING_SIZE(replacement));
        offset += OBJ_STRING_SIZE(replacement);
        from = at + OBJ_STRING_SIZE(target);
    }
    memcpy(data + offset, OBJ_STRING_DATA(s) + from, OBJ_STRING_SIZE(s) - from);
    offset += OBJ_STRING_SIZE(s) - from;

    vm_pop_n(vm, 3);
    vm_build_string_own(vm, data, offset);
}

// String -> String
static void spk_str_trim(VM *vm) {
    vm_expect(vm, TY_STRING);

    Object *s = vm_peek(vm);
    const char *data = OBJ_STRING_DATA(s);
    size_t size = OBJ_STRING_SIZE(s);

    size_t start = 0;
    while (start < size && isspace((unsigned char)data[start]))
        start++;

    size_t end = size;
    while (end > start && isspace((unsigned char)data[end - 1]))
        end--;

    vm_build_string(vm, data + start, end - start);
    vm_pop_prev(vm);
}

// String -> String
#define STRING_CASE_BUILTIN(func_name_, convert_)                                                  \
    static void func_name_(VM *vm) {                                                               \
        vm_expect(vm, TY_STRING);                                                                  \
                                                                                                   \
        Object *s = vm_peek(vm);                                                                   \
        size_t size = OBJ_STRING_SIZE(s);                                                          \
                                                                                                   \
        char *data = malloc(size + 1);                                                             \
        assert(data);                                                                              \
        for (size_t i = 0; i < size; i++)                                                          \
            data[i] = (char)convert_((unsigned char)OBJ_STRING_DATA(s)[i]);                        \
                                                                                                   \
        vm_pop(vm);                                                                                \
        vm_build_string_own(vm, data, size);                                                       \
    }

STRING_CASE_BUILTIN(spk_str_upper, toupper)
STRING_CASE_BUILTIN(spk_str_lower, tolower)

DEFINE_MODULE(STRING) = {
    {"str", spk_str, 1, false},
    {"str-len", spk_str_len, 1, false},
    {"str-get", spk_str_get, 2, false},
    {"str-sub", spk_str_sub, 3, false},
    {"str-cat", spk_str_cat, 0, true},
    {"str-find", spk_str_find, 2, false},
    {"str-ord", spk_str_ord, 1, false},
    {"str-split", spk_str_split, 2, false},
    {"str-join", spk_str_join, 2, false},
    {"str-replace", spk_str_replace, 3, false},
    {"str-trim", spk_str_trim, 1, false},
    {"str-upper", spk_str_upper, 1, false},
    {"str-lower", spk_str_lower, 1, false},
    {"str-chr", spk_str_chr, 1, false},
};

DEFINE_MODULE_SIZE(STRING);
