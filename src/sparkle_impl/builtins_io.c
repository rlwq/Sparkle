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

// Writes fmt into out, substituting $N placeholders with args[N] rendered
// exactly as str would render them. Returns false if some placeholder resolves
// to no argument, leaving out holding the partial output for the caller to
// release.
//
// "$$" is the escape for a literal '$'; a '$' that is followed by neither '$'
// nor a digit stands for itself, so a format may end in one or read "100$".
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

        if (i + 1 >= size || !IS_DIGIT(data[i + 1])) {
            da_push(*out, '$');
            continue;
        }

        size_t index = 0;
        bool overflows = false;
        size_t end = i + 1;
        for (; end < size && IS_DIGIT(data[end]); end++) {
            size_t digit = (size_t)(data[end] - '0');
            if (index > (SIZE_MAX - digit) / 10)
                overflows = true;
            else
                index = index * 10 + digit;
        }

        if (overflows || index >= argc)
            return false;

        write_expr(out, LIST_AT(args, index));
        i = end - 1;
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
