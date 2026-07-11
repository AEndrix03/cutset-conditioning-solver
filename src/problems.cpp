#include "problems.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>


ProblemInstance::ProblemInstance(
        std::string problem,
        std::string instance,
        CSP csp,
        std::function<bool(const Assignment &)> validate
) : problem(std::move(problem)),
    instance(std::move(instance)),
    csp(std::move(csp)),
    validate(std::move(validate)) {

    if (!this->validate) {
        throw std::invalid_argument("ProblemInstance needs a valid validator");
    }
}


namespace {

    void check_positive(int value, const std::string &name) {
        if (value <= 0) {
            throw std::invalid_argument(name + " must be positive");
        }
    }

    void check_var(Var x, int nvars) {
        if (x < 0 || x >= nvars) {
            throw std::out_of_range("Var index is out of range");
        }
    }

    Domain range_domain(Value from, Value to) {
        if (from > to) {
            throw std::invalid_argument("Invalid domain range");
        }

        Domain domain;

        for (Value v = from; v <= to; ++v) {
            domain.push_back(v);
        }

        return domain;
    }

    Domains repeated_domains(int nvars, const Domain &domain) {
        check_positive(nvars, "nvars");

        if (domain.empty()) {
            throw std::invalid_argument("Cannot repeat empty domain");
        }

        return Domains(nvars, domain);
    }

    std::string make_name(const std::string &base, int value) {
        std::ostringstream out;
        out << base << "_" << value;
        return out.str();
    }

    // Appiattisco la griglia (row, col) in un indice di variabile lineare.
    Var cell_var(int row, int col, int ncols) {
        return row * ncols + col;
    }

    void add_not_equal(CSP &csp, Var x, Var y, const std::string &name) {
        csp.add_constraint(std::make_unique<NotEqualConstraint>(x, y, name));
    }

    // all-different su k variabili = k*(k-1)/2 vincoli binari "!=": così il CSP resta binario.
    void add_pairwise_all_different(
            CSP &csp,
            const std::vector<Var> &vars,
            const std::string &name
    ) {
        for (int i = 0; i < static_cast<int>(vars.size()); ++i) {
            for (int j = i + 1; j < static_cast<int>(vars.size()); ++j) {
                add_not_equal(csp, vars[i], vars[j], name);
            }
        }
    }

    void check_clues_shape(
            const std::vector<std::vector<Value>> &clues,
            int order
    ) {
        if (clues.empty()) {
            return;
        }

        if (static_cast<int>(clues.size()) != order) {
            throw std::invalid_argument("Invalid number of rows in clues matrix");
        }

        for (const auto &row: clues) {
            if (static_cast<int>(row.size()) != order) {
                throw std::invalid_argument("Invalid number of columns in clues matrix");
            }
        }
    }

    // Dominio di ogni cella: 1..order se libera, il solo valore fissato se è un clue.
    Domains quasigroup_domains(
            int order,
            const std::vector<std::vector<Value>> &clues
    ) {
        check_clues_shape(clues, order);

        Domain full_domain = range_domain(1, order);

        Domains domains;
        domains.reserve(order * order);

        for (int r = 0; r < order; ++r) {
            for (int c = 0; c < order; ++c) {
                if (clues.empty()) {
                    domains.push_back(full_domain);
                    continue;
                }

                Value clue = clues[r][c];

                if (clue == 0 || clue == UNASSIGNED) {
                    domains.push_back(full_domain);
                } else {
                    if (clue < 1 || clue > order) {
                        throw std::invalid_argument("Quasigroup clue outside 1..order");
                    }

                    domains.push_back(Domain{clue});
                }
            }
        }

        return domains;
    }

    std::vector<std::vector<Value>> quasigroup_clues_order_4() {
        return {
                {1, 0, 0, 4},
                {0, 3, 0, 0},
                {0, 0, 1, 0},
                {4, 0, 0, 3}
        };
    }

    std::vector<std::vector<Value>> quasigroup_clues_order_5() {
        return {
                {1, 0, 0, 0, 5},
                {0, 3, 0, 5, 0},
                {0, 0, 5, 0, 0},
                {0, 5, 0, 2, 0},
                {5, 0, 0, 0, 4}
        };
    }

    std::vector<std::vector<Value>> quasigroup_default_clues(int order) {
        if (order == 4) {
            return quasigroup_clues_order_4();
        }

        if (order == 5) {
            return quasigroup_clues_order_5();
        }

        return {};
    }

    // Slot disponibili nella giornata: 0..7. Ogni riunione dura esattamente 1 slot.
    constexpr int meeting_slot_count = 8;

