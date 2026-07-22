#include "repl.h"

#include "dynamic_array.h"
#include "io.h"
#include "string_view.h"

#include <assert.h>
#include <stdlib.h>

Repl *repl_alloc(void) {
    Repl *repl = malloc(sizeof(Repl));
    assert(repl);

    repl->interp = interp_alloc();
    interp_set_echo(repl->interp, true);

    da_init(repl->line);

    return repl;
}

void repl_free(Repl *repl) {
    interp_free(repl->interp);
    da_free(repl->line);

    free(repl);
}

void repl_run(Repl *repl) {
    Io *io = repl->interp->io;

    while (true) {
        io_err_write(io, "> ", 2);

        if (!io_read_line(io, &repl->line))
            break;
        interp_eval(repl->interp, sv(repl->line.data, repl->line.size), "<repl>");
    }

    io_err_write(io, "\n", 1);
}
