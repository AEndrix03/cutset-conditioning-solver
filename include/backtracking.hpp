// Questa classe contiene il solver per backtracking

#pragma once

#include "csp.hpp" // Eredita tipi base

#include <optional>

/**
 * L'algoritmo è completo, ossia, se esiste soluzione la trova, altrimenti dimostra la non esistenza.
 */
class BacktrackingSolver {
public:
    /**
     * @param csp CSP
     * @param stats Statistiche
     * @return Assegnamento se esiste soluzione, altrimenti vuoto
     */
    std::optional<Assignment> solve(const CSP& csp, SolverStats* stats = nullptr) const;

private:
    // Core, fa il backtracking
    bool backtrack(const CSP& csp, Assignment& assignment, SolverStats* stats) const;

    // Scorre le variabili, trova la prima non assegnata e ritorna
    Var select_unassigned_variable(const Assignment& assignment) const;
};