    // Controlli sui conflitti: indici validi, niente riunione in conflitto con se stessa,
    // tempo di viaggio non negativo.
    void check_meeting_conflicts(int n_meetings, const std::vector<MeetingConflict> &conflicts) {
        for (const auto &conflict: conflicts) {
            check_var(conflict.first, n_meetings);
            check_var(conflict.second, n_meetings);

            if (conflict.first == conflict.second) {
                throw std::invalid_argument("A meeting cannot conflict with itself");
            }

            if (conflict.travel < 0) {
                throw std::invalid_argument("Travel time cannot be negative");
            }
        }
    }

    // Albero binario dei conflitti: la riunione i condivide un partecipante con la
    // riunione (i-1)/2. Esattamente n-1 conflitti, quindi il grafo dei conflitti è un albero.
    std::vector<MeetingConflict> meeting_tree_conflicts(int n_meetings) {
        if (n_meetings < 2) {
            throw std::invalid_argument("A meeting tree needs at least 2 meetings");
        }

        std::vector<MeetingConflict> conflicts;

        for (Var i = 1; i < n_meetings; ++i) {
            Var parent = (i - 1) / 2;
            int travel = i % 3;   // spostamento di 0, 1 o 2 slot fra le due sedi
            conflicts.push_back({parent, i, travel});
        }

        return conflicts;
    }

    // Dominio di ogni riunione: la giornata intera meno gli impegni privati dell'agente.
    // Qui una riunione su quattro ha l'agente occupato negli ultimi due slot.
    Domains meeting_domains(int n_meetings) {
        check_positive(n_meetings, "n_meetings");

        Domains domains;
        domains.reserve(n_meetings);

        for (Var i = 0; i < n_meetings; ++i) {
            Domain slots;

            for (Value slot = 0; slot < meeting_slot_count; ++slot) {
                bool busy = (i % 4 == 0) && (slot >= meeting_slot_count - 2);

                if (!busy) {
                    slots.push_back(slot);
                }
            }

            domains.push_back(slots);
        }

        return domains;
    }

}

CSP make_n_queens(int n) {
    check_positive(n, "n");

    // Una variabile per colonna, il valore è la riga della regina: le colonne sono
    // già distinte per costruzione, restano da vietare stessa riga e stessa diagonale.
    Domains domains = repeated_domains(n, range_domain(0, n - 1));
    CSP csp(make_name("n_queens", n), domains);

    for (Var col1 = 0; col1 < n; ++col1) {
        for (Var col2 = col1 + 1; col2 < n; ++col2) {
            add_not_equal(csp, col1, col2, "queens_same_row");

            csp.add_constraint(std::make_unique<PredicateConstraint>(
                    col1,
                    col2,
                    [col1, col2](Value row1, Value row2) {
                        return std::abs(col1 - col2) != std::abs(row1 - row2);
                    },
                    "queens_same_diagonal"
            ));
        }
    }

    return csp;
}

bool validate_n_queens(const Assignment &assignment) {
    int n = static_cast<int>(assignment.size());

    if (n <= 0) {
        return false;
    }

    for (Var col1 = 0; col1 < n; ++col1) {
        Value row1 = assignment[col1];

        if (row1 < 0 || row1 >= n) {
            return false;
        }

        for (Var col2 = col1 + 1; col2 < n; ++col2) {
            Value row2 = assignment[col2];

            if (row2 < 0 || row2 >= n) {
                return false;
            }

            if (row1 == row2) {
                return false;
            }

            if (std::abs(col1 - col2) == std::abs(row1 - row2)) {
                return false;
            }
        }
    }

    return true;
}

ProblemInstance make_n_queens_instance(int n) {
    CSP csp = make_n_queens(n);

    return ProblemInstance(
            "nqueens",
            make_name("n", n),
            std::move(csp),
            [](const Assignment &assignment) {
                return validate_n_queens(assignment);
            }
    );
}

CSP make_quasigroup_completion(
        int order,
        const std::vector<std::vector<Value>> &clues
) {
    check_positive(order, "order");

    Domains domains = quasigroup_domains(order, clues);
    CSP csp(make_name("quasigroup_completion", order), domains);

    // All-different su ogni riga e ogni colonna, spezzato in vincoli binari a coppie.
    for (int r = 0; r < order; ++r) {
        std::vector<Var> row_vars;
        row_vars.reserve(order);

        for (int c = 0; c < order; ++c) {
            row_vars.push_back(cell_var(r, c, order));
        }

        add_pairwise_all_different(csp, row_vars, "quasigroup_row_all_different");
    }

    for (int c = 0; c < order; ++c) {
        std::vector<Var> col_vars;
        col_vars.reserve(order);

        for (int r = 0; r < order; ++r) {
            col_vars.push_back(cell_var(r, c, order));
        }

        add_pairwise_all_different(csp, col_vars, "quasigroup_col_all_different");
    }

    return csp;
}

