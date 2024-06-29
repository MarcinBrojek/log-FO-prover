#include "fdpll.h"
#include <set>
#include <utility>

using namespace std;


/* -------------------------------- CNF ------------------------------------ */


void to_cnf(expr_t &expr) {
    static int cnt = 0;
    static expr_t empty_or_expr = {false, expr_name::Or};
    static vector<expr_t> tmp_exprs;
    static auto simple_exprs = unordered_set<expr_name>(
            {expr_name::T, expr_name::F, expr_name::Rel});
    bool simple_children = false;

    switch (expr.name) {
        case T:
        case F:
        case Rel:
            expr.exprs.clear();
            empty_or_expr.exprs.clear();
            empty_or_expr.exprs.push_back(expr);
            expr.exprs.push_back(empty_or_expr);
            break;
        case And:
            to_cnf(expr.exprs[0]);
            to_cnf(expr.exprs[1]);
            if (expr.exprs[0].exprs.size() < expr.exprs[1].exprs.size()) {
                swap(expr.exprs[0].exprs, expr.exprs[1].exprs);
            }
            expr.exprs[0].exprs.insert(
                    expr.exprs[0].exprs.end(),
                    expr.exprs[1].exprs.begin(),
                    expr.exprs[1].exprs.end()
            );
            swap(expr.exprs, expr.exprs[0].exprs);
            break;
        case Or:
            simple_children = simple_exprs.contains(expr.exprs[0].name) &&
                              simple_exprs.contains(expr.exprs[1].name);

            if (simple_children) {
                expr.exprs = {expr};
                break;
            }

            to_cnf(expr.exprs[0]);
            to_cnf(expr.exprs[1]);
            if (expr.exprs[0].exprs.size() < expr.exprs[1].exprs.size()) {
                swap(expr.exprs[0].exprs, expr.exprs[1].exprs);
            }
            tmp_exprs.clear();

            empty_or_expr.exprs.clear();
            empty_or_expr.exprs.push_back(
                    {false, expr_name::Rel, "R_a" + to_string(cnt)});
            empty_or_expr.exprs.push_back(
                    {false, expr_name::Rel, "R_b" + to_string(cnt)});

            tmp_exprs.push_back(empty_or_expr);

            empty_or_expr.exprs.clear();
            empty_or_expr.exprs.push_back(
                    {true, expr_name::Rel, "R_a" + to_string(cnt)});
            empty_or_expr.exprs.push_back(
                    {true, expr_name::Rel, "R_b" + to_string(cnt)});

            tmp_exprs.push_back(empty_or_expr);

            for (expr_t &child_expr: expr.exprs[0].exprs) {
                child_expr.exprs.insert(child_expr.exprs.begin(),
                                        {false, expr_name::Rel,
                                         "R_a" + to_string(cnt)});
            }
            for (expr_t &child_expr: expr.exprs[1].exprs) {
                child_expr.exprs.insert(child_expr.exprs.begin(),
                                        {false, expr_name::Rel,
                                         "R_b" + to_string(cnt)});
            }
            cnt++;
            tmp_exprs.insert(tmp_exprs.end(), expr.exprs[0].exprs.begin(),
                             expr.exprs[0].exprs.end());
            tmp_exprs.insert(tmp_exprs.end(), expr.exprs[1].exprs.begin(),
                             expr.exprs[1].exprs.end());
            swap(expr.exprs, tmp_exprs);
            break;
        default:
            break;
    }
    expr.name = expr_name::And;
    expr.negation = false;
}


/* ------------------------- Vars / Cosnts Extraction ---------------------- */


void all_vars(expr_t &expr, unordered_set<string> &idents) {
    switch (expr.name) {
        case Rel:
            for (term_t &term: expr.terms) {
                idents.merge(get_all_vars(term));
            }
        case And:
        case Or:
            for (expr_t &child_exprs: expr.exprs) {
                all_vars(child_exprs, idents);
            }
            break;
        default:
            break;
    }
}


void all_consts(term_t &term, unordered_set<string> &idents) {
    if (term.name == term_name::Fun) {
        if (term.terms.empty()) {
            idents.insert(term.ident);
        }
        for (term_t &fun_term: term.terms) {
            all_consts(fun_term, idents);
        }
    }
}


void all_consts(expr_t &expr, unordered_set<string> &idents) {
    switch (expr.name) {
        case Rel:
            for (term_t &term: expr.terms) {
                all_consts(term, idents);
            }
            break;
        case And:
        case Or:
            for (expr_t &child_exprs: expr.exprs) {
                all_consts(child_exprs, idents);
            }
            break;
        default:
            break;
    }
}


