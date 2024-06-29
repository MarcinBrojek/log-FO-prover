#ifndef FO_PROVER_PARSER_H
#define FO_PROVER_PARSER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;


enum term_name {
    Var, Fun
};
enum expr_name {
    T, F, Rel, And, Or, Implies, Iff, Exists, Forall
};


struct term_t {
    term_name name;
    string ident;
    vector<term_t> terms;

    string get_hash() const;

    bool operator<(const term_t &o) const;

    bool operator==(const term_t &o) const;
};


struct expr_t {
    bool negation = false;
    expr_name name;
    string ident;
    vector<expr_t> exprs;
    vector<term_t> terms;
    unordered_set<string> vars;

    bool operator==(const expr_t &o) const;
};


expr_t parse(string &s); // parse expression


// helpers
void print_term(term_t &);

void print_expr(expr_t &, string);


#endif //FO_PROVER_PARSER_H
