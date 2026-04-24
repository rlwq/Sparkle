#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "lisp_ast.h"
#include "string_view.h"

typedef struct {
    da_lisp_ast_ptr exprs;
    size_t cursor;
    Env* global_scope;
} Evaluator;

Evaluator *evaluator_alloc(da_lisp_ast_ptr exprs);
void evaluator_free(Evaluator *evaluator);

// TODO: Should maybe bind to a Symbol or smth like that
void register_builtin(Evaluator *evaluator, StringView name, LispBuiltin func_ptr);

LispAST *eval_expr(LispAST *expr, Env *env);
LispAST *eval_current(Evaluator *evaluator);
void eval_all(Evaluator *evaluator);
LispAST *lisp_print(LispAST);

void evaluator_mark(Evaluator *evaluator);

#endif
