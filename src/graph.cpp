#include "graph.hpp"

#include <queue>
#include <stdexcept>

namespace {

    // Controllo base sul grafo: ogni vicino deve essere un indice valido
    void check_graph(const Graph &graph) {
        int n = static_cast<int>(graph.size()); // normalmente sarebbe size_t, ma Var è int

        for (Var x = 0; x < n; ++x) { // Scorro tutte le variabili/nodi
            for (Var y: graph[x]) {   // Scorro tutti i vicini di x
                if (y < 0 || y >= n) {
                    throw std::out_of_range("Graph contains invalid variable index");
                }
            }
        }
    }

    // Il vettore removed deve avere un booleano per ogni variabile
    void check_removed_size(const Graph &graph, const std::vector<bool> &removed) {
        if (graph.size() != removed.size()) { // Convenzione: removed[x] dice se x è rimossa
            throw std::invalid_argument("removed vector has invalid size");
        }
    }

    /**
     * DFS usata per rilevare cicli in un grafo non orientato.
     *
     * parent serve perché in un grafo non orientato, quando da x vado a y,
     * poi da y rivedo x tra i vicini. Quello NON è un ciclo, è solo l'arco
     * con cui sono arrivato.
     *
     * Invece, se trovo un vicino già visitato diverso dal padre, allora ho
     * trovato un ciclo reale.
     */
    bool check_cycle(
            const Graph &graph,
            const std::vector<bool> &removed,
            std::vector<bool> &visited,
            Var x,
            Var parent
    ) {
        visited[x] = true; // Segno x come visitato

        for (Var y: graph[x]) {
            // Se y è stato rimosso dal grafo residuo, lo ignoro
            if (removed[y]) {
                continue;
            }

            // Se y non è stato visitato, continuo la DFS
            if (!visited[y]) {
                if (check_cycle(graph, removed, visited, y, x)) {
                    return true;
                }
            }
                // Se y era già visitato e non è il padre, allora ho chiuso un ciclo
            else if (y != parent) {
                return true;
            }
        }

        // Nessun ciclo trovato partendo da x
        return false;
    }

}

bool is_forest_after_removing(const Graph &graph, const std::vector<bool> &removed) {
    // Validazioni in entrata
    check_graph(graph);
    check_removed_size(graph, removed);

    int n = static_cast<int>(graph.size());
    std::vector<bool> visited(n, false);

    /*
     * Una foresta può avere più componenti connesse.
     * Quindi non basta fare una DFS da 0: devo partire da ogni nodo non visitato.
     */
    for (Var x = 0; x < n; ++x) {
        // I nodi rimossi non fanno parte del grafo residuo
        if (removed[x]) {
            continue;
        }

        // Se x non è stato visitato, è l'inizio di una nuova componente
        if (!visited[x]) {
            // Se in questa componente trovo un ciclo, il residuo non è una foresta
            if (check_cycle(graph, removed, visited, x, -1)) {
                return false;
            }
        }
    }

    // Se nessuna componente contiene cicli, il grafo residuo è una foresta
    return true;
}

// Helper interni al cutset: non fanno parte dell'API pubblica del grafo.
static int residual_degree(const Graph &graph, Var x, const std::vector<bool> &removed) {
    // Grado di x contando solo i vicini ancora nel grafo residuo.
    // Nel cutset mi serve questo, non il grado di partenza.
    if (removed[x]) {
        return 0;
    }

    int degree = 0;

    // Salto i vicini rimossi: stanno già nel cutset, non contano più per i cicli.
    for (Var y: graph[x]) {
        if (!removed[y]) {
            ++degree;
        }
    }

    return degree;
}

static std::vector<bool> two_core_vertices(const Graph &graph, const std::vector<bool> &removed) {
    int n = static_cast<int>(graph.size());

    std::vector<bool> in_core(n, false);
    std::vector<int> degree(n, 0);
    std::queue<Var> q;

    // 2-core = quel che resta se tolgo di continuo i nodi di grado 0 o 1.
    // Un nodo con meno di 2 archi non può stare su un ciclo, quindi come
    // candidato al cutset lo ignoro: guardo solo qui dentro.
    for (Var x = 0; x < n; ++x) {
        if (!removed[x]) {
            in_core[x] = true;
            degree[x] = residual_degree(graph, x, removed);
        }
    }

    // scarto chi ha grado 0 e 1 e inserisco nella queue
    for (Var x = 0; x < n; ++x) {
        if (in_core[x] && degree[x] <= 1) {
            q.push(x);
        }
    }

    // Peeling a catena: tolgo un nodo, i vicini perdono un arco, e se uno
    // scende a grado 1 diventa foglia e tocca togliere anche lui
    while (!q.empty()) {
        Var x = q.front();
        q.pop();

        // Può succedere che x sia già stato eliminato da un passo precedente
        if (!in_core[x]) {
            continue;
        }

        // Tolgo x dal core
        in_core[x] = false;

        // Aggiorno i gradi dei vicini ancora nel core
        for (Var y: graph[x]) {
            if (!in_core[y]) {
                continue;
            }

            --degree[y];

            // Se il vicino è diventato foglia, anche lui non può stare in un ciclo
            if (degree[y] <= 1) {
                q.push(y);
            }
        }
    }

    // Restano true solo i nodi del 2-core: se è vuoto il residuo è già una
    // foresta, altrimenti i cicli da rompere stanno lì dentro.
    return in_core;
}

std::vector<Var> greedy_cycle_cutset(const Graph &graph) {
    // Validazione del grafo in ingresso
    check_graph(graph);

    int n = static_cast<int>(graph.size());

    std::vector<bool> removed(n, false);
    std::vector<Var> cutset;

    /*
     * Cycle cutset (R&N 6.5.1, Detcher 2006): le variabili che, tolte dal grafo
     * primale, lasciano una foresta. Mi serve perché sull'albero il residuo si
     * risolve in fretta col tree solver; qui provo a trovarne uno piccolo a greedy.
     */
    while (true) {
        // Ricalcolo il 2-core del residuo: i cicli vivono solo lì dentro.
        std::vector<bool> core = two_core_vertices(graph, removed);

        Var best = -1;
        int best_degree = -1;

        // Tra i nodi del 2-core prendo il più connesso: sta su più cicli, quindi
        // togliendolo ne rompo di più in un colpo solo.
        for (Var x = 0; x < n; ++x) {
            if (!core[x]) {
                continue;
            }

            int degree = residual_degree(graph, x, removed);

            // Scelta greedy: rimuovo il nodo più connesso nel grafo residuo
            if (degree > best_degree) {
                best = x;
                best_degree = degree;
            }
        }

        /*
         * Se best resta -1, il 2-core è vuoto.
         * Quindi non ci sono più cicli: il cutset trovato basta.
         */
        if (best == -1) {
            break;
        }

        // Rimuovo il nodo scelto e lo salvo nel cutset
        removed[best] = true;
        cutset.push_back(best);
    }

    return cutset;
}
