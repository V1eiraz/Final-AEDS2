/**
 * @file dp.hpp
 * @brief Programação Dinâmica (DP) sobre o DAG para cálculo do Índice de Resiliência.
 *
 * Calcula, para cada estado u do DAG, a probabilidade de absorção no estado
 * terminal CLASSIFICADO, processando os vértices em ordem topológica reversa.
 * Complexidade de tempo: O(V + E).
 */

#ifndef DP_HPP
#define DP_HPP

#include "grafo.hpp"
#include "toposort.hpp"

#include <string>
#include <vector>

/**
 * @brief Registro detalhado do resultado da DP para um estado específico.
 */
struct RegistroEstadoDP {
    int id_vertice;             // ID interno do vértice no grafo
    std::string nome_estado;    // Ex.: "P2|S-1|T4" ou "CLASSIFICADO"
    int perna;                  // 1 (ida), 2 (volta) ou 0 (terminal)
    int saldo;                  // Saldo agregado S (-3 a +3), ou 0 para terminal
    int janela;                 // Janela de tempo T (0 a 7), ou 8 para terminal
    int n_observacoes;          // Número de observações (n) do estado nos dados
    double prob_classificar;    // P(chegar em CLASSIFICADO | estado atual)
};

/**
 * @brief Resultado completo da execução da Programação Dinâmica sobre um Grafo.
 */
struct ResultadoDP {
    std::vector<double> prob_por_id;            // Vetor indexado pelo ID do vértice (0 .. V-1)
    std::vector<RegistroEstadoDP> tabela;       // Lista de estados ordenada para exibição/CSV
    double tempo_ms;                            // Tempo de execução da DP em milissegundos
};

/**
 * @brief Decompõe o nome de um estado "P{p}|S{s}|T{t}" em seus campos numéricos.
 *
 * Para estados terminais ("CLASSIFICADO", "ELIMINADO", "FIM_IDA"), define
 * perna = 0, saldo = 0, janela = 8 e retorna false.
 *
 * @param nome String do estado.
 * @param perna Saída: número da perna (1 ou 2).
 * @param saldo Saída: saldo agregado (-3 a +3).
 * @param janela Saída: janela de tempo (0 a 7).
 * @return true se for um estado regular "P|S|T", false se for terminal.
 */
bool extrair_componentes_estado(const std::string& nome, int& perna, int& saldo, int& janela);

/**
 * @brief Calcula o Índice de Resiliência P(chegar em CLASSIFICADO) via DP em ordem topológica reversa.
 *
 * Condições de contorno (estados terminais):
 *   - dp[CLASSIFICADO] = 1.0
 *   - dp[ELIMINADO]    = 0.0
 *   - dp[FIM_IDA]      = 0.0
 *
 * Equação de recorrência (para u não-terminal, em ordem topológica reversa):
 *   dp[u] = soma_{(u -> v)} P(u -> v) * dp[v]
 *
 * @param g Grafo direcionado acíclico (DAG).
 * @param topo Resultado da ordenação topológica de Kahn sobre g.
 * @return ResultadoDP com o vetor de probabilidades, tabela ordenada e tempo gasto.
 */
ResultadoDP calcular_dp_resiliencia(const Grafo& g,
                                    const ResultadoToposort& topo,
                                    bool montar_tabela = true);

/**
 * @brief Salva a tabela completa de resiliência (COM_TORCIDA e SEM_TORCIDA) em arquivo CSV.
 *
 * @param caminho_csv Caminho do arquivo de saída (ex.: "results/dp_resiliencia.csv").
 * @param dp_com Resultado da DP para o grafo COM_TORCIDA.
 * @param dp_sem Resultado da DP para o grafo SEM_TORCIDA.
 */
void salvar_csv_resiliencia(const std::string& caminho_csv,
                            const ResultadoDP& dp_com,
                            const ResultadoDP& dp_sem);

/**
 * @brief Salva uma tabela comparativa lado a lado (COM_TORCIDA vs. SEM_TORCIDA) para a Perna 2 (Volta).
 *
 * @param caminho_csv Caminho do arquivo de saída (ex.: "results/dp_comparacao_volta.csv").
 * @param g_com Grafo COM_TORCIDA.
 * @param dp_com Resultado da DP para COM_TORCIDA.
 * @param g_sem Grafo SEM_TORCIDA.
 * @param dp_sem Resultado da DP para SEM_TORCIDA.
 */
void salvar_csv_comparacao_volta(const std::string& caminho_csv,
                                 const Grafo& g_com,
                                 const ResultadoDP& dp_com,
                                 const Grafo& g_sem,
                                 const ResultadoDP& dp_sem);

#endif // DP_HPP
