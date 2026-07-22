#include "interpreter.h"

#include "builtins.h"
#include "dynamic_array.h"
#include "gc.h"
#include "io.h"
#include "parser.h"
#include "special_forms.h"
#include "vm.h"
#include "write.h"

#include <assert.h>
#include <stdlib.h>

Interpreter *interp_alloc(void) {
    Interpreter *interp = malloc(sizeof(Interpreter));
    assert(interp);

    interp->si = si_alloc();
    interp->gc = gc_alloc();
    interp->io = io_alloc_std();
    interp->vm = vm_alloc(interp->gc, interp->si, interp->io);

    da_init(interp->tokens);
    da_init(interp->exprs);

    // The root scope goes up before register_builtins: vm_scope_define writes
    // into whatever scope is current, so there has to be one.
    vm_push_scope(interp->vm, gc_alloc_scope(interp->gc, NULL));
    register_builtins(interp->vm);
    special_forms_attach(interp->vm);

    return interp;
}

void interp_free(Interpreter *interp) {
    // Reverse order of allocation. The GC outlives the VM on purpose: vm_free
    // drops only the VM's own arrays, while every object in them belongs to the
    // GC and is released by gc_free.
    vm_free(interp->vm);

    da_free(interp->exprs);
    da_free(interp->tokens);

    io_free(interp->io);
    gc_free(interp->gc);
    si_free(interp->si);

    free(interp);
}

// The VM's result sink (installed by interp_set_echo). Renders through
// write_expr, the same path as str and print. Nil is never echoed.
static void interp_echo(VM *vm, Object *result) {
    if (OBJ_OFTYPE(result, TY_NIL))
        return;

    CharDA out;
    da_init(out);

    write_expr(vm, &out, result);
    io_write(vm->io, out.data, out.size);
    io_write(vm->io, "\n", 1);

    da_free(out);
}

void interp_set_echo(Interpreter *interp, bool echo) {
    interp->vm->on_result = echo ? interp_echo : NULL;
}

bool interp_eval(Interpreter *interp, StringView src, const char *name) {
    interp->tokens.size = 0;
    interp->exprs.size = 0;

    Lexer *lexer = lexer_alloc(src, &interp->tokens);
    lexer_run(lexer);

    // Reported before the lexer is freed, then answered after: io_report_lexer
    // reads the lexer, and the caller only ever sees the verdict.
    bool failed = lexer->is_err;
    if (failed)
        io_report_lexer(interp->io, name, lexer);
    lexer_free(lexer);
    if (failed)
        return false;

    Parser *parser = parser_alloc(&interp->tokens, &interp->exprs, interp->gc, interp->si);
    parser_run(parser);

    failed = parser->is_err;
    if (failed)
        io_report_parser(interp->io, name, parser);
    parser_free(parser);
    if (failed)
        return false;

    // The parsed objects sit in a plain C array here, rooted by nothing. That
    // is safe only because gc_alloc_* never collect on their own (see gc.h) and
    // nothing between the parse and the load builds anything.
    vm_load_instructions(interp->vm, interp->exprs);
    vm_run(interp->vm);

    if (interp->vm->is_err) {
        io_report_vm(interp->io, name, interp->vm);
        return false;
    }

    return true;
}
