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

                << "  ./cutset_csp --problem meeting --instance tree --meetings 40 --solver cutset\n"
                << "  ./cutset_csp --problem meeting --instance single_cycle --meetings 40 --solver bt\n"
                << "  ./cutset_csp --problem meeting --instance unsat --meetings 40 --solver cutset\n\n"

                << "Opzioni:\n"
                << "  --all                              esegue tutte le istanze di default\n"
                << "  --problem nqueens|quasigroup|meeting\n"
                << "  --instance tree|single_cycle|unsat solo per meeting\n"
                << "  --solver bt|cutset\n"
                << "  --repeat N                    ripete N volte e prende la mediana\n"
                << "  --help                        stampa questo messaggio\n";
    }

    // Costruisce l'istanza giusta a partire dai flag (--problem, --n, --order, --instance...).
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

        if (problem == "meeting") {
            std::string instance = get_arg(argc, argv, "--instance", "tree");
            int meetings = get_int_arg(argc, argv, "--meetings", 40);

            if (instance == "tree") {
                return make_meeting_tree_instance(meetings);
            }

            if (instance == "single_cycle") {
                return make_meeting_single_cycle_instance(meetings);
            }

            if (instance == "unsat") {
                return make_meeting_unsat_instance(meetings);
            }

            throw std::invalid_argument("Unknown meeting instance: " + instance);
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
            // Batch: tutte le istanze di default, entrambi i solver (per la relazione).
            results = run_default_experiments(repeat);
        } else {
            // Singola run scelta dai flag.
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
