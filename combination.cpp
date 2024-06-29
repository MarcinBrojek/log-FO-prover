#include "combination.h"

void combi_t::next_combi() {
    bool increase_max_value =
            1 == accumulate(current_mask.begin(), current_mask.end(), 1,
                            [](int acc, int new_value) {
                                return min(acc, new_value);
                            });

    bool shift_max_combination =
            current_max_value - 1 ==
            accumulate(current_combi.begin(), current_combi.end(),
                       current_max_value, [](int acc, int new_value) {
                        return min(acc, new_value);
                    });

    if (increase_max_value) {
        current_max_value++;
        current_mask.assign(no_of_vars, 0);
        current_mask.front() = 1;
    } else if (shift_max_combination) {
        for (int i = 0; i < no_of_vars; i++) {
            current_mask[i]++;
            if (current_mask[i] != 2) {
                break;
            } else {
                current_mask[i] = 0;
            }
        }
    }

    if (increase_max_value || shift_max_combination) {
        for (int i = 0; i < no_of_vars; i++) {
            if (current_mask[i]) {
                current_combi[i] = current_max_value;
            } else {
                current_combi[i] = 0;
            }
        }
    } else {
        auto find_next_index_to_increase = [&](int index) {
            for (index++; index < no_of_vars; index++) {
                if (!current_mask[index]) {
                    break;
                }
            }
            return index;
        };
        for (int i = find_next_index_to_increase(-1); i < no_of_vars;
             i = find_next_index_to_increase(i)) {
            current_combi[i]++;
            if (current_combi[i] != current_max_value) {
                break;
            } else {
                current_combi[i] = 0;
            }
        }
    }
}
