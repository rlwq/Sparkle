#include "interpreter.h"
#include "string_view.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "%s: cannot open file: %s\n", path, strerror(errno));
        exit(1);
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "%s: cannot seek file: %s\n", path, strerror(errno));
        exit(1);
    }

    // -1 here would wrap into a huge size_t below.
    long length = ftell(file);
    if (length < 0) {
        fprintf(stderr, "%s: cannot size file: %s\n", path, strerror(errno));
        exit(1);
    }
    rewind(file);

    size_t size = (size_t)length;

    // Not an assert: NDEBUG would drop it in release.
    char *src = malloc(size + 1);
    if (!src) {
        fprintf(stderr, "%s: out of memory\n", path);
        exit(1);
    }

    if (fread(src, 1, size, file) != size) {
        fprintf(stderr, "%s: cannot read file\n", path);
        exit(1);
    }
    src[size] = '\0';

    fclose(file);
    return src;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s source.spk\n", argv[0]);
        return 1;
    }

    char *src = read_file(argv[1]);

    // src has to outlive the call and need not outlive it by more: interp_eval
    // finishes parsing before returning, and parsed objects own their copies.
    Interpreter *interp = interp_alloc();
    bool ok = interp_eval(interp, sv_mk(src), argv[1]);

    interp_free(interp);
    free(src);

    return ok ? 0 : 1;
}
