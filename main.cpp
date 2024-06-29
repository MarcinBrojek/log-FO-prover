#include <iostream>
#include <string>
#include "fdpll.h"


void skolemization(expr_t &expr) {
    to_nnf(expr);
    rename_vars(expr); // done after translating <=> (in to_nnf)
    missing_forall_exprs(expr);

    expr.negation = !expr.negation;
    to_nnf(expr);

    miniscoping(expr);
    skolem_functions(expr);
    to_pnf(expr); // missing leading forall - intentionally (for dpll)
}


bool fdpll(expr_t &expr) {
    to_nnf(expr); // maybe not necessary (after skolemization)
    to_cnf(expr);
    return sat(expr);
}


int main() {
    string input;
    getline(std::cin, input);
    auto expr = parse(input);
    skolemization(expr);
    if (!fdpll(expr)) {
        printf("1");
    } else {
        printf("0");
    }
    return 0;
}
