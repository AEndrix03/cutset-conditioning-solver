# cutset-conditioning-solver

Solver per CSP binari basato su cutset conditioning (R&N 6.5.1, Dechter 2006), con un backtracking di confronto.

Tutto in C++17, solo std library, niente dipendenze esterne.

## Come si compila

Serve CMake (>= 3.16) e un compilatore C++17.

```
cmake -S . -B build
cmake --build build
./build/Debug/cutset_csp
```

## Come si riproducono i risultati

Il comando che genera tutta la tabella usata nella relazione (3 problemi x 2 istanze, backtracking e cutset a confronto):

```
./build/Debug/cutset_csp --all --repeat 5
```

`--repeat 5` ripete ogni run 5 volte e tiene la mediana. La tabella viene stampata a schermo: da lì ho copiato i numeri nella relazione.

Si può anche lanciare una singola istanza:

```
./build/Debug/cutset_csp --problem nqueens     --n 8                          --solver cutset
./build/Debug/cutset_csp --problem quasigroup  --order 5                      --solver bt
./build/Debug/cutset_csp --problem meeting     --instance tree         --meetings 40 --solver cutset
./build/Debug/cutset_csp --problem meeting     --instance single_cycle --meetings 40 --solver bt
```

Opzioni: `--all`, `--problem nqueens|quasigroup|meeting`, `--instance tree|single_cycle` (solo meeting), `--solver bt|cutset`, `--repeat N`, `--help`.

## Cosa c'è in ogni file

Sorgenti in `src/`, header corrispondenti in `include/`.

- `csp.*` - tipi base del CSP (variabili, domini, assegnamento), i vincoli binari e la classe `CSP` con i controlli di consistenza e il grafo primale.
- `graph.*` - utility sul grafo: algoritmi test foresta, greedy cycle cutset.
- `backtracking.*` - solver di backtracking .
- `tree_solver.*` - solver quando il grafo è una foresta: arc consistency direzionata dalle foglie alla radice, poi assegnamento top-down senza backtracking.
- `cutset.*` - cutset conditioning solver: trova il cutset, enumera i suoi assegnamenti e per ognuno delega il residuo (ormai un albero) al tree solver.
- `problems.*` - generatori e validatori dei problemi CSPLib.
- `experiment.*` - fa girare i solver, raccoglie le statistiche e stampa la tabella.
- `main.cpp` - la CLI.

Ogni algoritmo implementato riporta:

- pseudocodice se proveniente dal libro R&N
- citazione, con eccezione su algoritmi basilari sui grafi

## Problemi

I tre problemi vengono da CSPLib (https://www.csplib.org):

- **N-Queens** - prob054
- **Quasigroup Completion** - prob067
- **Meeting Scheduling** - prob046

In N-Queens si mettono n regine su una scacchiera n x n senza che due si attacchino: mai due sulla stessa riga, colonna o diagonale. Qui ogni variabile è una colonna e il valore è la riga della regina, così le colonne sono già distinte per costruzione e restano da vietare stessa riga e stessa diagonale per ogni coppia. Il grafo primale è completo (ogni coppia di colonne è in vincolo), quindi il cutset vale n-2. Le due istanze sono n = 8 e n = 10.

In Quasigroup Completion si completa un quadrato latino di ordine n parzialmente riempito: ogni riga e ogni colonna deve contenere i valori 1..n esattamente una volta. Ogni variabile è una cella con dominio 1..n, i vincoli sono all-different su righe e colonne (spezzati in disuguaglianze a coppie, così il CSP resta binario) e alcune celle partono già fissate come indizi. Il grafo primale è denso (rook graph). Le due istanze sono di ordine 4 e ordine 5.

In Meeting Scheduling ogni variabile è una riunione e il suo valore è lo slot iniziale; ogni arco del grafo primale è un partecipante in comune fra due riunioni, che quindi non possono sovrapporsi e devono lasciare il tempo di viaggio. Le due istanze hanno grafo dei conflitti ad albero e con un solo ciclo, così il cutset è rispettivamente vuoto e di dimensione uno.

I generatori delle istanze sono in `problems.*`.
