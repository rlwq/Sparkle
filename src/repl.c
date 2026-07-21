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

// Reads one line into repl->line, without the newline. Returns false only at
// the end of input: end of input with bytes already read is still a line, so a
// last line that no newline terminates is evaluated like any other.
static bool repl_read_line(Repl *repl) {
    repl->line.size = 0;

    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n')
        da_push(repl->line, (char)c);

    return c != EOF || repl->line.size > 0;
}

void repl_run(Repl *repl) {
    while (true) {
        // Everything the previous line produced goes out before the next
        // prompt. stdout is block buffered as soon as it is not a terminal, so
        // without this the prompts - stderr, unbuffered - all appear up front
        // and the output arrives afterwards in a heap.
        fflush(stdout);

        // The prompt goes to stderr so that stdout holds only what the session
        // produced. Flushed because it has to be on screen before the read
        // blocks waiting for the answer.
        fputs("> ", stderr);
        fflush(stderr);

        if (!repl_read_line(repl))
            break;

        // An empty line lexes to no tokens and parses to no expressions, so it
        // costs a run of nothing rather than needing a case here.
        interp_eval(repl->interp, sv(repl->line.data, repl->line.size), "<repl>");
    }

    // End of input leaves the cursor after the prompt, so the shell's own
    // prompt would land on the same line.
    fputc('\n', stderr);
}
