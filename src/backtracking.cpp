#include "backtracking.hpp"

/*
 * Logica:
 * - Crea un assegnamento vuoto
 * - Chiama in ricorsione backtrack
 * - Restituisce la soluzione, altrimenti nulla
 */
std::optional<Assignment> BacktrackingSolver::solve(const CSP &csp, SolverStats *stats) const {
    Assignment assignment = csp.empty_assignment();

    bool ok = backtrack(csp, assignment, stats);

    if (ok) {
        if (stats != nullptr) stats->solved = true;
        return assignment;
    }

    if (stats != nullptr) stats->solved = false;
    return std::nullopt;
}

Var BacktrackingSolver::select_unassigned_variable(const Assignment& assignment) const {
    for (Var x = 0; x < static_cast<Var>(assignment.size()); ++x) {
        if (assignment[x] == UNASSIGNED) {
            return x;
        }
    }

    return -1;
}

/*
 * Se assignment completo => true
 * x = prima var non assegnata
 * quindi, per ogni v nel dominio x:
 * assegno X con v, se consistente procedo con i rami, altrimenti metto non assegnata e continuo con v successivo
 */
bool BacktrackingSolver::backtrack(const CSP &csp, Assignment &assignment, SolverStats *stats) const {
    if (csp.is_complete(assignment)) {
        return csp.is_consistent(assignment, stats);
    }

    Var x = select_unassigned_variable(assignment);

    for (Value v : csp.domains()[x]) {
        assignment[x] = v;

        if (stats != nullptr) stats->nodes++;

        if (csp.is_consistent_var(x, assignment, stats)) {
            if (backtrack(csp, assignment, stats)) return true;
        }

        assignment[x] = UNASSIGNED;
    }

    return false;
}