bool validate_quasigroup_completion(
        const Assignment &assignment,
        int order,
        const std::vector<std::vector<Value>> &clues
) {
    if (order <= 0) {
        return false;
    }

    if (static_cast<int>(assignment.size()) != order * order) {
        return false;
    }

    for (Value value: assignment) {
        if (value < 1 || value > order) {
            return false;
        }
    }

    if (!clues.empty()) {
        if (static_cast<int>(clues.size()) != order) {
            return false;
        }

        for (int r = 0; r < order; ++r) {
            if (static_cast<int>(clues[r].size()) != order) {
                return false;
            }

            for (int c = 0; c < order; ++c) {
                Value clue = clues[r][c];

                if (clue == 0 || clue == UNASSIGNED) {
                    continue;
                }

                if (clue < 1 || clue > order) {
                    return false;
                }

                if (assignment[cell_var(r, c, order)] != clue) {
                    return false;
                }
            }
        }
    }

    for (int r = 0; r < order; ++r) {
        std::vector<bool> seen(order + 1, false);

        for (int c = 0; c < order; ++c) {
            Value value = assignment[cell_var(r, c, order)];

            if (seen[value]) {
                return false;
            }

            seen[value] = true;
        }
    }

    for (int c = 0; c < order; ++c) {
        std::vector<bool> seen(order + 1, false);

        for (int r = 0; r < order; ++r) {
            Value value = assignment[cell_var(r, c, order)];

            if (seen[value]) {
                return false;
            }

            seen[value] = true;
        }
    }

    return true;
}

ProblemInstance make_quasigroup_instance(int order) {
    std::vector<std::vector<Value>> clues = quasigroup_default_clues(order);
    CSP csp = make_quasigroup_completion(order, clues);

    return ProblemInstance(
            "quasigroup",
            make_name("order", order),
            std::move(csp),
            [order, clues](const Assignment &assignment) {
                return validate_quasigroup_completion(assignment, order, clues);
            }
    );
}

CSP make_meeting_scheduling(
        const std::string &name,
        const Domains &domains,
        const std::vector<MeetingConflict> &conflicts
) {
    check_meeting_conflicts(static_cast<int>(domains.size()), conflicts);

    CSP csp(name, domains);

    // Un vincolo per conflitto: le due riunioni non si sovrappongono e lasciano il
    // tempo di viaggio. Durata fissa a 1 slot, quindi |X_i - X_j| >= 1 + travel.
    for (const auto &conflict: conflicts) {
        int travel = conflict.travel;

        csp.add_constraint(std::make_unique<PredicateConstraint>(
                conflict.first,
                conflict.second,
                [travel](Value first_start, Value second_start) {
                    return std::abs(first_start - second_start) >= 1 + travel;
                },
                "meeting_arrival_time"
        ));
    }

    return csp;
}

bool validate_meeting_scheduling(
        const Assignment &assignment,
        const Domains &domains,
        const std::vector<MeetingConflict> &conflicts
) {
    int n_meetings = static_cast<int>(domains.size());

    if (n_meetings <= 0) {
        return false;
    }

    if (static_cast<int>(assignment.size()) != n_meetings) {
        return false;
    }

    // Ogni riunione ha uno slot preso dal suo dominio, niente variabili non assegnate.
    for (Var i = 0; i < n_meetings; ++i) {
        Value slot = assignment[i];

        if (slot == UNASSIGNED) {
            return false;
        }

        if (std::find(domains[i].begin(), domains[i].end(), slot) == domains[i].end()) {
            return false;
        }
    }

    // Ogni conflitto: indici validi, niente self-loop, distanza temporale rispettata.
    for (const auto &conflict: conflicts) {
        if (conflict.first < 0 || conflict.first >= n_meetings ||
            conflict.second < 0 || conflict.second >= n_meetings) {
            return false;
        }

        if (conflict.first == conflict.second) {
            return false;
        }

        int distance = std::abs(assignment[conflict.first] - assignment[conflict.second]);

        if (distance < 1 + conflict.travel) {
            return false;
        }
    }

    return true;
}

ProblemInstance make_meeting_tree_instance(int n_meetings) {
    Domains domains = meeting_domains(n_meetings);
    std::vector<MeetingConflict> conflicts = meeting_tree_conflicts(n_meetings);

    std::string name = make_name("tree", n_meetings);
    CSP csp = make_meeting_scheduling(name, domains, conflicts);

    return ProblemInstance(
            "meeting",
            name,
            std::move(csp),
            [domains, conflicts](const Assignment &assignment) {
                return validate_meeting_scheduling(assignment, domains, conflicts);
            }
    );
}

