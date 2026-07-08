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

    // Controlla che una variabile esista nel grafo
    void check_var(Var x, int nvars) {
        if (x < 0 || x >= nvars) {
            throw std::out_of_range("Var index is out of range");
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

bool is_forest(const Graph &graph) {
    // Caso comodo: non rimuovo nessun nodo
    std::vector<bool> removed(graph.size(), false);
    return is_forest_after_removing(graph, removed);
}

int residual_degree(const Graph &graph, Var x, const std::vector<bool> &removed) {
    // Validazioni
    check_graph(graph);
    check_removed_size(graph, removed);
    check_var(x, static_cast<int>(graph.size()));

    /*
     * Grado residuo:
     * è il grado di x nel grafo che rimane dopo aver rimosso alcune variabili.
     *
     * Nel cutset conditioning non ci interessa il grado originale,
     * ma quanti archi ha ancora x nel problema residuo.
     */
    if (removed[x]) {
        return 0;
    }

    int degree = 0;

    /*
     * Conto solo i vicini ancora presenti.
     *
     * I vicini rimossi appartengono già al cutset, quindi non fanno più parte
     * del grafo su cui sto cercando cicli.
     */
    for (Var y: graph[x]) {
        if (!removed[y]) {
            ++degree;
        }
    }

    return degree;
}

std::vector<bool> two_core_vertices(const Graph &graph, const std::vector<bool> &removed) {
    // Validazioni
    check_graph(graph);
    check_removed_size(graph, removed);

    int n = static_cast<int>(graph.size());

    std::vector<bool> in_core(n, false);
    std::vector<int> degree(n, 0);
    std::queue <Var> q;

    /*
     * 2-core:
     * è il sottografo ottenuto eliminando ripetutamente tutti i nodi
     * con grado 0 o 1.
     *
     * Si usa perché un nodo con grado minore di 2 non può stare in un ciclo.
     * Quindi non ha senso sceglierlo come variabile del cutset.
     */
    for (Var x = 0; x < n; ++x) {
        if (!removed[x]) {
            in_core[x] = true;
            degree[x] = residual_degree(graph, x, removed);
        }
    }

    /*
      * I primi nodi da eliminare sono quelli che sicuramente non stanno
      * in nessun ciclo:
      *
      * - grado 0: nodo isolato
      * - grado 1: foglia
      *
      * In entrambi i casi non possono chiudere un ciclo.
      */
    for (Var x = 0; x < n; ++x) {
        if (in_core[x] && degree[x] <= 1) {
            q.push(x);
        }
    }

    /*
     * Peeling del grafo:
     * elimino foglie e nodi isolati.
     *
     * Quando rimuovo un nodo, il grado dei suoi vicini diminuisce.
     * Un vicino che prima aveva grado 2 può diventare foglia,
     * quindi va eliminato a sua volta.
     */
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

    /*
      * Alla fine restano true solo i nodi del 2-core.
      *
      * Se il 2-core è vuoto, il grafo residuo è una foresta.
      * Se non è vuoto, lì dentro ci sono ancora cicli da rompere.
      */
    return in_core;
}

std::vector <Var> greedy_cycle_cutset(const Graph &graph) {
    // Validazione del grafo in ingresso
    check_graph(graph);

    int n = static_cast<int>(graph.size());

    std::vector<bool> removed(n, false);
    std::vector <Var> cutset;

    /*
     * Cycle cutset:
     * è un insieme di variabili che, se rimosse dal grafo primale,
     * rende il grafo residuo aciclico, cioè una foresta.
     *
     * Serve perché i CSP con struttura ad albero si risolvono in modo più efficiente.
     * Il cutset conditioning enumera i valori delle variabili nel cutset
     * e poi risolve il problema residuo sfruttando la struttura ad albero.
     */
    while (true) {
        /*
         * Calcolo il 2-core del grafo residuo.
         *
         * I cicli possono esistere solo nel 2-core.
         * Se fuori dal 2-core ci sono foglie o nodi isolati,
         * rimuoverli sarebbe inutile per rompere cicli.
         */
        std::vector<bool> core = two_core_vertices(graph, removed);

        Var best = -1;
        int best_degree = -1;

        /*
         * Cerco il miglior nodo da rimuovere.
         *
         * Considero solo i nodi nel 2-core, perché i nodi fuori dal 2-core
         * non partecipano a cicli e quindi rimuoverli sarebbe inutile. Un nodo più connesso tende a partecipare a più cicli,
         * quindi rimuoverlo può rompere più struttura ciclica insieme.
         */
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