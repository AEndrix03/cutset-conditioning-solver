#include "problems.hpp"

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

    bool edge_exists(const std::vector<Edge> &edges, Var u, Var v) {
        for (const auto &edge: edges) {
            if ((edge.first == u && edge.second == v) ||
                (edge.first == v && edge.second == u)) {
                return true;
            }
        }

        return false;
    }

    void add_edge(std::vector<Edge> &edges, Var u, Var v) {
        if (u == v) {
            throw std::invalid_argument("Self-loop is not allowed");
        }

        if (!edge_exists(edges, u, v)) {
            edges.push_back({u, v});
        }
    }

    void check_graph_edges(int n_vertices, const std::vector<Edge> &edges) {
        check_positive(n_vertices, "n_vertices");

        if (edges.empty()) {
            throw std::invalid_argument("Graph must contain at least one edge");
        }

        for (const auto &edge: edges) {
            check_var(edge.first, n_vertices);
            check_var(edge.second, n_vertices);

            if (edge.first == edge.second) {
                throw std::invalid_argument("Graph cannot contain self-loops");
            }
        }
    }

    bool every_vertex_has_edge(int n_vertices, const std::vector<Edge> &edges) {
        std::vector<bool> seen(n_vertices, false);

        for (const auto &edge: edges) {
            seen[edge.first] = true;
            seen[edge.second] = true;
        }

        for (bool value: seen) {
            if (!value) {
                return false;
            }
        }

        return true;
    }

    // Un arco porta le etichette dei suoi due estremi: le impacchetto in un solo
    // intero (base = q+1, così ogni etichetta 0..q sta in una "cifra").
    Value encode_pair(Value first, Value second, int base) {
        return first * base + second;
    }

    std::pair<Value, Value> decode_pair(Value value, int base) {
        if (value < 0) {
            throw std::invalid_argument("Encoded pair cannot be negative");
        }

        return {value / base, value % base};
    }

    // Dominio di una variabile-arco: tutte le coppie di etichette con estremi diversi.
    Domain graceful_edge_domain(int q) {
        int base = q + 1;
        Domain domain;

        for (Value first = 0; first <= q; ++first) {
            for (Value second = 0; second <= q; ++second) {
                if (first != second) {
                    domain.push_back(encode_pair(first, second, base));
                }
            }
        }

        return domain;
    }

    // Vincolo tra due archi: se condividono un vertice deve avere la stessa etichetta,
    // vertici diversi devono avere etichette diverse, e le due differenze devono differire.
    bool graceful_pair_ok(
            Edge first_edge,
            Value first_value,
            Edge second_edge,
            Value second_value,
            int q
    ) {
        int base = q + 1;

        auto [a, b] = decode_pair(first_value, base);
        auto [c, d] = decode_pair(second_value, base);

        std::vector<std::pair<Var, Value>> labels = {
                {first_edge.first,   a},
                {first_edge.second,  b},
                {second_edge.first,  c},
                {second_edge.second, d}
        };

        for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
            for (int j = i + 1; j < static_cast<int>(labels.size()); ++j) {
                bool same_vertex = labels[i].first == labels[j].first;
                bool same_label = labels[i].second == labels[j].second;

                if (same_vertex && !same_label) {
                    return false;
                }

                if (!same_vertex && same_label) {
                    return false;
                }
            }
        }

        int first_difference = std::abs(a - b);
        int second_difference = std::abs(c - d);

        return first_difference != second_difference;
    }

    // Dalle variabili-arco risalgo alle etichette dei vertici; ritorna false se gli
    // archi che toccano lo stesso vertice non concordano sull'etichetta.
    bool reconstruct_graceful_labels(
            const Assignment &assignment,
            int n_vertices,
            const std::vector<Edge> &edges,
            std::vector<Value> &labels
    ) {
        int q = static_cast<int>(edges.size());
        int base = q + 1;

        if (static_cast<int>(assignment.size()) != q) {
            return false;
        }

        labels.assign(n_vertices, UNASSIGNED);

        for (int i = 0; i < q; ++i) {
            auto [first_label, second_label] = decode_pair(assignment[i], base);

            if (first_label < 0 || first_label > q ||
                second_label < 0 || second_label > q ||
                first_label == second_label) {
                return false;
            }

            Var u = edges[i].first;
            Var v = edges[i].second;

            if (labels[u] != UNASSIGNED && labels[u] != first_label) {
                return false;
            }

            if (labels[v] != UNASSIGNED && labels[v] != second_label) {
                return false;
            }

            labels[u] = first_label;
            labels[v] = second_label;
        }

        for (Value label: labels) {
            if (label == UNASSIGNED) {
                return false;
            }
        }

        return true;
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

CSP make_graceful_graph(
        const std::string &name,
        int n_vertices,
        const std::vector<Edge> &edges
) {
    check_graph_edges(n_vertices, edges);

    if (!every_vertex_has_edge(n_vertices, edges)) {
        throw std::invalid_argument("Graceful instances used here must not contain isolated vertices");
    }

    // Modello: una variabile per arco (non per vertice), così il CSP resta binario.
    int q = static_cast<int>(edges.size());
    Domain edge_domain = graceful_edge_domain(q);
    Domains domains = repeated_domains(q, edge_domain);

    CSP csp(name, domains);

    // Un vincolo per ogni coppia di archi.
    for (Var e1 = 0; e1 < q; ++e1) {
        for (Var e2 = e1 + 1; e2 < q; ++e2) {
            Edge first_edge = edges[e1];
            Edge second_edge = edges[e2];

            csp.add_constraint(std::make_unique<PredicateConstraint>(
                    e1,
                    e2,
                    [first_edge, second_edge, q](Value first_value, Value second_value) {
                        return graceful_pair_ok(
                                first_edge,
                                first_value,
                                second_edge,
                                second_value,
                                q
                        );
                    },
                    "graceful_pair_consistency"
            ));
        }
    }

    return csp;
}

bool validate_graceful_graph(
        const Assignment &assignment,
        int n_vertices,
        const std::vector<Edge> &edges
) {
    if (n_vertices <= 0 || edges.empty()) {
        return false;
    }

    int q = static_cast<int>(edges.size());

    if (static_cast<int>(assignment.size()) != q) {
        return false;
    }

    std::vector<Value> labels;

    if (!reconstruct_graceful_labels(assignment, n_vertices, edges, labels)) {
        return false;
    }

    std::vector<bool> used_vertex_labels(q + 1, false);

    for (Value label: labels) {
        if (label < 0 || label > q) {
            return false;
        }

        if (used_vertex_labels[label]) {
            return false;
        }

        used_vertex_labels[label] = true;
    }

    std::vector<bool> used_edge_labels(q + 1, false);

    for (const auto &edge: edges) {
        Value difference = std::abs(labels[edge.first] - labels[edge.second]);

        if (difference < 1 || difference > q) {
            return false;
        }

        if (used_edge_labels[difference]) {
            return false;
        }

        used_edge_labels[difference] = true;
    }

    return true;
}

std::vector<Edge> graceful_path_edges(int n_vertices) {
    check_positive(n_vertices, "n_vertices");

    if (n_vertices < 2) {
        throw std::invalid_argument("Path graph needs at least 2 vertices");
    }

    std::vector<Edge> edges;

    for (Var v = 0; v + 1 < n_vertices; ++v) {
        add_edge(edges, v, v + 1);
    }

    return edges;
}

std::vector<Edge> graceful_star_edges(int n_leaves) {
    check_positive(n_leaves, "n_leaves");

    std::vector<Edge> edges;

    for (Var leaf = 1; leaf <= n_leaves; ++leaf) {
        add_edge(edges, 0, leaf);
    }

    return edges;
}

ProblemInstance make_graceful_path_instance(int n_vertices) {
    std::vector<Edge> edges = graceful_path_edges(n_vertices);

    std::ostringstream name;
    name << "path_" << n_vertices;

    CSP csp = make_graceful_graph(name.str(), n_vertices, edges);

    return ProblemInstance(
            "graceful",
            name.str(),
            std::move(csp),
            [n_vertices, edges](const Assignment &assignment) {
                return validate_graceful_graph(assignment, n_vertices, edges);
            }
    );
}

ProblemInstance make_graceful_star_instance(int n_leaves) {
    std::vector<Edge> edges = graceful_star_edges(n_leaves);
    int n_vertices = n_leaves + 1;

    std::ostringstream name;
    name << "star_" << n_leaves << "_leaves";

    CSP csp = make_graceful_graph(name.str(), n_vertices, edges);

    return ProblemInstance(
            "graceful",
            name.str(),
            std::move(csp),
            [n_vertices, edges](const Assignment &assignment) {
                return validate_graceful_graph(assignment, n_vertices, edges);
            }
    );
}

std::vector<ProblemInstance> make_default_instances() {
    std::vector<ProblemInstance> instances;

    instances.push_back(make_n_queens_instance(8));
    instances.push_back(make_n_queens_instance(10));

    instances.push_back(make_quasigroup_instance(4));
    instances.push_back(make_quasigroup_instance(5));

    instances.push_back(make_graceful_path_instance(6));
    instances.push_back(make_graceful_star_instance(6));

    return instances;
}
