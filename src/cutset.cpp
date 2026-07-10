#include "cutset.hpp"
#include "graph.hpp"

#include <stdexcept>

namespace {

    void check_var(Var x, int nvars) {
        if (x < 0 || x >= nvars) {
            throw std::out_of_range("Var index is out of range");
        }
    }

    /**
     * Costruisce il vettore removed richiesto dalle funzioni sul grafo.
     * removed[x] = true significa: x viene tolta dal grafo perché appartiene al cutset.
     */
    std::vector<bool> removed_from_cutset(const std::vector<Var> &cutset, int nvars) {
        std::vector<bool> removed(nvars, false);

        for (Var x: cutset) {
            check_var(x, nvars);

            if (removed[x]) {
                throw std::invalid_argument("Cutset contains duplicate variables");
            }

            removed[x] = true;
        }

        return removed;
    }

}

std::optional<Assignment> CutsetSolver::solve(const CSP &csp, SolverStats *stats) const {
    Graph graph = csp.primal_graph();

    // Scelta automatica del cutset. greedy_cycle_cutset non garantisce il cutset minimo (NP-hard, R&N 6.5.1)
    std::vector<Var> cutset = greedy_cycle_cutset(graph);

    return solve_with_cutset(csp, cutset, stats);
}

std::optional<Assignment> CutsetSolver::solve_with_cutset(
        const CSP &csp,
        const std::vector<Var> &cutset,
        SolverStats *stats
) const {
    Graph graph = csp.primal_graph();

    // rimuovendo il cutset, il grafo residuo deve essere una foresta.
    std::vector<bool> removed = removed_from_cutset(cutset, csp.nvars());

    if (!is_forest_after_removing(graph, removed)) {
        throw std::invalid_argument("The given cutset does not make the residual graph a forest");
    }

    if (stats != nullptr) {
        stats->cutset_size = static_cast<int>(cutset.size());
    }

    Assignment assignment = csp.empty_assignment();

    // Se il cutset è vuoto il CSP è già una foresta: delega al TreeSolver.
    return condition(csp, cutset, 0, assignment, stats);
}

std::optional<Assignment> CutsetSolver::condition(
        const CSP &csp,
        const std::vector<Var> &cutset,
        int index,
        Assignment &assignment,
        SolverStats *stats
) const {
    // Ho assegnato tutte le variabili del cutset: rimane una foresta => TreeSolver
    if (index == static_cast<int>(cutset.size())) {
        if (stats != nullptr) {
            stats->cutset_assignments++;
        }

        return tree_solver_.solve(csp, assignment, stats);
    }

    Var x = cutset[index];

    // Backtracking solo sulle variabili del cutset (fase di "conditioning", R&N 6.5.1)
    for (Value v: csp.domains()[x]) {
        assignment[x] = v;

        if (stats != nullptr) {
            stats->nodes++;
        }

        /*
         * Consistenza locale della variabile appena assegnata, controllata subito.
         * Se x=v è già incompatibile con le variabili del cutset assegnate prima,
         * NON entro nella ricorsione: così salto in blocco tutto il sottoalbero che
         * ne sarebbe derivato (gli assegnamenti delle altre variabili del cutset e la
         * risoluzione ad albero). Poi ripristino x e provo il v dopo.
         */
        if (csp.is_consistent_var(x, assignment, stats)) {
            std::optional<Assignment> result = condition(csp, cutset, index + 1, assignment, stats);

            if (result.has_value()) {
                return result;
            }
        }

        assignment[x] = UNASSIGNED;
    }

    return std::nullopt;
}
