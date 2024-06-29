#ifndef FO_PROVER_FDPLL_H
#define FO_PROVER_FDPLL_H

#include "skolem.h"
#include "combination.h"

void to_cnf(expr_t &);

unordered_set<string> get_all_vars(term_t &);

bool sat(expr_t &);

#endif //FO_PROVER_FDPLL_H
