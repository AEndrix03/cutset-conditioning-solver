#include "experiment.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>


namespace {

    bool has_arg(int argc, char **argv, const std::string &key) {
        for (int i = 1; i < argc; ++i) {
            if (argv[i] == key) {
                return true;
            }
        }

        return false;
    }

    std::string get_arg(
            int argc,
            char **argv,
            const std::string &key,
            const std::string &default_value
    ) {
        for (int i = 1; i + 1 < argc; ++i) {
            if (argv[i] == key) {
                return argv[i + 1];
            }
        }

        return default_value;
    }

    int get_int_arg(
            int argc,
            char **argv,
            const std::string &key,
            int default_value
    ) {
        std::string value = get_arg(argc, argv, key, "");

        if (value.empty()) {
            return default_value;
        }

        return std::stoi(value);
    }

    void print_usage() {
        std::cout
                << "Cutset CSP Solver\n\n"

                << "Uso batch:\n"
                << "  ./cutset_csp --all --repeat 5\n\n"

                << "Esecuzioni singole:\n"
                << "  ./cutset_csp --problem nqueens --n 8 --solver cutset\n"
                << "  ./cutset_csp --problem nqueens --n 8 --solver bt\n\n"

                << "  ./cutset_csp --problem quasigroup --order 4 --solver cutset\n"
                << "  ./cutset_csp --problem quasigroup --order 5 --solver bt\n\n"

                << "  ./cutset_csp --problem graceful --instance path --n 6 --solver cutset\n"
                << "  ./cutset_csp --problem graceful --instance star --leaves 6 --solver bt\n\n"

                << "Opzioni:\n"
                << "  --all                         esegue tutte le istanze di default\n"
                << "  --problem nqueens|quasigroup|graceful\n"
                << "  --instance path|star          solo per graceful\n"
                << "  --solver bt|cutset\n"
                << "  --repeat N                    ripete N volte e prende la mediana\n"
                << "  --help                        stampa questo messaggio\n";
    }

    ProblemInstance make_instance_from_args(int argc, char **argv) {
        std::string problem = get_arg(argc, argv, "--problem", "");

        if (problem == "nqueens") {
            int n = get_int_arg(argc, argv, "--n", 8);
            return make_n_queens_instance(n);
        }

        if (problem == "quasigroup") {
            int order = get_int_arg(argc, argv, "--order", 4);
            return make_quasigroup_instance(order);
        }

        if (problem == "graceful") {
            std::string instance = get_arg(argc, argv, "--instance", "path");

            if (instance == "path") {
                int n = get_int_arg(argc, argv, "--n", 6);
                return make_graceful_path_instance(n);
            }

            if (instance == "star") {
                int leaves = get_int_arg(argc, argv, "--leaves", 6);
                return make_graceful_star_instance(leaves);
            }

            throw std::invalid_argument("Unknown graceful instance: " + instance);
        }

        throw std::invalid_argument("Unknown problem: " + problem);
    }

}


int main(int argc, char **argv) {
    try {
        if (argc == 1 || has_arg(argc, argv, "--help")) {
            print_usage();
            return 0;
        }

        int repeat = get_int_arg(argc, argv, "--repeat", 1);

        std::vector<ExperimentResult> results;

        if (has_arg(argc, argv, "--all")) {
            results = run_default_experiments(repeat);
        } else {
            ProblemInstance instance = make_instance_from_args(argc, argv);

            std::string solver_arg = get_arg(argc, argv, "--solver", "cutset");
            SolverKind solver = solver_from_string(solver_arg);

            results.push_back(
                    run_repeated_experiment(instance, solver, repeat)
            );
        }

        print_results(results);

        return 0;

    } catch (const std::exception &ex) {
        std::cerr << "Errore: " << ex.what() << "\n\n";
        print_usage();
        return 1;
    }
}
