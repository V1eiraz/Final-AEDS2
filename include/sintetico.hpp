/**
 * @file sintetico.hpp
 * @brief Geração de dados sintéticos e experimentos de escalabilidade assintótica.
 *
 * Avalia empiricamente o comportamento assintótico dos algoritmos implementados
 * quando o tamanho da entrada cresce em ordens de grandeza:
 *   - Grafos Acíclicos Direcionados (DAGs) sintéticos com V em {10^2, 10^3, 10^4, 10^5}
 *     e E proporcional (~3*V), medindo: Kahn (Toposort), DP de Resiliência,
 *     Caminho Mínimo em DAG, Dijkstra (Min-Heap), BFS e DFS.
 *   - Datasets sintéticos de partidas com n em {10^2, 10^3, 10^4} amostras,
 *     medindo o tempo de treinamento da Árvore de Decisão CART.
 */

#ifndef SINTETICO_HPP
#define SINTETICO_HPP

#include "arvore.hpp"
#include "grafo.hpp"

#include <string>
#include <vector>

/**
 * @brief Armazena a medição de tempo de um algoritmo para um tamanho específico de entrada.
 */
struct RegistroEscalabilidade {
    std::string algoritmo;              // Ex.: "Kahn_Toposort", "DP_Resiliencia", "Dijkstra_MinHeap", "CART_Treino"
    std::string categoria;              // "GRAFO_DAG" ou "ARVORE_CART"
    int tamanho_entrada;                // Número de vértices V (para grafo) ou amostras n (para árvore)
    int tamanho_secundario;             // Número de arestas distintas E (para grafo) ou atributos M (para árvore)
    int repeticoes;                     // Quantidade de execuções independentes para cálculo da média
    double tempo_medio_ms;              // Tempo médio de execução em milissegundos
    double tempo_desvio_ms;             // Desvio-padrão amostral do tempo em milissegundos
    std::string complexidade_teorica;   // Ex.: "O(V + E)", "O((V + E) log V)", "O(N * M * log N)"
};

/**
 * @brief Constrói um DAG sintético em camadas com exatamente V vértices e ~3*V arestas.
 *
 * O vértice 0 é a origem principal ("ORIGEM"), o vértice V-2 é "CLASSIFICADO"
 * e o vértice V-1 é "ELIMINADO". Todas as arestas apontam de um vértice u para
 * vértices v > u, garantindo que o grafo seja estritamente acíclico (DAG).
 *
 * @param num_vertices Número total de vértices desejado (V >= 10).
 * @param semente Semente fixa para o gerador pseudoaleatório Xorshift32.
 * @return Objeto Grafo populado e com probabilidades normalizadas.
 */
Grafo gerar_dag_sintetico(int num_vertices, unsigned int semente = 12345);

/**
 * @brief Gera um conjunto de 'num_amostras' jogos sintéticos para teste de escala da Árvore CART.
 *
 * @param num_amostras Quantidade de linhas (n).
 * @param semente Semente fixa para reprodutibilidade.
 * @return Vetor de AmostraArvore com os 9 atributos preditores e rótulo binário.
 */
std::vector<AmostraArvore> gerar_amostras_sinteticas_arvore(int num_amostras,
                                                            unsigned int semente = 54321);

/**
 * @brief Executa toda a bateria de testes de escalabilidade, imprime a análise no terminal
 *        e grava o arquivo results/escalabilidade.csv.
 */
std::vector<RegistroEscalabilidade> executar_experimentos_escalabilidade(
    const std::string& caminho_csv = "results/escalabilidade.csv");

#endif // SINTETICO_HPP
