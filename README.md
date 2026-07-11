# cutset-conditioning-solver

Solver per CSP binari basato su cutset conditioning (R&N 6.5.1, Dechter 2006), con un backtracking di confronto.

Tutto in C++17, solo std library, niente dipendenze esterne.

## Come si compila

Serve CMake (>= 3.16) e un compilatore C++17.

```
cmake -S . -B build
cmake --build build --config Release
./build/Release/cutset_csp
```

## Come si riproducono i risultati

Il comando che genera tutta la tabella usata nella relazione (i tre problemi CSPLib con le loro istanze, backtracking e
cutset a confronto):

```
./build/Release/cutset_csp --all --repeat 5
```

`--repeat 5` ripete ogni run 5 volte e tiene la mediana. La tabella viene stampata a schermo: da lì ho copiato i numeri
nella relazione.

Si può anche lanciare una singola istanza:

```
./build/Release/cutset_csp --problem nqueens     --n 8                          --solver cutset
./build/Release/cutset_csp --problem quasigroup  --order 5                      --solver bt
./build/Release/cutset_csp --problem meeting     --instance tree         --meetings 40 --solver cutset
./build/Release/cutset_csp --problem meeting     --instance single_cycle --meetings 40 --solver bt
```

Opzioni: `--all`, `--problem nqueens|quasigroup|meeting`, `--instance tree|single_cycle|unsat|hard_sat` (solo meeting),
`--solver bt|cutset`, `--repeat N`, `--help`.

## Cosa c'è in ogni file

Sorgenti in `src/`, header corrispondenti in `include/`.

- `csp.*` - tipi base del CSP (variabili, domini, assegnamento), i vincoli binari e la classe `CSP` con i controlli di
  consistenza e il grafo primale.
- `graph.*` - utility sul grafo: algoritmi test foresta, greedy cycle cutset.
- `backtracking.*` - solver di backtracking .
- `tree_solver.*` - solver quando il grafo è una foresta: arc consistency direzionata dalle foglie alla radice, poi
  assegnamento top-down senza backtracking.
- `cutset.*` - cutset conditioning solver: trova il cutset, enumera i suoi assegnamenti e per ognuno delega il residuo (
  ormai un albero) al tree solver.
- `problems.*` - generatori e validatori dei problemi CSPLib.
- `experiment.*` - fa girare i solver, raccoglie le statistiche e stampa la tabella.
- `main.cpp` - la CLI.

Ogni algoritmo implementato riporta:

- pseudocodice se proveniente dal libro R&N
- citazione, con eccezione su algoritmi basilari sui grafi

## Problemi

Tre problemi da CSPLib (https://www.csplib.org), scelti per avere grafi primali
di densità diversa.

### N-Queens (prob054)

n regine su una scacchiera n x n senza che si attacchino (stessa riga, colonna o
diagonale). Variabile = colonna, valore = riga: le colonne sono distinte da sole,
resta da vietare stessa riga e stessa diagonale.

Grafo completo → cutset = n-2. Istanze: n = 8, n = 10.

### Quasigroup Completion (prob067)

Quadrato latino di ordine n riempito a metà: ogni riga e ogni colonna deve avere
1..n una volta sola. Variabile = cella, dominio 1..n; i vincoli all-different su
righe e colonne li spezzo in coppie (così restano binari); alcune celle partono
già fissate.

Grafo denso. Istanze: ordine 4, ordine 5.

### Meeting Scheduling (prob046)

Variabile = riunione, valore = slot di inizio. Un arco = due riunioni con un
partecipante in comune: non possono sovrapporsi e serve il tempo di spostamento.
Quattro istanze per far vedere il cutset in azione:

- `tree` - grafo ad albero, cutset vuoto.
- `cycle` - un solo ciclo, cutset = 1.
- `unsat` - non ha soluzione. Un ciclo di lunghezza dispari con vincolo "slot
  diverso" (solo due slot possibili) non si può colorare con due colori. Nessun
  vincolo preso da solo è sbagliato e l'arc consistency non se ne accorge: serve
  cercare. Il backtracking parte male e prova milioni di nodi; il cutset fissa una
  variabile del ciclo e sul resto la propagazione trova subito la contraddizione
  (due nodi).
- `hard_sat` - ha soluzione, ma è molto stretta. Una riunione "perno" ha un solo
  slot libero e va in conflitto con sette riunioni che vanno tutte di pomeriggio.
  La soluzione c'è ma il backtracking, con il suo ordine fisso, non la trova e
  prova milioni di nodi a vuoto; il cutset fissa il perno e chiude subito
  sull'albero che resta (cutset 1).

Insieme, `unsat` e `hard_sat` mostrano che il cutset conditioning vince sia
quando non c'è soluzione sia quando la soluzione è nascosta.

Generatori e validatori delle istanze sono in `problems.*`.