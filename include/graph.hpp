// Questa classe descrive le utilities per il grafo

#pragma once

#include "csp.hpp"

#include <vector>

/**
 * @param graph Grafo
 * @param removed  Nodi rimossi
 * @return Se il grafo rimanente è una foresta, ritorna true
 */
bool is_forest_after_removing(const Graph &graph, const std::vector<bool> &removed);

/**
 * @param graph Grafo
 * @return Returna true se il grafo è una foresta
 */
bool is_forest(const Graph &graph);


/**
 * Conta il grado di una variabile, ossia quanti vicini non rimossi ha ancora
 */
int residual_degree(const Graph &graph, Var x, const std::vector<bool> &removed);

/**
 * Calcola il 2-core del grafo residuo
 * @param graph Grafo
 * @param removed Nodi rimossi
 * @return
 */
std::vector<bool> two_core_vertices(const Graph &graph, const std::vector<bool> &removed);

/**
 * Euristica "ingorda" per trovare un cycle cutset
 *
 * Logica:
 * - Calcola 2-core del residuo
 * - se 2-core è vuoto => è una foresta
 * - altrimenti rimuove il nodo di grado residuo massimo (il più collegato)
 * - ripete
 *
 * Non garantisce minimo, ma va bene per ridurre la computazione.
 * @param graph
 * @return
 */
std::vector <Var> greedy_cycle_cutset(const Graph &graph);