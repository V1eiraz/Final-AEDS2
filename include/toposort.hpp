/**
 * @file toposort.hpp
 * @brief Algoritmo de Kahn para Ordenação Topológica e detecção de ciclos em DAGs.
 *
 * Complexidade de tempo: O(V + E), onde V é o número de vértices e E é o
 * número de arestas direcionadas distintas.
 */

#ifndef TOPOSORT_HPP
#define TOPOSORT_HPP

#include "grafo.hpp"

#include <vector>

/**
 * @brief Armazena o resultado da execução da ordenação topológica de Kahn.
 */
struct ResultadoToposort {
    std::vector<int> ordem; // Sequência linear de IDs dos vértices em ordem topológica
    bool tem_ciclo;         // true se o grafo possuir algum ciclo (erro nos dados)
    double tempo_ms;        // Tempo de execução do algoritmo em milissegundos
};

/**
 * @brief Executa o algoritmo de Kahn sobre um Grafo direcionado.
 * @param g Grafo de entrada.
 * @return ResultadoToposort contendo a ordem topológica, flag de ciclo e tempo gasto.
 */
ResultadoToposort ordenacao_topologica_kahn(const Grafo& g);

#endif // TOPOSORT_HPP
