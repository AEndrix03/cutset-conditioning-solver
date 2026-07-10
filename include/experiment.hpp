#pragma once

#include "problems.hpp"

#include <string>
#include <vector>

/*
 * experiment.hpp
 *
 * Qui rimane solo la parte sperimentale:
 *   - scelta del solver;
 *   - esecuzione;
 *   - raccolta statistiche;
 *   - stampa.
 *
 * La costruzione delle istanze sta in problems.hpp/cpp.
 */

enum class SolverKind {
    Backtracking,
    Cutset
};

/**
 * Risultato di una run sperimentale.
 * Una struct = una riga del CSV.
 */
struct ExperimentResult {
    std::string problem;
    std::string instance;
    std::string solver;

    int nvars = 0;
    int constraints = 0;
    int edges = 0;

    int cutset_size = 0;
    long long nodes = 0;
    long long constraint_checks = 0;
    long long cutset_assignments = 0;

    bool solved = false;
    bool valid = false;

    int repeat = 1;
    double time_ms = 0.0;
    double min_time_ms = 0.0;
};

std::string solver_name(SolverKind solver);

SolverKind solver_from_string(const std::string &name);

ExperimentResult run_single_experiment(
        const ProblemInstance &instance,
        SolverKind solver
);

ExperimentResult run_repeated_experiment(
        const ProblemInstance &instance,
        SolverKind solver,
        int repeat
);

std::vector<ExperimentResult> run_default_experiments(int repeat);

void print_results(const std::vector<ExperimentResult> &results);
