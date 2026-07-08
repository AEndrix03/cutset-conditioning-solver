#include "experiment.hpp"

#include "backtracking.hpp"
#include "cutset.hpp"
#include "graph.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>


namespace {

    int count_edges(const Graph &graph) {
        int sum = 0;

        for (const auto &adjacency: graph) {
            sum += static_cast<int>(adjacency.size());
        }

        return sum / 2;
    }

    void ensure_parent_directory(const std::string &path) {
        std::filesystem::path p(path);

        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    }

    const char *bool_text(bool value) {
        return value ? "true" : "false";
    }

}

std::string solver_name(SolverKind solver) {
    switch (solver) {
        case SolverKind::Backtracking:
            return "bt";

        case SolverKind::Cutset:
            return "cutset";
    }

    throw std::invalid_argument("Unknown solver kind");
}

SolverKind solver_from_string(const std::string &name) {
    if (name == "bt" || name == "backtracking") {
        return SolverKind::Backtracking;
    }

    if (name == "cutset") {
        return SolverKind::Cutset;
    }

    throw std::invalid_argument("Unknown solver: " + name);
}

ExperimentResult run_single_experiment(
        const ProblemInstance &instance,
        SolverKind solver,
        long long timeout_ms
) {
    SolverStats stats;
    std::optional<Assignment> solution;

    auto start = std::chrono::steady_clock::now();

    /*
     * Nota:
     * il timeout qui viene solo segnalato a posteriori.
     * Per un timeout vero bisognerebbe passare una deadline nelle ricorsioni.
     */
    if (solver == SolverKind::Backtracking) {
        BacktrackingSolver backtracking;
        solution = backtracking.solve(instance.csp, &stats);
    } else if (solver == SolverKind::Cutset) {
        CutsetSolver cutset;
        solution = cutset.solve(instance.csp, &stats);
    } else {
        throw std::invalid_argument("Unsupported solver");
    }

    auto end = std::chrono::steady_clock::now();

    double elapsed_ms =
            std::chrono::duration<double, std::milli>(end - start).count();

    stats.milliseconds = elapsed_ms;

    if (timeout_ms > 0 && elapsed_ms > static_cast<double>(timeout_ms)) {
        stats.timed_out = true;
    }

    bool valid = false;

    if (solution.has_value()) {
        valid = instance.validate(solution.value());
    }

    Graph graph = instance.csp.primal_graph();

    ExperimentResult result;

    result.problem = instance.problem;
    result.instance = instance.instance;
    result.solver = solver_name(solver);

    result.nvars = instance.csp.nvars();
    result.constraints = static_cast<int>(instance.csp.constraints().size());
    result.edges = count_edges(graph);

    result.cutset_size = stats.cutset_size;
    result.nodes = stats.nodes;
    result.constraint_checks = stats.constraint_checks;
    result.cutset_assignments = stats.cutset_assignments;

    result.solved = solution.has_value();
    result.valid = valid;
    result.timed_out = stats.timed_out;

    result.repeat = 1;
    result.time_ms = elapsed_ms;
    result.min_time_ms = elapsed_ms;

    return result;
}

ExperimentResult run_repeated_experiment(
        const ProblemInstance &instance,
        SolverKind solver,
        int repeat,
        long long timeout_ms
) {
    if (repeat <= 1) {
        return run_single_experiment(instance, solver, timeout_ms);
    }

    std::vector<ExperimentResult> runs;
    runs.reserve(repeat);

    for (int i = 0; i < repeat; ++i) {
        runs.push_back(run_single_experiment(instance, solver, timeout_ms));
    }

    std::sort(
            runs.begin(),
            runs.end(),
            [](const ExperimentResult &a, const ExperimentResult &b) {
                return a.time_ms < b.time_ms;
            }
    );

    ExperimentResult median = runs[runs.size() / 2];

    median.repeat = repeat;
    median.min_time_ms = runs.front().time_ms;

    for (const auto &run: runs) {
        if (run.timed_out) {
            median.timed_out = true;
            break;
        }
    }

    return median;
}

std::vector<ExperimentResult> run_default_experiments(
        int repeat,
        long long timeout_ms
) {
    std::vector<ProblemInstance> instances = make_default_instances();

    std::vector<SolverKind> solvers = {
            SolverKind::Backtracking,
            SolverKind::Cutset
    };

    std::vector<ExperimentResult> results;

    for (const auto &instance: instances) {
        for (SolverKind solver: solvers) {
            results.push_back(
                    run_repeated_experiment(
                            instance,
                            solver,
                            repeat,
                            timeout_ms
                    )
            );
        }
    }

    return results;
}

void write_results_csv(
        const std::string &path,
        const std::vector<ExperimentResult> &results
) {
    ensure_parent_directory(path);

    std::ofstream out(path);

    if (!out) {
        throw std::runtime_error("Cannot open CSV file: " + path);
    }

    out << "problem,"
        << "instance,"
        << "solver,"
        << "nvars,"
        << "constraints,"
        << "edges,"
        << "cutset_size,"
        << "nodes,"
        << "constraint_checks,"
        << "cutset_assignments,"
        << "solved,"
        << "valid,"
        << "timed_out,"
        << "repeat,"
        << "time_ms,"
        << "min_time_ms"
        << "\n";

    for (const auto &result: results) {
        out << result.problem << ","
            << result.instance << ","
            << result.solver << ","
            << result.nvars << ","
            << result.constraints << ","
            << result.edges << ","
            << result.cutset_size << ","
            << result.nodes << ","
            << result.constraint_checks << ","
            << result.cutset_assignments << ","
            << bool_text(result.solved) << ","
            << bool_text(result.valid) << ","
            << bool_text(result.timed_out) << ","
            << result.repeat << ","
            << std::fixed << std::setprecision(3) << result.time_ms << ","
            << std::fixed << std::setprecision(3) << result.min_time_ms
            << "\n";
    }
}

void print_results(const std::vector<ExperimentResult> &results) {
    std::cout
            << std::left
            << std::setw(13) << "problem"
            << std::setw(28) << "instance"
            << std::setw(8) << "solver"
            << std::setw(7) << "nvars"
            << std::setw(7) << "edges"
            << std::setw(8) << "cutset"
            << std::setw(10) << "nodes"
            << std::setw(10) << "checks"
            << std::setw(8) << "valid"
            << std::setw(10) << "time_ms"
            << "\n";

    for (const auto &result: results) {
        std::cout
                << std::left
                << std::setw(13) << result.problem
                << std::setw(28) << result.instance
                << std::setw(8) << result.solver
                << std::setw(7) << result.nvars
                << std::setw(7) << result.edges
                << std::setw(8) << result.cutset_size
                << std::setw(10) << result.nodes
                << std::setw(10) << result.constraint_checks
                << std::setw(8) << bool_text(result.valid)
                << std::setw(10) << std::fixed << std::setprecision(3) << result.time_ms
                << "\n";
    }
}
