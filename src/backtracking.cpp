#include "backtracking.hpp"

/*
 * Logica:
 * - Crea un assegnamento vuoto
 * - Chiama backtrack, funzione ricorsiva
 * - Restituisce la soluzione, altrimenti nulla
 */
std::optional<Assignment> BacktrackingSolver::solve(const CSP &csp, SolverStats *stats) const {
    Assignment assignment = csp.empty_assignment();

    bool ok = backtrack(csp, assignment, stats);

    if (ok) {
        return assignment;
    }

    return std::nullopt;
}

/*
 * SCEGLI-VARIABILE-NON-ASSEGNATA (R&N 6.3.1).
 * Versione base: ordine statico, ritorna la prima variabile libera, senza euristiche
 * come MRV o grado. Ritorna -1 se sono tutte assegnate.
 */
Var BacktrackingSolver::select_unassigned_variable(const Assignment &assignment) const {
    for (Var x = 0; x < static_cast<Var>(assignment.size()); ++x) {
        if (assignment[x] == UNASSIGNED) {
            return x;
        }
    }

    return -1;
}

/*
 * Backtracking ricorsivo. Fonte: R&N 6.3, fig. 6.5.
 *
 * Pseudocodice dal libro:
 *
 *   function RICERCA-BACKTRACKING(csp) returns una soluzione, o fallimento
 *       return BACKTRACKING(csp, { })
 *
 *   function BACKTRACKING(csp, assegnamento) returns una soluzione, o fallimento
 *       if assegnamento è completo then return assegnamento
 *       var <- SCEGLI-VARIABILE-NON-ASSEGNATA(csp, assegnamento)
 *       for each valore in ORDINA-VALORI-DOMINIO(csp, var, assegnamento) do
 *           if valore è consistente con assegnamento then
 *               aggiungi {var = valore} ad assegnamento
 *               inferenze <- INFERENZA(csp, var, assegnamento)
 *               if inferenze != fallimento then
 *                   aggiungi inferenze a csp
 *                   risultato <- BACKTRACKING(csp, assegnamento)
 *                   if risultato != fallimento then return risultato
 *                   rimuovi inferenze da csp
 *               rimuovi {var = valore} da assegnamento
 *       return fallimento
 *
 * Nota: qui non applico il passo INFERENZA in quanto facoltativa; il resto è identico. Inoltre in questo caso l'assegnamento
 * lo modello da riferimento, returno solo se risolvo
 */
bool BacktrackingSolver::backtrack(const CSP &csp, Assignment &assignment, SolverStats *stats) const {
    if (csp.is_complete(assignment)) {
        return csp.is_consistent(assignment, stats);
    }

    Var x = select_unassigned_variable(assignment);

    for (Value v: csp.domains()[x]) {
        assignment[x] = v;

        if (stats != nullptr) stats->nodes++;

        if (csp.is_consistent_var(x, assignment, stats)) {
            if (backtrack(csp, assignment, stats)) return true;
        }

        assignment[x] = UNASSIGNED;
    }

    return false;
}
