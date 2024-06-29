#include "skolem.h"
#include <unordered_set>
#include <functional>
using namespace std;


/* ---------------------- set new names of variables  ---------------------- */


string add_new_var(string& ident, unordered_map<string, string>& new_idents) {
    static size_t cnt = 0;
    string new_ident = "_" + ident + "_" + to_string(cnt++);
    new_idents[ident] = new_ident;
    return new_ident;
}


void rename_term_vars(term_t& term, unordered_map<string, string>& new_idents) {
    switch(term.name) {
        case Var:
            if (new_idents.find(term.ident) != new_idents.end()) {
                term.ident = new_idents[term.ident];
            } else {
                term.ident = add_new_var(term.ident, new_idents);
            }
            break;
        case Fun:
            for (term_t& fun_term : term.terms) {
                rename_term_vars(fun_term, new_idents);
            }
            break;
    }
}


void rename_expr_vars(expr_t& expr, unordered_map<string, string>& new_idents) {

    string ident, new_ident;

    switch(expr.name) {
        case Rel:
            for (term_t& rel_term : expr.terms) {
                rename_term_vars(rel_term, new_idents);
            }
            break;
        case And:
        case Or:
        case Implies:
        case Iff:
            rename_expr_vars(expr.exprs[0], new_idents);
            rename_expr_vars(expr.exprs[1], new_idents);
            break;
        case Exists:
        case Forall:
            ident = expr.ident;
            if (new_idents.find(ident) != new_idents.end()) {
                new_ident = new_idents[ident];
            }

            expr.ident = add_new_var(ident, new_idents);
            rename_expr_vars(expr.exprs[0], new_idents);

            if (new_ident.empty()) {
                new_idents.erase(ident);
            } else {
                new_idents[ident] = new_ident; // restore ident
            }
        default:
            break;
    }
}


void rename_vars(expr_t& expr) {
    unordered_map<string, string> new_idents;
    rename_expr_vars(expr, new_idents);
}


/* -------------------------- negation normal form ------------------------- */


void neg_expr_negation(expr_t& expr) {
    expr.negation = !expr.negation;
}


void neg_expr_name(expr_t& expr) {
    const static unordered_map<expr_name, expr_name> new_names = {
            {expr_name::T, expr_name::F},
            {expr_name::F, expr_name::T},
            {expr_name::And, expr_name::Or},
            {expr_name::Or, expr_name::And},
            {expr_name::Implies, expr_name::And},
            {expr_name::Exists, expr_name::Forall},
            {expr_name::Forall, expr_name::Exists},
            {expr_name::Iff, expr_name::Or}
    };

    auto it = new_names.find(expr.name);
    if (it != new_names.end()) {
        expr.name = it->second;
    }
}


void to_nnf(expr_t& expr) {

    expr_t expr01, expr10;

    if (expr.negation) {
        expr.negation = false;

        switch(expr.name) {
            case T: // F
            case F: // T
                break;
            case And: // neg Or neg
            case Or: // neg And neg
                neg_expr_negation(expr.exprs[0]);
                neg_expr_negation(expr.exprs[1]);
                break;
            case Implies: // _ And neg
                neg_expr_negation(expr.exprs[1]);
                break;
            case Iff: // (neg =>) Or (neg <=)
                expr01 = {true, expr_name::Implies, "", {expr.exprs[0], expr.exprs[1]}};
                expr10 = {true, expr_name::Implies, "", {expr.exprs[1], expr.exprs[0]}};
                expr.exprs = {expr01, expr10};
                break;
            case Exists:
            case Forall:
                neg_expr_negation(expr.exprs[0]);
                break;
            default: // Rel
                expr.negation = true;
                break;
        }
        neg_expr_name(expr);

    } else { // no negation
        switch(expr.name) {
            case Implies: // neg Or _
                expr.name = expr_name::Or;
                neg_expr_negation(expr.exprs[0]);
                break;
            case Iff:
                expr.name = expr_name::And;
                expr01 = {false, expr_name::Implies, "", {expr.exprs[0], expr.exprs[1]}};
                expr10 = {false, expr_name::Implies, "", {expr.exprs[1], expr.exprs[0]}};
                expr.exprs = {expr01, expr10};
                break;
            default:
                break;
        }
    }

    switch(expr.name) { // nnf down to sons
        case And:
        case Or:
        case Implies:
        case Iff:
            to_nnf(expr.exprs[0]);
            to_nnf(expr.exprs[1]);
            break;
        case Exists:
        case Forall:
            to_nnf(expr.exprs[0]);
            break;
        default:
            break;
    }
}


/* ------------------------------- miniscoping ----------------------------- */


unordered_set<string> get_all_vars(term_t& term) {
    switch (term.name) {
        case Var:
            return {term.ident};
        case Fun:
            unordered_set<string> idents;
            for (term_t& fun_term: term.terms) {
                unordered_set<string> new_idents = get_all_vars(fun_term);
                idents.insert(new_idents.begin(), new_idents.end());
            }
            return idents;
    }
    return {}; // [-Wreturn-type] - warning control
}


