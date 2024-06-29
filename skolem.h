#ifndef FO_PROVER_SKOLEM_H
#define FO_PROVER_SKOLEM_H
#include "parser.h"

void rename_vars(expr_t&);
void to_nnf(expr_t&);
void missing_forall_exprs(expr_t&);
void miniscoping(expr_t&);
void skolem_functions(expr_t&);
void to_pnf(expr_t&);


#endif //FO_PROVER_SKOLEM_H
