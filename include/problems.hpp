// In questa classe definisco i problemi da CSPLib

#pragma once

#include "csp.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

/*
 * Qui tengo separata la modellazione dei problemi dal codice dei solver: il solver
 * vede solo un CSP binario generico, mentre ogni problema sa:
 *   - come costruire le proprie istanze;
 *   - come validare una soluzione senza riusare i vincoli del CSP.
 */

using Edge = std::pair<Var, Var>;

/**
 * Istanza concreta usata negli esperimenti.
 * Impacchetta il CSP e il suo validatore, così il solver non deve sapere
 * di che problema si tratta: risolve il csp e poi chiama validate.
 */
struct ProblemInstance {
    std::string problem;   // famiglia, es. "nqueens"
    std::string instance;  // istanza specifica, es. "n_8"
    CSP csp;
    std::function<bool(const Assignment &)> validate; // controllo a sé, non riusa i vincoli

    ProblemInstance(
            std::string problem,
            std::string instance,
            CSP csp,
            std::function<bool(const Assignment &)> validate
    );
};

/*
 * Ogni problema ha lo stesso trio di funzioni:
 *   make_*          costruisce il CSP (variabili, domini, vincoli);
 *   validate_*      ricontrolla una soluzione da zero, senza riusare i vincoli.
 *                   È un oracolo indipendente: se la modellazione ha un bug, si vede qui;
 *   make_*_instance mette insieme CSP + validatore in una ProblemInstance pronta.
 */

// N-Regine (CSPLib prob054): una regina per colonna, mai due sulla stessa riga o diagonale.
CSP make_n_queens(int n);

bool validate_n_queens(const Assignment &assignment);

ProblemInstance make_n_queens_instance(int n);

// Quasigroup completion (CSPLib prob067): completa un quadrato latino parziale.
// clues = celle già fissate (0 o UNASSIGNED = libera).
CSP make_quasigroup_completion(
        int order,
        const std::vector<std::vector<Value>> &clues = {}
);

bool validate_quasigroup_completion(
        const Assignment &assignment,
        int order,
        const std::vector<std::vector<Value>> &clues = {}
);

ProblemInstance make_quasigroup_instance(int order);

// Graceful graphs (CSPLib prob053): etichetta i vertici così che le differenze sugli
// archi siano tutte diverse. Qui la variabile è l'arco (coppia di etichette codificata).
CSP make_graceful_graph(
        const std::string &name,
        int n_vertices,
        const std::vector<Edge> &edges
);

bool validate_graceful_graph(
        const Assignment &assignment,
        int n_vertices,
        const std::vector<Edge> &edges
);

// Due topologie di grafo usate come istanze: cammino e stella.
std::vector<Edge> graceful_path_edges(int n_vertices);

std::vector<Edge> graceful_star_edges(int n_leaves);

ProblemInstance make_graceful_path_instance(int n_vertices);

ProblemInstance make_graceful_star_instance(int n_leaves);


// Le istanze di default usate dal benchmark (--all): 3 problemi x 2 istanze.
std::vector<ProblemInstance> make_default_instances();
