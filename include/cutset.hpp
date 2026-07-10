// Solver CSP basato su Cutset Conditioning

#pragma once

#include "csp.hpp"
#include "tree_solver.hpp"

#include <optional>
#include <vector>

/**
 * Solver cutset conditioning.
 * Fonte: R&N 6.5.1, Dechter 2006 (par. 3.1).
 *
 * Idea:
 * - calcola un cycle-cutset C;
 * - enumera gli assegnamenti possibili delle variabili in C;
 * - per ogni assegnamento consistente, risolve il CSP residuo con TreeSolver;
 * - se TreeSolver trova una soluzione, allora è soluzione anche del CSP originale.
 *
 * Procedura dal libro (R&N 6.5.1):
 *   1. scegli un sottoinsieme S di variabili tale che, rimosso S, il grafo dei
 *      vincoli diventi un albero (S è il cycle cutset);
 *   2. per ogni assegnamento delle variabili di S consistente con i vincoli su S:
 *      a. elimina dai domini delle variabili restanti i valori inconsistenti con S;
 *      b. se il CSP rimanente ha soluzione, restituiscila insieme all'assegnamento di S.
 */
class CutsetSolver {
public:
    /**
     * Risolve il CSP scegliendo automaticamente il cutset con euristica greedy.
     *
     * @param csp CSP da risolvere
     * @param stats Statistiche solver
     * @return Assegnamento completo se esiste soluzione, altrimenti vuoto
     */
    std::optional<Assignment> solve(const CSP &csp, SolverStats *stats = nullptr) const;

    /**
     * Risolve il CSP usando un cutset già fornito.
     *
     * Utile per test, debug e relazione:
     * puoi confrontare cutset automatici e cutset scelti manualmente.
     *
     * @param csp CSP da risolvere
     * @param cutset Variabili da assegnare prima
     * @param stats Statistiche solver
     * @return Assegnamento completo se esiste soluzione, altrimenti vuoto
     */
    std::optional<Assignment> solve_with_cutset(
            const CSP &csp,
            const std::vector<Var> &cutset,
            SolverStats *stats = nullptr
    ) const;

private:
    /**
     * Ricorsione sulle variabili del cutset.
     *
     * È un backtracking limitato solo al cutset, non a tutte le variabili.
     * Quando il cutset è tutto assegnato, delega il residuo al TreeSolver.
     */
    std::optional<Assignment> condition(
            const CSP &csp,
            const std::vector<Var> &cutset,
            int index,
            Assignment &assignment,
            SolverStats *stats
    ) const;

    TreeSolver tree_solver_;
};
