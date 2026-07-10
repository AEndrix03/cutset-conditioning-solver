// Questa classe descrive le utilities per il grafo

#pragma once

#include "csp.hpp"

#include <vector>

/**
 * Verifica la proprietà richiesta dal cutset conditioning (R&N 6.5.1): rimosso il
 * cutset, il grafo residuo deve essere una foresta.
 *
 * @param graph Grafo
 * @param removed  Nodi rimossi
 * @return Se il grafo rimanente è una foresta, ritorna true
 */
bool is_forest_after_removing(const Graph &graph, const std::vector<bool> &removed);

/**
 * Euristica greedy per trovare un cycle cutset
 *
 * Logica:
 * - Calcola 2-core del residuo
 * - se 2-core è vuoto => è una foresta
 * - altrimenti rimuove il nodo di grado residuo massimo (il più collegato)
 * - ripete
 *
 * Il cycle cutset è il concetto alla base del cutset conditioning (R&N 6.5.1, Dechter 2006).
 * Il criterio "grado massimo" richiama la degree heuristic (R&N 6.3.1),
 * qui riadattata alla scelta del cutset ("). 2-core e peeling sono teoria dei grafi standard.
 * @param graph
 * @return
 */
std::vector<Var> greedy_cycle_cutset(const Graph &graph);