ProblemInstance make_meeting_single_cycle_instance(int n_meetings) {
    if (n_meetings < 4) {
        throw std::invalid_argument("A single-cycle meeting instance needs at least 4 meetings");
    }

    Domains domains = meeting_domains(n_meetings);
    std::vector<MeetingConflict> conflicts = meeting_tree_conflicts(n_meetings);

    // Un solo conflitto in più chiude un ciclo (di lunghezza pari) fra riunioni già
    // collegate dall'albero: il grafo diventa unicyclic e basta un cutset di una variabile.
    conflicts.push_back({2, 3, 2});

    std::string name = make_name("single_cycle", n_meetings);
    CSP csp = make_meeting_scheduling(name, domains, conflicts);

    return ProblemInstance(
            "meeting",
            name,
            std::move(csp),
            [domains, conflicts](const Assignment &assignment) {
                return validate_meeting_scheduling(assignment, domains, conflicts);
            }
    );
}

ProblemInstance make_meeting_unsat_instance(int n_meetings) {
    const int feeder_len = 6;
    const int core_len = 5;     // ciclo DISPARI: chiave dell'insoddisfacibilità

    if (n_meetings < feeder_len + core_len) {
        throw std::invalid_argument("An unsat meeting instance needs at least 11 meetings");
    }

    const int core = feeder_len;
    const int pad = core + core_len;

    Domains domains(n_meetings, Domain{0, 1, 2, 3, 4, 5, 6, 7});
    for (Var i = core; i < core + core_len; ++i) {
        domains[i] = Domain{0, 4};
    }

    std::vector<MeetingConflict> conflicts;

    for (Var i = 1; i < feeder_len; ++i) {
        conflicts.push_back({i - 1, i, 0});
    }
    conflicts.push_back({feeder_len - 1, core, 0});

    for (Var i = 0; i < core_len; ++i) {
        conflicts.push_back({core + i, core + (i + 1) % core_len, 3});
    }

    Var prev = core + core_len - 1;
    for (Var i = pad; i < n_meetings; ++i) {
        conflicts.push_back({prev, i, 0});
        prev = i;
    }

    std::string name = make_name("unsat", n_meetings);
    CSP csp = make_meeting_scheduling(name, domains, conflicts);

    return ProblemInstance(
            "meeting",
            name,
            std::move(csp),
            [domains, conflicts](const Assignment &assignment) {
                return validate_meeting_scheduling(assignment, domains, conflicts);
            }
    );
}

ProblemInstance make_meeting_hard_sat_instance(int n_meetings) {
    const int tight = 7;

    if (n_meetings < tight + 2) {
        throw std::invalid_argument("A hard-sat meeting instance needs at least 9 meetings");
    }

    const int hub = n_meetings - 1;
    const int first_tight = hub - tight;

    Domains domains(n_meetings, Domain{0, 1, 2, 3, 4, 5, 6, 7});
    domains[hub] = Domain{0};   // l'hub è libero solo nello slot 0, e lo assegno per ultimo

    std::vector<MeetingConflict> conflicts;

    for (Var i = 1; i < first_tight; ++i) {
        conflicts.push_back({i - 1, i, 0});
    }
    conflicts.push_back({first_tight - 1, first_tight, 0});

    for (Var i = first_tight; i < hub; ++i) {
        conflicts.push_back({i, hub, 3});
    }
    conflicts.push_back({first_tight, first_tight + 1, 0});

    std::string name = make_name("hard_sat", n_meetings);
    CSP csp = make_meeting_scheduling(name, domains, conflicts);

    return ProblemInstance(
            "meeting",
            name,
            std::move(csp),
            [domains, conflicts](const Assignment &assignment) {
                return validate_meeting_scheduling(assignment, domains, conflicts);
            }
    );
}

std::vector<ProblemInstance> make_default_instances() {
    std::vector<ProblemInstance> instances;

    instances.push_back(make_n_queens_instance(8));
    instances.push_back(make_n_queens_instance(10));

    instances.push_back(make_quasigroup_instance(4));
    instances.push_back(make_quasigroup_instance(5));

    instances.push_back(make_meeting_tree_instance(40));
    instances.push_back(make_meeting_single_cycle_instance(40));
    instances.push_back(make_meeting_unsat_instance(40));
    instances.push_back(make_meeting_hard_sat_instance(40));

    return instances;
}
