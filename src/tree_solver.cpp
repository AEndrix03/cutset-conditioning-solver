#include "tree_solver.hpp"
#include "graph.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {

    void check_assignment_size(const Assignment &assignment, int nvars) {
        if (static_cast<int>(assignment.size()) != nvars) {
            throw std::invalid_argument("Invalid assignment size");
        }
    }

    bool value_in_domain(const Domain &domain, Value value) {
        return std::find(domain.begin(), domain.end(), value) != domain.end();
    }

    /**
     * active[x] = true se x è ancora da risolvere nel problema residuo.
     *
     * Le variabili già assegnate sono considerate rimosse dal grafo residuo.
     */
    std::vector<bool> active_variables(const Assignment &assignment) {
        std::vector<bool> active(assignment.size(), false);

        for (Var x = 0; x < static_cast<Var>(assignment.size()); ++x) {
            active[x] = assignment[x] == UNASSIGNED;
        }

        return active;
    }

}

std::optional<Assignment> TreeSolver::solve(const CSP &csp, SolverStats *stats) const {
    return solve(csp, csp.empty_assignment(), stats);
}

std::optional<Assignment> TreeSolver::solve(
        const CSP &csp,
        const Assignment &partial_assignment,
        SolverStats *stats
) const {
    check_assignment_size(partial_assignment, csp.nvars());

    Assignment assignment = partial_assignment;

    /*
     * Validazione dei valori già assegnati.
     *
     * Se una variabile è fissata a un valore non presente nel suo dominio,
     * l'assegnamento parziale è impossibile.
     */
    for (Var x = 0; x < csp.nvars(); ++x) {
        if (assignment[x] == UNASSIGNED) {
            continue;
        }

        if (!value_in_domain(csp.domains()[x], assignment[x])) {
            return std::nullopt;
        }
    }

    // Prima controllo che il cutset, o comunque la parte già assegnata, sia internamente consistente
    if (!csp.is_consistent(assignment, stats)) {
        return std::nullopt;
    }

    // Caso limite: se l'assegnamento è già completo, basta la consistenza controllata sopra.
    if (csp.is_complete(assignment)) {
        return assignment;
    }

    Graph graph = csp.primal_graph();

    std::vector<bool> active = active_variables(assignment);

    /*
     * removed è il complemento di active: una variabile è fuori dal grafo residuo
     * se e solo se è già assegnata. Le funzioni di graph.hpp ragionano su removed.
     */
    std::vector<bool> removed(active.size());
    for (Var x = 0; x < static_cast<Var>(active.size()); ++x) {
        removed[x] = !active[x];
    }

    if (!is_forest_after_removing(graph, removed)) {
        throw std::invalid_argument("TreeSolver requires the residual primal graph to be a forest");
    }

    // Primo filtro dei domini: ogni variabile residua tiene solo i valori compatibili con le variabili già assegnate.
    std::optional<Domains> maybe_domains = build_initial_domains(csp, assignment, active, stats);

    if (!maybe_domains.has_value()) {
        return std::nullopt;
    }

    Domains domains = std::move(maybe_domains.value());

    std::vector<Var> order;
    std::vector<Var> parent;

    build_forest_order(graph, active, order, parent);

    // Propagazione bottom-up: elimina dai domini dei padri i valori che non hanno supporto nei figli.
    if (!enforce_directional_arc_consistency(csp, domains, order, parent, stats)) {
        return std::nullopt;
    }

    // Costruzione concreta della soluzione: top-down basta scendere dalle radici verso le foglie.
    if (!construct_solution(csp, assignment, domains, order, stats)) {
        return std::nullopt;
    }

    // Controllo volutamente ridondante per assicurarci che funzioni
    if (!csp.is_complete(assignment) || !csp.is_consistent(assignment, stats)) {
        return std::nullopt;
    }

    return assignment;
}

