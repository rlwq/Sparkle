#include "repl.h"

#include "dynamic_array.h"
#include "string_view.h"

#include <assert.h>
#include <stdio.h>
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

static bool repl_read_line(Repl *repl) {
    repl->line.size = 0;

    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n')
        da_push(repl->line, (char)c);

    return c != EOF || repl->line.size > 0;
}

void repl_run(Repl *repl) {
    while (true) {
        fflush(stdout);
        fputs("> ", stderr);
        fflush(stderr);

        if (!repl_read_line(repl))
            break;
        interp_eval(repl->interp, sv(repl->line.data, repl->line.size), "<repl>");
    }

    fputc('\n', stderr);
}
