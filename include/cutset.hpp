// Solver CSP basato su Cutset Conditioning

#pragma once

#include "csp.hpp"
#include "tree_solver.hpp"

#include <optional>
#include <vector>

/**
 * Solver principale per cutset conditioning.
 *
 * Idea:
 * - calcola un cycle-cutset C;
 * - enumera gli assegnamenti possibili delle variabili in C;
 * - per ogni assegnamento consistente, risolve il CSP residuo con TreeSolver;
 * - se TreeSolver trova una soluzione, allora è soluzione anche del CSP originale.
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
    std::optional <Assignment> solve(const CSP &csp, SolverStats *stats = nullptr) const;

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
    std::optional <Assignment> solve_with_cutset(
            const CSP &csp,
            const std::vector <Var> &cutset,
            SolverStats *stats = nullptr
    ) const;

private:
    /**
     * Ricorsione sulle variabili del cutset.
     *
     * È un backtracking limitato solo al cutset, non a tutte le variabili.
     * Quando il cutset è tutto assegnato, delega il residuo al TreeSolver.
     */
    std::optional <Assignment> condition(
            const CSP &csp,
            const std::vector <Var> &cutset,
            int index,
            Assignment &assignment,
            SolverStats *stats
    ) const;

    TreeSolver tree_solver_;
};