void all_funs(term_t &term, set<pair<string, int>> &idents) {
    if (term.name == term_name::Fun && !term.terms.empty()) {
        idents.insert({term.ident, term.terms.size()});
        for (term_t &fun_term: term.terms) {
            all_funs(fun_term, idents);
        }
    }
}


void all_funs(expr_t &expr, set<pair<string, int>> &idents) {
    switch (expr.name) {
        case Rel:
            for (term_t &term: expr.terms) {
                all_funs(term, idents);
            }
            break;
        case And:
        case Or:
            for (expr_t &child_expr: expr.exprs) {
                all_funs(child_expr, idents);
            }
            break;
        default:
            break;
    }
}


/* ------------------------- Herbrand Env ---------------------------------- */


struct hp_t {
    unordered_set<string> vars, consts;
    set<pair<string, int>> funs;

    vector<term_t> hp_prod;
    set<term_t> hp_prod_set;

    hp_t(expr_t &expr) {
        all_vars(expr, vars);
        all_consts(expr, consts);
        all_funs(expr, funs);

        if (consts.empty()) {
            consts.insert("c"); // default const if no other exists
        }
        for (const string &ident: consts) {
            hp_prod.push_back({term_name::Fun, ident});
            hp_prod_set.insert({term_name::Fun, ident});
        }
    }

    vector<term_t> new_prod_for_fun(pair<string, int> &fun) {
        vector<term_t> new_prod;
        string fun_ident = fun.first;
        int arity = fun.second;

        int max_mask = 1;
        for (int i = 0; i < arity; i++) {
            max_mask *= (int) hp_prod.size();
        }

        for (int i = 0; i < max_mask; i++) {
            int mask = i;
            term_t new_term = {term_name::Fun, fun_ident};
            for (int j = 0; j < arity; j++) {
                new_term.terms.push_back(hp_prod[mask % hp_prod.size()]);
                mask /= (int) hp_prod.size();
            }
            if (!hp_prod_set.contains(new_term)) {
                new_prod.push_back(new_term);
                hp_prod_set.insert(new_term);
            }
        }
        return new_prod;
    }

    bool get_prod(int index, term_t &term) { // expected ident in map
        if (funs.empty() && index >= consts.size()) {
            return false;
        }
        while (index >= hp_prod.size()) {
            vector<term_t> new_prod;
            for (pair<string, int> fun: funs) {
                auto prod_for_fun = new_prod_for_fun(fun);
                new_prod.insert(new_prod.end(), prod_for_fun.begin(),
                                prod_for_fun.end());
            }
            hp_prod.insert(hp_prod.end(), new_prod.begin(), new_prod.end());
        }
        term = hp_prod[index];
        return true;
    }
};


/* ----------------------------- first order dpll -------------------------- */


void clear_unused(vector<expr_t> &vec, vector<expr_t>::iterator it) {
    while (it != vec.end()) {
        vec.pop_back();
    }
}


bool simplify_formula(expr_t &expr) {
    // Remove clauses with T
    clear_unused(expr.exprs,
                 remove_if(expr.exprs.begin(), expr.exprs.end(),
                           [](expr_t &or_expr) {
                               return accumulate(or_expr.exprs.begin(),
                                                 or_expr.exprs.end(), false,
                                                 [](bool acc,
                                                    expr_t &rel_expr) {
                                                     return acc ||
                                                            rel_expr.name ==
                                                            expr_name::T;
                                                 });
                           }));
    // Remove clauses with: R v !R
    clear_unused(expr.exprs,
                 remove_if(expr.exprs.begin(), expr.exprs.end(),
                           [](expr_t &or_expr) {
                               return accumulate(or_expr.exprs.begin(),
                                                 or_expr.exprs.end(), false,
                                                 [&](bool acc,
                                                     expr_t &rel_expr) {
                                                     expr_t neg_rel_expr = {
                                                             !rel_expr.negation,
                                                             rel_expr.name,
                                                             rel_expr.ident,
                                                             rel_expr.exprs,
                                                             rel_expr.terms,
                                                             rel_expr.vars};
                                                     return acc || std::find(
                                                             or_expr.exprs.begin(),
                                                             or_expr.exprs.end(),
                                                             neg_rel_expr) !=
                                                                   or_expr.exprs.end();
                                                 });
                           }));
    // Remove F from the clauses -> and return false if there are an empty clause
    for_each(expr.exprs.begin(), expr.exprs.end(), [](expr_t &or_expr) {
        clear_unused(or_expr.exprs,
                     remove_if(or_expr.exprs.begin(), or_expr.exprs.end(),
                               [](expr_t &rel_expr) {
                                   return rel_expr.name == expr_name::F;
                               }));
    });
    return accumulate(expr.exprs.begin(), expr.exprs.end(), true,
                      [](bool acc, expr_t &or_expr) {
                          return acc && !or_expr.exprs.empty();
                      });
}


