#pragma once

#include "csp.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/*
 * problems.hpp
 *
 * Qui tengo separata la modellazione dei problemi dal codice dei solver.
 * Il solver vede solo un CSP binario generico; ogni famiglia di problemi sa invece:
 *   - come costruire le proprie istanze;
 *   - come validare una soluzione senza riusare i vincoli del CSP;
 *   - a quale problema CSPLib corrisponde.
 */

using Edge = std::pair<Var, Var>;

/**
 * Istanza concreta usata negli esperimenti.
 */
struct ProblemInstance {
    std::string problem;
    std::string instance;
    CSP csp;
    std::function<bool(const Assignment &)> validate;

    ProblemInstance(
            std::string problem,
            std::string instance,
            CSP csp,
            std::function<bool(const Assignment &)> validate
    );
};

/**
 * Astrazione comune alle famiglie di problemi CSPLib.
 */
class ProblemFamily {
public:
    virtual ~ProblemFamily() = default;

    virtual std::string name() const = 0;

    virtual std::string csplib_id() const = 0;

    virtual std::string description() const = 0;

    virtual std::vector<ProblemInstance> default_instances() const = 0;
};

class NQueensFamily final : public ProblemFamily {
public:
    std::string name() const override;

    std::string csplib_id() const override;

    std::string description() const override;

    std::vector<ProblemInstance> default_instances() const override;
};

CSP make_n_queens(int n);

bool validate_n_queens(const Assignment &assignment);

ProblemInstance make_n_queens_instance(int n);

class QuasigroupCompletionFamily final : public ProblemFamily {
public:
    std::string name() const override;

    std::string csplib_id() const override;

    std::string description() const override;

    std::vector<ProblemInstance> default_instances() const override;
};

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

class GracefulGraphsFamily final : public ProblemFamily {
public:
    std::string name() const override;

    std::string csplib_id() const override;

    std::string description() const override;

    std::vector<ProblemInstance> default_instances() const override;
};

/**
 * Costruisce il CSP per Graceful Graphs.
 *
 * Modello binario usato:
 *   - una variabile per ogni arco del grafo;
 *   - il valore della variabile è una coppia codificata (label_u, label_v),
 *     cioè le etichette assegnate ai due estremi dell'arco;
 *   - i vincoli binari tra coppie di archi impongono:
 *       1) coerenza delle etichette sui vertici condivisi;
 *       2) etichette diverse su vertici distinti;
 *       3) differenze di arco tutte diverse.
 *
 * Nota: le istanze usate non hanno vertici isolati, quindi ogni vertice compare
 * in almeno una variabile-arco.
 */
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

std::vector<Edge> graceful_path_edges(int n_vertices);

std::vector<Edge> graceful_star_edges(int n_leaves);

ProblemInstance make_graceful_path_instance(int n_vertices);

ProblemInstance make_graceful_star_instance(int n_leaves);


std::vector<std::unique_ptr<ProblemFamily>> make_problem_families();

std::vector<ProblemInstance> make_default_instances();
