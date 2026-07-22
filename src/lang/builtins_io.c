#include "builtins.h"
#include "io.h"
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

// A format string carries arbitrary bytes, so every char reaching isdigit is
// cast through unsigned char: on a signed-char build the high bytes of UTF-8
// text are negative, which is outside the domain ctype defines.
#define IS_DIGIT(c_) (isdigit((unsigned char)(c_)))

// Reads the decimal run at data[*cursor], leaving *cursor one past it. Fails on
// an empty run or one that would overflow size_t.
static bool read_index(const char *data, size_t size, size_t *cursor, size_t *index) {
    size_t start = *cursor;
    size_t value = 0;

    for (; *cursor < size && IS_DIGIT(data[*cursor]); (*cursor)++) {
        size_t digit = (size_t)(data[*cursor] - '0');
        if (value > (SIZE_MAX - digit) / 10)
            return false;
        value = value * 10 + digit;
    }

    if (*cursor == start)
        return false;

    *index = value;
    return true;
}

// Writes fmt into out with its placeholders substituted. Returns false on a
// malformed placeholder, leaving out holding the partial output for the caller
// to release.
static bool format_walk(VM *vm, CharDA *out, Object *fmt, Object *args) {
    const char *data = OBJ_STRING_DATA(fmt);
    size_t size = OBJ_STRING_SIZE(fmt);
    size_t argc = OBJ_LIST_SIZE(args);

    for (size_t i = 0; i < size; i++) {
        if (data[i] != '$') {
            da_push(*out, data[i]);
            continue;
        }

        if (i + 1 < size && data[i + 1] == '$') {
            da_push(*out, '$');
            i++;
            continue;
        }

        size_t cursor;
        size_t index;

        if (i + 1 < size && data[i + 1] == '{') {
            cursor = i + 2;
            if (!read_index(data, size, &cursor, &index))
                return false;
            if (cursor >= size || data[cursor] != '}')
                return false;
            if (index >= argc)
                return false;

            write_expr(vm, out, OBJ_LIST_AT(args, index));
            i = cursor;
            continue;
        }

        if (i + 1 >= size || !IS_DIGIT(data[i + 1])) {
            da_push(*out, '$');
            continue;
        }

        cursor = i + 1;
        if (!read_index(data, size, &cursor, &index) || index >= argc)
            return false;

        write_expr(vm, out, OBJ_LIST_AT(args, index));
        i = cursor - 1;
    }

    return true;
}

// String (fmt), List (args) -> Nil   (variadic: args arrive packed)
static void spk_print(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));

    Object *fmt = vm_prev(vm);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(fmt, TY_STRING), vm->singletons._TYPE_EXCEPTION);

    CharDA out;
    da_init(out);

    // vm_recover longjmps out of this frame and knows nothing about the C heap,
    // so the buffer has to be released before raising rather than after.
    if (!format_walk(vm, &out, fmt, vm_peek(vm))) {
        da_free(out);
        vm_recover(vm, vm->singletons._VALUE_EXCEPTION);
    }

    fwrite(out.data, 1, out.size, stdout);
    fputc('\n', stdout);
    da_free(out);

    vm_pop_n(vm, 2);
    vm_build_nil(vm);
}

// -> String | Nil
static void spk_input(VM *vm) {
    CharDA line;
    da_init(line);

    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n')
        da_push(line, (char)c);

    if (c == EOF && line.size == 0) {
        da_free(line);
        vm_build_nil(vm);
        return;
    }

    // Strings are a data/size pair rather than a C string, so the buffer can go
    // over as it is instead of being copied.
    vm_build_string_own(vm, line.data, line.size);
}

DEFINE_MODULE(IO) = {{"print", spk_print, 1, true}, {"input", spk_input, 0, false}};
DEFINE_MODULE_SIZE(IO);