void subs_exprs(expr_t &expr, expr_t &true_expr) {
    expr_t false_expr = true_expr;
    false_expr.negation = !true_expr.negation;

    for (expr_t &or_expr: expr.exprs) {
        for (expr_t &rel_expr: or_expr.exprs) {
            if (rel_expr == true_expr) {
                rel_expr = {false, expr_name::T};
            }
            if (rel_expr == false_expr) {
                rel_expr = {false, expr_name::F};
            }
        }
    }
}


bool dpll(expr_t &expr) {
    if (!simplify_formula(expr)) {
        return false;
    }
    if (expr.exprs.empty()) {
        return true;
    }
    auto singleton_it = find_if(expr.exprs.begin(), expr.exprs.end(),
                                [](expr_t &or_expr) {
                                    return or_expr.exprs.size() == 1;
                                });
    if (singleton_it != expr.exprs.end()) {
        expr_t true_expr = singleton_it->exprs[0];
        subs_exprs(expr, true_expr);
        return dpll(expr);
    } else {
        expr_t expr_copy = expr;
        expr_t true_expr = expr.exprs[0].exprs[0];

        subs_exprs(expr, true_expr);
        if (dpll(expr)) {
            return true;
        }
        true_expr.negation = !true_expr.negation;
        subs_exprs(expr_copy, true_expr);
        return dpll(expr_copy);
    }
}


/* -------------------- following Herbrands substitutions ------------------ */


unordered_map<string, int> assign_vars_indexes(hp_t &hp) {
    int index = 0;
    unordered_map<string, int> vars_to_indexes;
    for (const string &ident: hp.vars) {
        vars_to_indexes[ident] = index++;
    }
    return vars_to_indexes;
}


bool substitute_term(term_t &term, hp_t &hp, combi_t &c,
                     unordered_map<string, int> &vti) {
    switch (term.name) {
        case Var:
            return hp.get_prod(c.current_combi[vti[term.ident]], term);
        case Fun:
            for (term_t &sub_term: term.terms) {
                if (!substitute_term(sub_term, hp, c, vti)) {
                    return false;
                }
            }
            return true;
    }
    return true; // [-Wreturn-type] - warning control
}


bool append_combined_expr(expr_t &res, expr_t &expr, hp_t &hp, combi_t &c,
                          unordered_map<string, int> &vti) {
    static int combine_cnt = 0;
    for (expr_t &or_expr: expr.exprs) {
        res.exprs.push_back(or_expr);
        for (expr_t &rel_expr: res.exprs.back().exprs) {
            if (rel_expr.name != expr_name::Rel) {
                continue;
            }
            if (rel_expr.ident.size() > 2 && rel_expr.ident[1] == '_' &&
                rel_expr.terms.empty()) {
                rel_expr.ident += "_cc:" + to_string(combine_cnt);
            } else {
                for (term_t &term: rel_expr.terms) {
                    if (!substitute_term(term, hp, c, vti)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}


bool sat(expr_t &expr) {

    static int ddpl_frequency = 128;

    if (!simplify_formula(expr)) {
        return false;
    }
    hp_t hp = hp_t(expr);
    combi_t combination = combi_t((int) hp.vars.size());
    auto vars_to_indexes = assign_vars_indexes(hp);
    expr_t combined_expr = {false, expr_name::And};
    for (int i = 0; append_combined_expr(combined_expr, expr, hp, combination,
                                         vars_to_indexes); i++) {

        expr_t expr_copy = combined_expr;

        // we will sometimes run ddpl, not after each change of formula
        if (!(i % ddpl_frequency) && !dpll(expr_copy)) {
            return false;
        }

        if (hp.vars.empty()) { // C1
            break;
        }
        combination.next_combi();
    }

    if (!dpll(combined_expr)) {
        return false;
    }
    return true;
}


