#include "parser.h"
#include <cstring>
#include <functional>
#include <numeric>

using namespace std;


term_name get_term_name(string &s) {
    const static unordered_map<string, term_name> term_names = {
            {"Var", term_name::Var},
            {"Fun", term_name::Fun}
    };

    auto it = term_names.find(s);
    if (it != term_names.end()) return it->second;
    exit(1);
}


expr_name get_expr_name(string &s) {
    const static unordered_map<string, expr_name> expr_names = {
            {"T",       expr_name::T},
            {"F",       expr_name::F},
            {"Rel",     expr_name::Rel},
            {"And",     expr_name::And},
            {"Or",      expr_name::Or},
            {"Implies", expr_name::Implies},
            {"Iff",     expr_name::Iff},
            {"Exists",  expr_name::Exists},
            {"Forall",  expr_name::Forall},
    };

    auto it = expr_names.find(s);
    if (it != expr_names.end()) return it->second;
    exit(1);
}


string parse_chars(size_t *pos_ptr, string &input, function<bool(char)> pred) {
    size_t i = 0, pos_start = *pos_ptr, size = input.size() - pos_start;

    while (i < size && pred(input[pos_start + i])) { i++; }
    *pos_ptr += i;

    return input.substr(pos_start, i);
}


void parse_white_spaces(size_t *pos_ptr, string &input) {
    parse_chars(pos_ptr, input, [](char c) {
        return strchr(" \n\t", c) != nullptr;
    });
}


void parse_one_char(size_t *pos_ptr, string &input, char a) {
    int cnt = 0;
    parse_chars(pos_ptr, input, [&cnt, &a](char c) {
        cnt++;
        return cnt == 1 && c == a;
    });
}


string parse_alnums(size_t *pos_ptr, string &input) {
    return parse_chars(pos_ptr, input, [](char c) {
        return isalnum(c);
    });
}


string parse_string(size_t *pos_ptr, string &input) {
    parse_one_char(pos_ptr, input, '"');
    string s = parse_alnums(pos_ptr, input);
    parse_one_char(pos_ptr, input, '"');
    return s;
}


vector<term_t> parse_terms(size_t *, string &);


term_t parse_term(size_t *pos_ptr, string &input) {
    term_t term;
    string name_str = parse_alnums(pos_ptr, input);
    parse_white_spaces(pos_ptr, input);
    term.name = get_term_name(name_str);

    switch (term.name) {
        case Var:
            term.ident = parse_string(pos_ptr, input);
            break;
        case Fun:
            term.ident = parse_string(pos_ptr, input);
            parse_white_spaces(pos_ptr, input);
            term.terms = parse_terms(pos_ptr, input);
            break;
    }
    return term;
}


vector<term_t> parse_terms(size_t *pos_ptr, string &input) {
    parse_one_char(pos_ptr, input, '[');
    parse_white_spaces(pos_ptr, input);

    vector<term_t> terms;
    size_t pos_last;

    pos_last = *pos_ptr; // no terms case
    parse_one_char(pos_ptr, input, ']');
    if (*pos_ptr != pos_last) {
        return {};
    }

    do { // at least one term
        parse_white_spaces(pos_ptr, input);
        term_t term = parse_term(pos_ptr, input);
        terms.push_back(term);
        parse_white_spaces(pos_ptr, input);

        pos_last = *pos_ptr;
        parse_one_char(pos_ptr, input, ',');
    } while (*pos_ptr != pos_last);

    parse_white_spaces(pos_ptr, input);
    parse_one_char(pos_ptr, input, ']');
    return terms;
}


expr_t parse_expr_brackets(size_t *, string &);


