#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include "builtins.h"
#include "io.h"
#include "object.h"
#include "vm.h"

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

// Writes fmt into out, substituting placeholders with args[N] rendered exactly
// as str would render them. Returns false if a placeholder is malformed or
// resolves to no argument, leaving out holding the partial output for the
// caller to release.
//
// A placeholder is $N or ${N}. The braced form is the only way to put a digit
// straight after a placeholder, since the bare form reads the whole digit run
// as one index: "${0}0" is argument 0 followed by a literal 0, where "$00"
// asks for argument 0 twice over.
//
// "$$" is the escape for a literal '$'; a '$' that is followed by none of '$',
// '{' or a digit stands for itself, so a format may end in one or read "100$".
static bool format_walk(CharDA *out, Object *fmt, Object *args) {
    const char *data = STRING_DATA(fmt);
    size_t size = STRING_SIZE(fmt);
    size_t argc = LIST_SIZE(args);

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

            write_expr(out, LIST_AT(args, index));
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

        write_expr(out, LIST_AT(args, index));
        i = cursor - 1;
    }

    return true;
}

// String (fmt), List (args) -> Nil
void rkl_print(VM *vm) {
    // Variadic: everything past the format string arrives packed in a list.
    assert(OFTYPE(vm_peek(vm), TY_LIST));

    Object *fmt = vm_prev(vm);
    VM_RECOVER_IF(vm, !OFTYPE(fmt, TY_STRING), vm->singletons._TYPE_EXCEPTION);

    CharDA out;
    da_init(out);

    // vm_recover longjmps out of this frame and knows nothing about the C heap,
    // so the buffer has to be released before raising rather than after.
    if (!format_walk(&out, fmt, vm_peek(vm))) {
        da_free(out);
        vm_recover(vm, vm->singletons._VALUE_EXCEPTION);
    }

    fwrite(out.data, 1, out.size, stdout);
    fputc('\n', stdout);
    da_free(out);

    vm_pop_n(vm, 2);
    vm_build_nil(vm);
}

DEFINE_MODULE(IO) = {{"print", rkl_print, 1, true}};
DEFINE_MODULE_SIZE(IO);
