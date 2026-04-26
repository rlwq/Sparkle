#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "lisp_ast.h"
#include "string_view.h"

#define EVALUATOR_DONE(e_) ((e_)->stmts_count == 0)
#define EVALUATOR_VALID(e_) (!EVALUATOR_DONE(e_) && !(e_)->is_err)

typedef struct {
    LispAST **stmts;
    size_t stmts_count;

    LispASTPtrDA results;
    Env* global_scope;

    bool is_err;
} Evaluator;

Evaluator *evaluator_alloc(LispASTPtrDA exprs);
void evaluator_free(Evaluator *evaluator);

// TODO: Should maybe bind to a Symbol or smth like that
void register_builtin(Evaluator *evaluator, StringView name, LispBuiltin func_ptr);

void eval_current(Evaluator *evaluator);
void eval_all(Evaluator *evaluator);

LispASTPtrDA extract_results(Evaluator *evaluator);

void evaluator_mark_stmts(Evaluator *evaluator);

#endif
