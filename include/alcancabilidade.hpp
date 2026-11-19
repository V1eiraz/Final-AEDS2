/**
 * @file alcancabilidade.hpp
 * @brief Busca em Largura (BFS) e Busca em Profundidade (DFS) para análise de alcançabilidade.
 *
 * Identifica quais estados de pressão (saldo agregado negativo S < 0 na volta)
 * conseguem alcançar o estado terminal CLASSIFICADO em cada contexto
 * (COM_TORCIDA vs. SEM_TORCIDA), destacando estados de virada exclusivos de um contexto.
 */

#ifndef ALCANCABILIDADE_HPP
#define ALCANCABILIDADE_HPP

#include "grafo.hpp"

#include <string>
#include <vector>

/**
 * @brief Resultado de uma busca de alcançabilidade (BFS ou DFS) a partir de um vértice origem.
 */
struct ResultadoBusca {
    std::vector<bool> visitado;     // visitado[v] == true se v é alcançável a partir da origem
    std::vector<int> ordem_visita;  // Sequência em que os vértices foram descobertos
    double tempo_ms;                // Tempo de execução em milissegundos
};

/**
 * @brief Linha comparativa de alcançabilidade de virada para um estado P2 com saldo negativo.
 */
struct ComparacaoViradaEstado {
    std::string estado;             // Ex.: "P2|S-1|T5"
    int janela;                     // 0 a 7
    int saldo;                      // -3, -2 ou -1
    bool existe_com_torcida;        // O estado foi observado em COM_TORCIDA?
    int n_com_torcida;              // Número de observações em COM_TORCIDA
    bool vira_com_torcida;          // Alcança CLASSIFICADO em COM_TORCIDA (via BFS/DFS)?
    bool existe_sem_torcida;        // O estado foi observado em SEM_TORCIDA?
    int n_sem_torcida;              // Número de observações em SEM_TORCIDA
    bool vira_sem_torcida;          // Alcança CLASSIFICADO em SEM_TORCIDA (via BFS/DFS)?
    std::string categoria;          // "AMBOS_VIRAM", "SO_COM_TORCIDA_VIRA", "SO_SEM_TORCIDA_VIRA", "NENHUM_VIRA"
};

/**
 * @brief Executa Busca em Largura (BFS) a partir de 'id_origem'. Complexidade: O(V + E).
 */
ResultadoBusca bfs_alcancabilidade(const Grafo& g, int id_origem);

/**
 * @brief Executa Busca em Profundidade (DFS) a partir de 'id_origem'. Complexidade: O(V + E).
 */
ResultadoBusca dfs_alcancabilidade(const Grafo& g, int id_origem);

/**
 * @brief Compara todos os estados de saldo negativo (P2, S < 0) entre COM_TORCIDA e SEM_TORCIDA.
 *
 * Verifica por BFS e DFS (confirmando que ambos dão o mesmo resultado) quais
 * estados alcançam CLASSIFICADO em um contexto e nunca no outro.
 */
std::vector<ComparacaoViradaEstado> comparar_alcancabilidade_virada(const Grafo& g_com,
                                                                    const Grafo& g_sem);

/**
 * @brief Salva a tabela comparativa de alcançabilidade de virada em arquivo CSV.
 */
void salvar_csv_alcancabilidade(const std::string& caminho_csv,
                                const std::vector<ComparacaoViradaEstado>& tabela);

#endif // ALCANCABILIDADE_HPP