void count_up(expr_t& expr) {
    expr.vars.clear();
    switch (expr.name) {
        case Rel:
            for (term_t& rel_term: expr.terms) {
                unordered_set<string> new_idents = get_all_vars(rel_term);
                expr.vars.insert(new_idents.begin(), new_idents.end());
            }
            break;
        case And:
        case Or:
            count_up(expr.exprs[0]);
            count_up(expr.exprs[1]);
            expr.vars.insert(expr.exprs[0].vars.begin(), expr.exprs[0].vars.end());
            expr.vars.insert(expr.exprs[1].vars.begin(), expr.exprs[1].vars.end());
            break;
        case Exists:
        case Forall:
            count_up(expr.exprs[0]);
            expr.vars = expr.exprs[0].vars;
            break;
        default: // ignore Implies and Iff (we consider only nnf form)
            break;
    }
}


void push_down_one(expr_t& expr) {
    expr_name name = expr.name;
    string ident = expr.ident;
    expr_t child_expr = expr.exprs[0];
    if (child_expr.name == expr_name::And || child_expr.name == expr_name::Or) {
        bool left_contain = child_expr.exprs[0].vars.contains(expr.ident);
        bool right_contain = child_expr.exprs[1].vars.contains(expr.ident);
        if (left_contain && !right_contain) {
            expr = child_expr;
            expr.exprs[0] = {false, name, ident, {child_expr.exprs[0]}, {}, child_expr.exprs[0].vars};
            push_down_one(expr.exprs[0]);
        } else if (!left_contain && right_contain) {
            expr = child_expr;
            expr.exprs[1] = {false, name, ident, {child_expr.exprs[1]}, {}, child_expr.exprs[1].vars};
            push_down_one(expr.exprs[1]);
        }
    } else if (expr.exprs[0].name == expr_name::Forall) {
        swap(expr.exprs[0].ident, expr.ident);
        push_down_one(expr.exprs[0]);
    }
}


void push_down(expr_t& expr) {

    for (expr_t& child_expr: expr.exprs) {
        push_down(child_expr);
    }
    if (expr.name == expr_name::Exists || expr.name == expr_name::Forall) {
        if (!expr.exprs[0].vars.contains(expr.ident)) {
            swap(expr, expr.exprs[0]);
            return;
        }
        push_down_one(expr);
    }
}


void miniscoping(expr_t& expr) {
    count_up(expr);
    push_down(expr);
}


/* --------------------------------- free vars  ---------------------------- */


void tied_vars(expr_t& expr, unordered_set<string>& idents) {
    unordered_set<string> idents0, idents1;
    switch (expr.name) {
        case And:
        case Or:
            tied_vars(expr.exprs[0], idents);
            tied_vars(expr.exprs[1], idents);
            break;
        case Exists:
        case Forall:
            tied_vars(expr.exprs[0], idents);
            idents.insert(expr.ident);
            break;
        default: // ignore Implies and Iff (we consider only nnf form)
            break;
    }
}


unordered_set<string> free_vars(expr_t& expr) {
    count_up(expr);
    unordered_set<string> idents1 = expr.vars;
    unordered_set<string> idents2;
    tied_vars(expr, idents2);
    for (auto& ident: idents2) {
        idents1.erase(ident);
    }
    return idents1;
}


void missing_forall_exprs(expr_t& expr) {
    unordered_set<string> vars = free_vars(expr);
    for (const string& ident : vars) {
        expr = {false, expr_name::Forall, ident, {expr}};
    }
}


/* -------------------------------- skolemization -------------------------- */
/* good to use on negation normal form + expected unique variables*/


 void add_new_exist(string& ident, vector<string>& args, unordered_map<string, term_t>& new_terms) {
    string new_ident = "E_" + ident;
    vector<term_t> term_args;
    for (const string& forall_ident : args) {
        term_args.push_back({term_name::Var, forall_ident, {}});
    }
    new_terms[ident] = {term_name::Fun, new_ident, term_args};
}


void map_vars_in_term(term_t& term, unordered_map<string, term_t>& new_terms) {
    switch (term.name) {
        case Var:
            if (new_terms.contains(term.ident)) {
                term = new_terms[term.ident];
            }
            break;
        case Fun:
            for (term_t& fun_term: term.terms) {
                map_vars_in_term(fun_term, new_terms);
            }
            break;
    }
}


void erase_exists(expr_t& expr, vector<string>& args, unordered_map<string, term_t>& new_terms) {
    switch (expr.name) {
        case Rel:
            for (term_t& rel_term: expr.terms) {
                map_vars_in_term(rel_term, new_terms);
            }
            break;
        case And:
        case Or:
            erase_exists(expr.exprs[0], args, new_terms);
            erase_exists(expr.exprs[1], args, new_terms);
            break;
        case Exists:
            add_new_exist(expr.ident, args, new_terms);
            erase_exists(expr.exprs[0], args, new_terms);
            swap(expr, expr.exprs[0]);
            break;
        case Forall:
            args.push_back(expr.ident);
            erase_exists(expr.exprs[0], args, new_terms);
            args.pop_back();
            break;
        default:
            break;
    }
}


void skolem_functions(expr_t& expr) {
    vector<string> args;
    unordered_map<string, term_t> new_terms;
    erase_exists(expr, args, new_terms);
}


/* --------------------------- prenex normal form -------------------------- */


void extract_forall(expr_t& expr) {
    switch (expr.name) {
        case And:
        case Or:
            extract_forall(expr.exprs[0]);
            extract_forall(expr.exprs[1]);
            break;
        case Forall:
            extract_forall(expr.exprs[0]);
            swap(expr, expr.exprs[0]);
        default:
            break;
    }
}


void to_pnf(expr_t& expr) {
    extract_forall(expr);
//    missing_forall_exprs(expr);
//    Later we would erase it while finding SAT in negation
}