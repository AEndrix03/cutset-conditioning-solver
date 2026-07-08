// Solver CSP per grafo primale aciclico, quindi albero / foresta

#pragma once

#include "csp.hpp"

#include <optional>
#include <vector>

class TreeSolver {
public:

    /**
     * Solver del CSP, risolve assumendo che nessuna variabile sia stata assegnata.
     *
     * Funziona solo se il grafo primale del CSP è una foresta
     *
     * @param csp Problema da risolvere
     * @param stats Statistiche
     * @return Assegnamento completo e consistente se esiste, altrimenti vuoto (optional)
     */
    std::optional <Assignment> solve(const CSP &csp, SolverStats *stats = nullptr) const;

    /**
     * Solver del CSP, partendo da un assegnamento parziole.
     *
     * Le variabili già assegnate sono considerate fisse, mentre le altre devono formare una foresta nel grafo residuo.
     *
     * @param csp  Problema da risolvere
     * @param assignment Assegnamento parziale iniziale
     * @param stats Stats
     * @return Assegnamento completo e consistente se esiste, altrimenti vuoto
     */
    std::optional <Assignment> solve(const CSP &csp, const Assignment &assignment, SolverStats *stats = nullptr) const;

private:
    /**
     * Costruisce un ordine radice -> foglie per ogni componente della foresta.
     *
     * parent[x] = padre di x nella foresta radicata.
     * Se x è radice, parent[x] = -1.
     */
    void build_forest_order(
            const Graph &graph,
            const std::vector<bool> &active,
            std::vector <Var> &order,
            std::vector <Var> &parent
    ) const;

    /**
     * Costruisce i domini iniziali delle variabili non assegnate.
     *
     * Ogni dominio viene filtrato rispetto alle variabili già assegnate.
     * Questo è il primo effetto del conditioning: il cutset riduce i domini residui.
     */
    std::optional <Domains> build_initial_domains(
            const CSP &csp,
            Assignment &assignment,
            const std::vector<bool> &active,
            SolverStats *stats
    ) const;

    /**
     * Potatura bottom-up.
     *
     * Per ogni valore del padre controllo che esista almeno un valore compatibile
     * nel figlio. Se non esiste supporto, quel valore del padre viene eliminato.
     */
    bool revise_parent_against_child(
            const CSP &csp,
            Domains &domains,
            Var parent,
            Var child,
            SolverStats *stats
    ) const;

    /**
     * Rende i domini consistenti dal basso verso l'alto.
     *
     * Su una foresta basta processare i nodi in ordine inverso:
     * prima le foglie, poi i loro padri, fino alle radici.
     */
    bool enforce_directional_arc_consistency(
            const CSP &csp,
            Domains &domains,
            const std::vector <Var> &order,
            const std::vector <Var> &parent,
            SolverStats *stats
    ) const;

    /**
     * Dopo la potatura bottom-up, costruisce una soluzione top-down.
     *
     * Scelgo un valore per la radice e poi, per ogni figlio,
     * scelgo un valore compatibile con il padre già assegnato.
     */
    bool construct_solution(
            const CSP &csp,
            Assignment &assignment,
            const Domains &domains,
            const std::vector <Var> &order,
            SolverStats *stats
    ) const;
};
