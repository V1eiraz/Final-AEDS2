/**
 * @file metricas.cpp
 * @brief Implementação do cálculo das métricas de colapso a partir de eventos_ko.csv.
 */

#include "metricas.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <unordered_map>

namespace {

struct GolSimples {
    int minuto;
    std::string lado_autor; // "H" (gol do mandante) ou "A" (gol do visitante)
};

} // namespace

std::vector<MetricaColapsoGrupo> calcular_metricas_colapso(const std::vector<JogoRow>& jogos,
                                                           const std::vector<EventoRow>& eventos) {
    // Agrupa todos os eventos do tipo GOAL por game_id
    std::unordered_map<int, std::vector<GolSimples>> gols_por_jogo;
    for (const EventoRow& e : eventos) {
        if (e.tipo == "GOAL") {
            gols_por_jogo[e.game_id].push_back(GolSimples{e.minuto, e.lado}.lado_autor == "H"
                                                   ? GolSimples{e.minuto, "H"}
                                                   : GolSimples{e.minuto, "A"});
        }
    }

    // Ordena os gols de cada jogo cronologicamente pelo minuto
    for (auto& par : gols_por_jogo) {
        std::stable_sort(par.second.begin(), par.second.end(),
                         [](const GolSimples& a, const GolSimples& b) {
                             return a.minuto < b.minuto;
                         });
    }

    const std::vector<std::string> contextos = {"COM_TORCIDA", "SEM_TORCIDA", "NEUTRO", "TODOS"};
    const std::vector<std::string> lados_avaliados = {"H", "A"}; // H = Mandante, A = Visitante

    std::vector<MetricaColapsoGrupo> resultados;

    for (const std::string& ctx : contextos) {
        for (const std::string& lado_vitima : lados_avaliados) {
            // Se avaliamos o colapso do Mandante ("H"), ele sofre gol quando o autor é o Visitante ("A")
            std::string lado_adversario = (lado_vitima == "H") ? "A" : "H";

            MetricaColapsoGrupo m;
            m.contexto = ctx;
            m.equipe_avaliada = (lado_vitima == "H") ? "MANDANTE (H)" : "VISITANTE (A)";
            m.total_jogos_contexto = 0;
            m.n_sofreu_1o_gol = 0;
            m.n_sofreu_2o_seguido = 0;
            m.prob_2o_dado_1o = 0.0;
            m.n_sofreu_2o_ate_15min = 0;
            m.prob_2o_ate_15min_dado_1o = 0.0;
            m.n_gols_sofridos_total = 0;
            m.n_proximo_gol_tambem_sofrido = 0;
            m.prob_colapso_geral = 0.0;
            m.n_proximo_sofrido_ate_15min = 0;
            m.prob_colapso_rapido_15min = 0.0;

            for (const JogoRow& j : jogos) {
                if (ctx != "TODOS" && j.contexto != ctx) {
                    continue;
                }
                m.total_jogos_contexto += 1;

                auto it = gols_por_jogo.find(j.game_id);
                if (it == gols_por_jogo.end() || it->second.empty()) {
                    continue; // Jogo 0x0
                }

                const std::vector<GolSimples>& gols = it->second;

                // 1. Métrica A: Condicionada a ter sofrido o 1º gol do jogo
                if (gols[0].lado_autor == lado_adversario) {
                    m.n_sofreu_1o_gol += 1;
                    // Verifica se houve um 2º gol na partida e se também foi do adversário (0x2)
                    if (gols.size() >= 2 && gols[1].lado_autor == lado_adversario) {
                        m.n_sofreu_2o_seguido += 1;
                        if (gols[1].minuto - gols[0].minuto <= 15) {
                            m.n_sofreu_2o_ate_15min += 1;
                        }
                    }
                }

                // 2. Métrica B: Condicionada a qualquer gol sofrido na partida
                for (std::size_t i = 0; i < gols.size(); ++i) {
                    if (gols[i].lado_autor == lado_adversario) {
                        m.n_gols_sofridos_total += 1;
                        if (i + 1 < gols.size() && gols[i + 1].lado_autor == lado_adversario) {
                            m.n_proximo_gol_tambem_sofrido += 1;
                            if (gols[i + 1].minuto - gols[i].minuto <= 15) {
                                m.n_proximo_sofrido_ate_15min += 1;
                            }
                        }
                    }
                }
            }

            if (m.n_sofreu_1o_gol > 0) {
                m.prob_2o_dado_1o = static_cast<double>(m.n_sofreu_2o_seguido) /
                                    static_cast<double>(m.n_sofreu_1o_gol);
                m.prob_2o_ate_15min_dado_1o = static_cast<double>(m.n_sofreu_2o_ate_15min) /
                                              static_cast<double>(m.n_sofreu_1o_gol);
            }

            if (m.n_gols_sofridos_total > 0) {
                m.prob_colapso_geral = static_cast<double>(m.n_proximo_gol_tambem_sofrido) /
                                       static_cast<double>(m.n_gols_sofridos_total);
                m.prob_colapso_rapido_15min = static_cast<double>(m.n_proximo_sofrido_ate_15min) /
                                              static_cast<double>(m.n_gols_sofridos_total);
            }

            resultados.push_back(m);
        }
    }

    return resultados;
}

void salvar_csv_metricas_colapso(const std::string& caminho_csv,
                                 const std::vector<MetricaColapsoGrupo>& metricas) {
    std::ofstream arq(caminho_csv);
    if (!arq.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo CSV: " + caminho_csv);
    }

    arq << "contexto,equipe_avaliada,total_jogos,n_sofreu_1o_gol,n_sofreu_2o_seguido,"
        << "prob_2o_dado_1o,n_sofreu_2o_ate_15min,prob_2o_ate_15min_dado_1o,"
        << "n_gols_sofridos_total,n_proximo_gol_tambem_sofrido,prob_colapso_geral,"
        << "n_proximo_sofrido_ate_15min,prob_colapso_rapido_15min\n";
    arq << std::fixed << std::setprecision(6);

    for (const MetricaColapsoGrupo& m : metricas) {
        arq << m.contexto << ","
            << m.equipe_avaliada << ","
            << m.total_jogos_contexto << ","
            << m.n_sofreu_1o_gol << ","
            << m.n_sofreu_2o_seguido << ","
            << m.prob_2o_dado_1o << ","
            << m.n_sofreu_2o_ate_15min << ","
            << m.prob_2o_ate_15min_dado_1o << ","
            << m.n_gols_sofridos_total << ","
            << m.n_proximo_gol_tambem_sofrido << ","
            << m.prob_colapso_geral << ","
            << m.n_proximo_sofrido_ate_15min << ","
            << m.prob_colapso_rapido_15min << "\n";
    }
}
