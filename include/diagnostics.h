#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "lexer.h"
#include "parser.h"

void diag_lexer(const char *path, Lexer *lexer);
void diag_parser(const char *path, Parser *parser);
void diag_vm(const char *path, VM *vm);

#endif
