#ifndef FO_PROVER_COMBINATION_H
#define FO_PROVER_COMBINATION_H

#include <vector>
#include <numeric>

using namespace std;

struct combi_t {
    int no_of_vars;
    int current_max_value = 0;
    vector<int> current_mask;
    vector<int> current_combi;

    combi_t(int no_of_vars) : no_of_vars(no_of_vars) {
        current_combi = vector<int>(no_of_vars, 0);
        current_mask = vector<int>(no_of_vars, 1);
    }

    void next_combi();
};

#endif //FO_PROVER_COMBINATION_H
