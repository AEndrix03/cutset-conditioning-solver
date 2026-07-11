// In questa classe definisco i problemi da CSPLib

#pragma once

#include "csp.hpp"

#include <functional>
#include <string>
#include <vector>

/*
 * Un conflitto collega due riunioni che condividono almeno un partecipante: fra le due
 * deve passare il tempo di finire la prima e raggiungere la sede della seconda.
 * travel = slot di spostamento fra le due sedi (ogni riunione dura 1 slot).
 */
struct MeetingConflict {
    Var first;
    Var second;
    int travel;
};

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

/* Meeting scheduling (CSPLib prob046): una variabile per riunione, il valore è lo slot
 iniziale. Due riunioni con un partecipante comune non possono sovrapporsi e devono
 lasciare il tempo di viaggio: |X_i - X_j| >= 1 + travel. Gli impegni privati degli
 agenti sono slot tolti dal dominio, non vincoli a parte.*/
CSP make_meeting_scheduling(
        const std::string &name,
        const Domains &domains,
        const std::vector<MeetingConflict> &conflicts
);

bool validate_meeting_scheduling(
        const Assignment &assignment,
        const Domains &domains,
        const std::vector<MeetingConflict> &conflicts
);

ProblemInstance make_meeting_tree_instance(int n_meetings);

ProblemInstance make_meeting_single_cycle_instance(int n_meetings);

ProblemInstance make_meeting_unsat_instance(int n_meetings);

ProblemInstance make_meeting_hard_sat_instance(int n_meetings);

std::vector<ProblemInstance> make_default_instances();