expr_t parse_expr(size_t *pos_ptr, string &input) {
    expr_t expr;
    string name_str = parse_alnums(pos_ptr, input);
    parse_white_spaces(pos_ptr, input);

    if (name_str == "Not") {
        expr = parse_expr_brackets(pos_ptr, input);
        expr.negation = !expr.negation;
    } else {
        expr.name = get_expr_name(name_str);

        switch (expr.name) {
            case Rel:
                expr.ident = parse_string(pos_ptr, input);
                parse_white_spaces(pos_ptr, input);
                expr.terms = parse_terms(pos_ptr, input);
                break;
            case And:
            case Or:
            case Implies:
            case Iff:
                expr.exprs.push_back(parse_expr_brackets(pos_ptr, input));
                parse_white_spaces(pos_ptr, input);
                expr.exprs.push_back(parse_expr_brackets(pos_ptr, input));
                break;
            case Exists:
            case Forall:
                expr.ident = parse_string(pos_ptr, input);
                parse_white_spaces(pos_ptr, input);
                expr.exprs.push_back(parse_expr_brackets(pos_ptr, input));
            default:
                break;
        }
    }
    return expr;
}


expr_t parse_expr_brackets(size_t *pos_ptr, string &input) {
    parse_one_char(pos_ptr, input, '(');
    parse_white_spaces(pos_ptr, input);

    expr_t expr = parse_expr(pos_ptr, input);

    parse_white_spaces(pos_ptr, input);
    parse_one_char(pos_ptr, input, ')');
    return expr;
}


expr_t parse(string &input) {
    size_t pos = 0;
    parse_white_spaces(&pos, input);
    expr_t expr = parse_expr(&pos, input);
    parse_white_spaces(&pos, input);
    return expr;
}


void print_term(term_t &t) {
    switch (t.name) {
        case (term_name::Var):
            printf("Var %s", t.ident.c_str());
            break;
        case (term_name::Fun):
            printf("Fun %s [", t.ident.c_str());
            for (auto &ft: t.terms) {
                print_term(ft);
                printf(", ");
            }
            printf("]");
            break;
    }
}


void print_expr(expr_t &e, string offset) {
    printf("%s", offset.c_str());
    if (e.negation) {
        printf("¬");
    }
    switch (e.name) {
        case T:
            printf("T\n");
            break;
        case F:
            printf("F\n");
            break;
        case Rel:
            printf("Rel %s [", e.ident.c_str());
            for (auto &ft: e.terms) {
                print_term(ft);
                printf(", ");
            }
            printf("]\n");
            break;
        case And:
            printf("^\n");
            for (auto &expr: e.exprs) {
                print_expr(expr, offset + "  ");
            }
            break;
        case Or:
            printf("v\n");
            for (auto &expr: e.exprs) {
                print_expr(expr, offset + "  ");
            }
            break;
        case Implies:
            printf("=>\n");
            for (auto &expr: e.exprs) {
                print_expr(expr, offset + "  ");
            }
            break;
        case Iff:
            printf("<=>\n");
            for (auto &expr: e.exprs) {
                print_expr(expr, offset + "  ");
            }
            break;
        case Exists:
            printf("∃ %s\n", e.ident.c_str());
            for (auto &expr: e.exprs) {
                print_expr(expr, offset + "  ");
            }
            break;
        case Forall:
            printf("∀ %s\n", e.ident.c_str());
            for (auto &expr: e.exprs) {
                print_expr(expr, offset + "  ");
            }
            break;
    }
}


/* -------------------------- hash function for term ------------------------*/


string term_t::get_hash() const {
    if (name == term_name::Var) {
        return ident;
    }
    string hash = "#_" + ident + "(";
    for (const term_t &child_term: terms) {
        hash += child_term.get_hash() + ",";
    }
    hash += ")";
    return hash;
}

bool term_t::operator<(const term_t &o) const {
    return this->get_hash() < o.get_hash();
}

bool term_t::operator==(const term_t &o) const {
    return this->get_hash() == o.get_hash();
}

bool expr_t::operator==(const expr_t &o) const { // Missing exprs and vars
    bool simple = (negation == o.negation) && (name == o.name) &&
                  (ident == o.ident) && (terms.size() == o.terms.size());
    for (int i = 0; i < terms.size() && simple; i++) {
        simple = simple && terms[i] == o.terms[i];
    }
    return simple;
}
