/**
 * @file caminho.hpp
 * @brief Caminho mais provável até a virada em DAG usando peso w = -ln(p).
 *
 * Maximizar o produto das probabilidades das arestas de um caminho equivale a
 * minimizar a soma de -ln(p_i). Como 0 < p_i <= 1, todos os pesos -ln(p_i) são
 * não-negativos (>= 0).
 *
 * Implementa e compara dois algoritmos clássicos:
 *   1. Algoritmo de Dijkstra com fila de prioridades (min-heap): O((V + E) log V)
 *   2. Caminho Mínimo em DAG por Ordem Topológica: O(V + E)
 */

#ifndef CAMINHO_HPP
#define CAMINHO_HPP

#include "grafo.hpp"
#include "toposort.hpp"

#include <string>
#include <vector>

/**
 * @brief Resultado da busca pelo caminho mais provável entre origem e destino.
 */
struct ResultadoCaminho {
    bool alcancavel;                        // true se existe caminho de origem até destino
    double custo_neg_log;                   // Soma de -ln(p) ao longo do caminho mínimo
    double probabilidade_caminho;           // Produto das probabilidades: exp(-custo_neg_log)
    std::vector<int> caminho_ids;           // Sequência de IDs dos vértices (origem -> ... -> destino)
    std::vector<std::string> caminho_nomes; // Sequência de nomes dos estados
    double tempo_ms;                        // Tempo de execução em milissegundos
};

/**
 * @brief Encontra o caminho mais provável de 'id_origem' até 'id_destino' usando Dijkstra.
 *
 * Usa std::priority_queue (min-heap) com peso w(u -> v) = -ln(P(u -> v)).
 * Complexidade: O((V + E) log V).
 */
ResultadoCaminho caminho_mais_provavel_dijkstra(const Grafo& g, int id_origem, int id_destino);

/**
 * @brief Encontra o caminho mais provável de 'id_origem' até 'id_destino' via Ordem Topológica.
 *
 * Relaxa as arestas em uma única passada seguindo a ordem topológica do DAG.
 * Complexidade: O(V + E).
 */
ResultadoCaminho caminho_mais_provavel_dag(const Grafo& g,
                                           const ResultadoToposort& topo,
                                           int id_origem,
                                           int id_destino);

/**
 * @brief Formata a sequência de estados de um caminho como string "A -> B -> C".
 */
std::string formatar_caminho(const ResultadoCaminho& res);

/**
 * @brief Salva em CSV a comparação dos caminhos mais prováveis de virada (S < 0 -> CLASSIFICADO).
 */
void salvar_csv_caminhos_virada(const std::string& caminho_csv,
                                const Grafo& g_com,
                                const ResultadoToposort& topo_com,
                                const Grafo& g_sem,
                                const ResultadoToposort& topo_sem);

#endif // CAMINHO_HPP
