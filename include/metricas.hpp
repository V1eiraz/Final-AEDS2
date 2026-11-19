/**
 * @file metricas.hpp
 * @brief Métricas de colapso psicológico calculadas a partir de data/eventos_ko.csv.
 *
 * Avalia P(sofrer o 2º gol | acabou de sofrer o 1º), comparando Mandante (H)
 * vs. Visitante (A) em cada contexto (COM_TORCIDA, SEM_TORCIDA, NEUTRO e TODOS).
 *
 * São calculadas duas variantes complementares:
 *   1. Após sofrer o 1º gol da partida (placar 0x1 contra a equipe):
 *      - Probabilidade de sofrer também o gol seguinte da partida (0x2 antes de empatar 1x1);
 *      - Probabilidade de sofrer esse 2º gol consecutivo em até 15 minutos ("choque rápido").
 *   2. Após qualquer gol sofrido na partida:
 *      - Probabilidade de o próximo gol da partida ser novamente contra a mesma equipe;
 *      - Probabilidade de sofrer outro gol consecutivo em até 15 minutos.
 */

#ifndef METRICAS_HPP
#define METRICAS_HPP

#include "csv.hpp"

#include <string>
#include <vector>

/**
 * @brief Estatísticas de colapso para um par (Contexto, Lado: Mandante ou Visitante).
 */
struct MetricaColapsoGrupo {
    std::string contexto;               // COM_TORCIDA, SEM_TORCIDA, NEUTRO ou TODOS
    std::string equipe_avaliada;        // "MANDANTE (H)" ou "VISITANTE (A)"
    int total_jogos_contexto;           // Total de jogos naquele contexto

    // --- Métrica A: Condicionada a ter sofrido o 1º gol da partida ---
    int n_sofreu_1o_gol;                // Quantos jogos a equipe saiu perdendo por 0x1
    int n_sofreu_2o_seguido;            // Quantos desses viraram 0x2 antes de 1x1
    double prob_2o_dado_1o;             // n_sofreu_2o_seguido / n_sofreu_1o_gol
    int n_sofreu_2o_ate_15min;          // Quantos viraram 0x2 em <= 15 minutos após o 1º gol
    double prob_2o_ate_15min_dado_1o;   // n_sofreu_2o_ate_15min / n_sofreu_1o_gol

    // --- Métrica B: Condicionada a qualquer gol sofrido na partida ---
    int n_gols_sofridos_total;          // Total de vezes que a equipe sofreu um gol
    int n_proximo_gol_tambem_sofrido;   // Vezes em que o gol seguinte do jogo foi contra a mesma equipe
    double prob_colapso_geral;          // n_proximo_gol_tambem_sofrido / n_gols_sofridos_total
    int n_proximo_sofrido_ate_15min;    // Vezes em que sofreu outro gol seguido em <= 15 min
    double prob_colapso_rapido_15min;   // n_proximo_sofrido_ate_15min / n_gols_sofridos_total
};

/**
 * @brief Calcula as métricas de colapso para Mandante e Visitante por contexto.
 */
std::vector<MetricaColapsoGrupo> calcular_metricas_colapso(const std::vector<JogoRow>& jogos,
                                                           const std::vector<EventoRow>& eventos);

/**
 * @brief Salva a tabela de métricas de colapso em arquivo CSV.
 */
void salvar_csv_metricas_colapso(const std::string& caminho_csv,
                                 const std::vector<MetricaColapsoGrupo>& metricas);

#endif // METRICAS_HPP