void TreeSolver::build_forest_order(
        const Graph &graph,
        const std::vector<bool> &active,
        std::vector<Var> &order,
        std::vector<Var> &parent
) const {
    int n = static_cast<int>(graph.size());

    order.clear();
    parent.assign(n, -1);

    std::vector<bool> visited(n, false);

    /*
     * Il residuo può essere una foresta, quindi può avere più componenti.
     * Ogni componente viene radicata nel primo nodo non visitato.
     */
    for (Var root = 0; root < n; ++root) {
        if (!active[root] || visited[root]) {
            continue;
        }

        std::vector<Var> stack;
        stack.push_back(root);
        visited[root] = true;
        parent[root] = -1;

        while (!stack.empty()) {
            Var x = stack.back();
            stack.pop_back();

            order.push_back(x);

            for (Var y: graph[x]) {
                if (!active[y] || visited[y]) {
                    continue;
                }

                visited[y] = true;
                parent[y] = x;
                stack.push_back(y);
            }
        }
    }
}

std::optional<Domains> TreeSolver::build_initial_domains(
        const CSP &csp,
        Assignment &assignment,
        const std::vector<bool> &active,
        SolverStats *stats
) const {
    Domains domains(csp.nvars());

    for (Var x = 0; x < csp.nvars(); ++x) {
        // Per le variabili già assegnate salvo il dominio.
        if (!active[x]) {
            domains[x].push_back(assignment[x]);
            continue;
        }

        /*
         * Provo ogni valore del dominio originale.
         * Tengo solo i valori compatibili con le variabili già fissate.
         */
        for (Value v: csp.domains()[x]) {
            assignment[x] = v;

            if (csp.is_consistent_var(x, assignment, stats)) {
                domains[x].push_back(v);
            }

            assignment[x] = UNASSIGNED;
        }

        /*
         * Se una variabile residua rimane senza valori, questa scelta del cutset
         * non può portare a una soluzione.
         */
        if (domains[x].empty()) {
            return std::nullopt;
        }
    }

    return domains;
}

bool TreeSolver::revise_parent_against_child(
        const CSP &csp,
        Domains &domains,
        Var parent,
        Var child,
        SolverStats *stats
) const {
    Domain pruned_parent_domain;

    // Un valore del padre è valido solo se ha almeno un supporto nel dominio del figlio.
    for (Value parent_value: domains[parent]) {
        bool supported = false;

        for (Value child_value: domains[child]) {
            if (csp.constraints_ok_pair(parent, parent_value, child, child_value, stats)) {
                supported = true;
                break;
            }
        }

        if (supported) {
            pruned_parent_domain.push_back(parent_value);
        }
    }

    domains[parent] = std::move(pruned_parent_domain);

    return !domains[parent].empty();
}

bool TreeSolver::enforce_directional_arc_consistency(
        const CSP &csp,
        Domains &domains,
        const std::vector<Var> &order,
        const std::vector<Var> &parent,
        SolverStats *stats
) const {
    /*
     * order è radice -> foglie.
     * Per propagare verso l'alto lo scorro al contrario.
     */
    for (int i = static_cast<int>(order.size()) - 1; i >= 0; --i) {
        Var child = order[i];
        Var p = parent[child];

        if (p == -1) {
            continue;
        }

        if (!revise_parent_against_child(csp, domains, p, child, stats)) {
            return false;
        }
    }

    return true;
}

bool TreeSolver::construct_solution(
        const CSP &csp,
        Assignment &assignment,
        const Domains &domains,
        const std::vector<Var> &order,
        SolverStats *stats
) const {
    /*
     * Scorro radice -> foglie.
     *
     * Quando arrivo su x, il padre è già assegnato.
     * Quindi basta scegliere un valore compatibile con ciò che è già fissato.
     */
    for (Var x: order) {
        bool found = false;

        for (Value v: domains[x]) {
            assignment[x] = v;

            if (stats != nullptr) stats->nodes++;

            if (csp.is_consistent_var(x, assignment, stats)) {
                found = true;
                break;
            }

            assignment[x] = UNASSIGNED;
        }

        if (!found) {
            assignment[x] = UNASSIGNED;
            return false;
        }
    }

    return true;
}
