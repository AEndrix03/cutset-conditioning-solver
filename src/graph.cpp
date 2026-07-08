#include "graph.hpp"

#include <queue>
#include <stdexcept>

namespace {

    void check_graph(const Graph& graph) {
        int n = static_cast<int>(graph.size()); // normalmente sarebbe size_t
        for (Var x = 0; x < n; ++x) { // Mi sposto sulle variabili una alla volta
            for (Var y : graph[x]) {
                if (y < 0 || y >= n)
                    throw std::out_of_range("Graph contains invalid variable index");
            }
        }
    }

    void check_removed_size(const Graph& graph, const std::vector<bool>& removed) {
        if (graph.size() != removed.size()) { // CONVENZIONE
            throw std::invalid_argument("removed vector has invalid size");
        }
    }

    void check_var(Var x, int nvars) {
        if (x < 0 || x >= nvars) {
            throw std::out_of_range("Var index is out of range");
        }
    }

    /**
     * DFS usato per rilevare cicli in un grafo non orientato. Si passa parent per evitare di considerare ciclo la sorgente di inizio
     * @return
     */
    bool check_cycle(const Graph& graph, const std::vector<bool>& removed, std::vector<bool>& visited, Var x, Var parent) {
        visited[x] = true;

        for (Var y : graph[x]) {
            if (removed[y]) continue; // rimosso, va avanti

            if (!visited[y]) {
                if (check_cycle(graph, removed, visited, y, x)) return true;
            } else if (y != parent) {
                return true;
            }
        }

        return false;
    }

    bool is_forest_after_removing(const Graph& graph, const std::vector<bool>& removed) {
        // Validazioni in entrata
        check_graph(graph);
        check_removed_size(graph, removed);

        int n = static_cast<int>(graph.size()); // come sopra, nativo size_t
        std::vector<bool> visited(n, false);

        for (Var x = 0; x < n; ++x) {
            if (removed[x]) continue;

            if (!visited[x]) {
                if (check_cycle(graph, removed, visited, x, -1)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool is_forest(const Graph& graph) {
        std::vector<bool> removed(graph.size(), false);
        return is_forest_after_removing(graph, removed);
    }

    int residual_degree(const Graph& graph, Var x, const std::vector<bool>& removed) {
        check_graph(graph);
        check_removed_size(graph, removed);
        check_var(x, static_cast<int>(graph.size()));

        if (removed[x]) return 0;

        int degree = 0;

        for (Var x : graph[x]) {
            if (!removed[y]) ++degree;
        }

        return degree;
    }

    std::vector<bool> two_core_vertices(const Graph& graph, const std::vector<bool>& removed) {
        check_graph(graph);
        check_removed_size(graph, removed);

        int n = static_cast<int>(graph.size());

        std::vector<bool> in_core(n, false);
        std::vector<int> degree(n, 0);
        std::queue<Var> q;

        for (Var x = 0; x < n; ++x) {
            if (!removed[x]) {
                inc_core[x] = true;
                degree[x] = residual_degree(graph, x, removed);
            }
        }

        for (Var x = 0; x < n; ++x) {
            if (in_core[x] && degree[x] <= 1) {
                q.push(x);
            }
        }

        while(!q.empty()) {
            Var x = q.front();
            q.pop();

            if (!in_core[x]) {
                continue;
            }

            in_core[x] = false;

            for (Var y: graph[x]) {
                if (!in_core[y]) continue;

                --degree[y];

                if (degree[y] <= 1) {
                    q.push(y);
                }
            }
        }

        return in_core;
    }

    std::vector<Var> greedy_cycle_cutset(const Graph& graph) {
        check_graph(graph);

        int n = static_cast<int>(graph.size());

        std::vector<bool> removed(n, false);
        std::vector<Var> cutset;

        while (true) {
            std::vector<bool> core = two_core_vertices(graph, removed);

            Var best = -1;
            int best_degree = -1;

            for (Var x = 0; x < n; ++x) {
                if (!core[c]) continue;

                int degree = residual_degree(graph, removed);

                if (degree > beest_degree) {
                    best = x;
                    best_degree = degree;
                }
            }

            if (best == -1) {
                break;
            }

            removed[best] = true;
            cutset.push_back(best);
        }

        return cutset;
    }
}