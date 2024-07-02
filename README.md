# FO-prover

The objective of this assignment is to build a prover for tautologies of first-order
logic based on Herbrand’s theorem and the Davis-Putnam SAT solver. The solution program must read the standard input stdin. The input consists of a single line encoding a formula of first-order logic according to the following Backus-Naur grammar:

```haskell
formula ::= 
    | "T"
    | "F"
    | "Rel" string [t]*
    | "Not(" formula ")"
    | "And(" formula ")(" formula ")"
    | "Or(" formula ")(" formula ")"
    | "Implies(" formula ")(" formula ")"
    | "Iff(" formula ")(" formula ")"
    | "Exists" string "(" formula ")"
    | "Forall" string "(" formula ")"


t ::= "Var" string | "Fun" string [t]*
```

string is any sequence of alphanumeric characters.

---

FO-prover should output:

- timeout - when formula contains free variables and infinite Herbrand universe 
  (non-tautology)

otherwise:

- 1 - tautology,

- 0 - non-tautology.

---

## Example run

```bash
> make
> echo 'Or(Rel "r" [Var "a"])(Not(Rel "r" [Var "a"]))' | ./FO-prover
```

returns 1 (known tautology $p \vee \neg p$)

---

## File structure

- `README.md` (this)
- `parser.h`, `parser.cpp`
- `combination.h`, `combination.cpp`
- `skolem.h`, `skolem.cpp`
- `fdpll.h`, `fdpll.cpp`
- `main.cpp`
- `makefile`
