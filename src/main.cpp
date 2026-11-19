/**
 * @file main.cpp
 * @brief Ponto de entrada do programa principal — Etapas 2, 3, 4 (Módulo A) e Etapa 5 (Módulo B: Árvore CART).
 *
 * Nota metodológica: A amostra é observacional e de tamanho reduzido (139 jogos, sendo 123 no DAG).
 * Todos os resultados descrevem associações empíricas ("está associado a"), e não causalidade.
 */

#include "main.hpp"

#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Imprime o resumo estrutural do grafo e retorna sua ordenação topológica de Kahn.
 */
static ResultadoToposort inspecionar_grafo_e_toposort(const std::string& titulo,
                                                      const Grafo& g,
                                                      bool imprimir_todos_vertices) {
    ResultadoToposort topo = ordenacao_topologica_kahn(g);

    std::cout << "====================================================================\n";
    std::cout << "Contexto / Recorte: " << titulo << "\n";
    std::cout << "--------------------------------------------------------------------\n";
    std::cout << "  Vertices (V)                 : " << g.num_vertices() << "\n";
    std::cout << "  Arestas distintas (E)        : " << g.num_arestas_distintas() << "\n";
    std::cout << "  Transicoes observadas (soma) : " << g.total_transicoes_observadas() << "\n";
    std::cout << "  Ciclo detectado (Kahn)?      : " << (topo.tem_ciclo ? "SIM (ERRO!)" : "NAO (DAG valido)") << "\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Tempo Kahn                   : " << topo.tempo_ms << " ms\n";

    if (topo.tem_ciclo) {
        std::cerr << "[ERRO CRITICO] Ciclo detectado no grafo '" << titulo
                  << "'! Visitados apenas " << topo.ordem.size() << " de "
                  << g.num_vertices() << " vertices.\n";
        return topo;
    }

    if (imprimir_todos_vertices) {
        std::cout << "\n  Ordem Topologica completa (" << topo.ordem.size() << " estados):\n";
        for (std::size_t i = 0; i < topo.ordem.size(); ++i) {
            int u = topo.ordem[i];
            std::cout << "    " << std::setw(3) << (i + 1) << ". "
                      << std::left << std::setw(14) << g.obter_nome(u)
                      << std::right
                      << " [n=" << std::setw(3) << g.observacoes_estado(u)
                      << ", grau_in=" << std::setw(2) << g.grau_entrada(u)
                      << ", grau_out=" << std::setw(2) << g.vizinhos(u).size() << "]\n";
        }
    }
    std::cout << "\n";
    return topo;
}

/**
 * @brief Imprime a tabela comparativa do Índice de Resiliência (Perna 2 - Volta) com n por estado.
 */
static void imprimir_tabela_resiliencia_volta(const Grafo& g_com,
                                              const ResultadoDP& dp_com,
                                              const Grafo& g_sem,
                                              const ResultadoDP& dp_sem) {
    std::cout << "================================================================================\n";
    std::cout << "ETAPA 3: INDICE DE RESILIENCIA P(CLASSIFICADO | estado) - PERNA 2 (VOLTA)\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "  Tempo DP COM_TORCIDA : " << std::fixed << std::setprecision(4) << dp_com.tempo_ms << " ms\n";
    std::cout << "  Tempo DP SEM_TORCIDA : " << std::fixed << std::setprecision(4) << dp_sem.tempo_ms << " ms\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(12) << "Estado"
              << std::setw(8)  << "Janela"
              << std::setw(8)  << "Saldo"
              << std::right
              << std::setw(14) << "P(Com Torc.)"
              << std::setw(8)  << "n(Com)"
              << std::setw(14) << "P(Sem Torc.)"
              << std::setw(8)  << "n(Sem)"
              << std::setw(12) << "Dif(C - S)"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    for (int t = 0; t <= 7; ++t) {
        for (int s = -3; s <= 3; ++s) {
            std::string nome = "P2|S" + std::to_string(s) + "|T" + std::to_string(t);
            int id_com = g_com.obter_id(nome);
            int id_sem = g_sem.obter_id(nome);

            if (id_com == -1 && id_sem == -1) {
                continue;
            }

            std::cout << std::left
                      << std::setw(12) << nome
                      << std::setw(8)  << ("T" + std::to_string(t))
                      << std::setw(8)  << (s > 0 ? "+" + std::to_string(s) : std::to_string(s))
                      << std::right;

            if (id_com != -1) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(1) << (dp_com.prob_por_id[id_com] * 100.0) << "%";
                std::cout << std::setw(14) << oss.str()
                          << std::setw(8)  << g_com.observacoes_estado(id_com);
            } else {
                std::cout << std::setw(14) << "---"
                          << std::setw(8)  << 0;
            }

            if (id_sem != -1) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(1) << (dp_sem.prob_por_id[id_sem] * 100.0) << "%";
                std::cout << std::setw(14) << oss.str()
                          << std::setw(8)  << g_sem.observacoes_estado(id_sem);
            } else {
                std::cout << std::setw(14) << "---"
                          << std::setw(8)  << 0;
            }

            if (id_com != -1 && id_sem != -1) {
                double diff_pct = (dp_com.prob_por_id[id_com] - dp_sem.prob_por_id[id_sem]) * 100.0;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(1) << (diff_pct >= 0.0 ? "+" : "") << diff_pct << " pp";
                std::cout << std::setw(12) << oss.str();
            } else {
                std::cout << std::setw(12) << "N/A";
            }
            std::cout << "\n";
        }
        std::cout << "  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .\n";
    }

    int id_ex_com = g_com.obter_id("P2|S-1|T4");
    int id_ex_sem = g_sem.obter_id("P2|S-1|T4");
    std::cout << "\n>>> Destaque (Pergunta-Exemplo Secao 5.2.4 - Estado P2|S-1|T4):\n";
    if (id_ex_com != -1) {
        std::cout << "    - COM_TORCIDA: P(Classificar | P2|S-1|T4) = "
                  << std::fixed << std::setprecision(2) << (dp_com.prob_por_id[id_ex_com] * 100.0)
                  << "% (n = " << g_com.observacoes_estado(id_ex_com) << " jogos)\n";
    }
    if (id_ex_sem != -1) {
        std::cout << "    - SEM_TORCIDA: P(Classificar | P2|S-1|T4) = "
                  << std::fixed << std::setprecision(2) << (dp_sem.prob_por_id[id_ex_sem] * 100.0)
                  << "% (n = " << g_sem.observacoes_estado(id_ex_sem) << " jogos)\n";
    }
    std::cout << "================================================================================\n\n";
}

/**
 * @brief Imprime os resultados da Etapa 4.1: Caminho Mais Provável até a Virada (Dijkstra vs. DAG).
 */
static void imprimir_caminhos_virada(const Grafo& g_com,
                                     const ResultadoToposort& topo_com,
                                     const Grafo& g_sem,
                                     const ResultadoToposort& topo_sem) {
    std::cout << "================================================================================\n";
    std::cout << "ETAPA 4.1: CAMINHO MAIS PROVAVEL ATE A VIRADA (Dijkstra vs. DAG com w = -ln p)\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    const std::vector<std::string> origens_destaque = {
        "P2|S-1|T0", "P2|S-2|T0", "P2|S-3|T0",
        "P2|S-1|T3", "P2|S-1|T4", "P2|S-1|T5", "P2|S-2|T5", "P2|S-2|T6"
    };

    auto exibir_contexto = [](const std::string& nome_ctx,
                              const Grafo& g,
                              const ResultadoToposort& topo,
                              const std::vector<std::string>& origens) {
        std::cout << ">>> Contexto: " << nome_ctx << "\n";
        int id_class = g.obter_id("CLASSIFICADO");
        double soma_dijk_ms = 0.0;
        double soma_dag_ms = 0.0;
        int cont = 0;

        for (const std::string& est : origens) {
            int id_orig = g.obter_id(est);
            if (id_orig == -1) {
                std::cout << "  - Origem " << std::setw(10) << est << " [n= 0]: Estado nao observado neste contexto.\n";
                continue;
            }
            int n_obs = g.observacoes_estado(id_orig);
            ResultadoCaminho rc_dijk = caminho_mais_provavel_dijkstra(g, id_orig, id_class);
            ResultadoCaminho rc_dag  = caminho_mais_provavel_dag(g, topo, id_orig, id_class);

            soma_dijk_ms += rc_dijk.tempo_ms;
            soma_dag_ms  += rc_dag.tempo_ms;
            cont++;

            std::cout << "  - Origem " << std::setw(10) << est
                      << " [n=" << std::setw(2) << n_obs << "]: ";
            if (!rc_dijk.alcancavel) {
                std::cout << "INALCANCAVEL ate CLASSIFICADO\n";
            } else {
                std::cout << "P(caminho) = " << std::fixed << std::setprecision(2)
                          << (rc_dijk.probabilidade_caminho * 100.0) << "% "
                          << "(-ln p = " << std::setprecision(4) << rc_dijk.custo_neg_log << ")\n"
                          << "      Trajetoria: " << formatar_caminho(rc_dijk) << "\n";
            }
        }
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "  [Comparacao de tempo (" << cont << " consultas)] "
                  << "Dijkstra: " << soma_dijk_ms << " ms | "
                  << "DAG Toposort: " << soma_dag_ms << " ms\n\n";
    };

    exibir_contexto("COM_TORCIDA", g_com, topo_com, origens_destaque);
    exibir_contexto("SEM_TORCIDA", g_sem, topo_sem, origens_destaque);
    std::cout << "================================================================================\n\n";
}

/**
 * @brief Imprime os resultados da Etapa 4.2: Alcançabilidade de Virada (BFS e DFS).
 */
static void imprimir_alcancabilidade_virada(const std::vector<ComparacaoViradaEstado>& tabela) {
    std::cout << "================================================================================\n";
    std::cout << "ETAPA 4.2: ALCANCABILIDADE (BFS/DFS) - ESTADOS DE VIRADA (P2 com Saldo < 0)\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(12) << "Estado"
              << std::setw(16) << "COM_TORCIDA"
              << std::setw(16) << "SEM_TORCIDA"
              << std::setw(24) << "Categoria Comparativa"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    int cont_ambos = 0;
    int cont_so_com = 0;
    int cont_so_sem = 0;
    int cont_nenhum = 0;

    for (const ComparacaoViradaEstado& r : tabela) {
        if (r.categoria == "AMBOS_VIRAM") cont_ambos++;
        else if (r.categoria == "SO_COM_TORCIDA_VIRA") cont_so_com++;
        else if (r.categoria == "SO_SEM_TORCIDA_VIRA") cont_so_sem++;
        else cont_nenhum++;

        std::string str_com = !r.existe_com_torcida ? "Ausente (n=0)"
                              : (std::string(r.vira_com_torcida ? "VIRA" : "Nao vira") +
                                 " (n=" + std::to_string(r.n_com_torcida) + ")");
        std::string str_sem = !r.existe_sem_torcida ? "Ausente (n=0)"
                              : (std::string(r.vira_sem_torcida ? "VIRA" : "Nao vira") +
                                 " (n=" + std::to_string(r.n_sem_torcida) + ")");

        std::cout << std::left
                  << std::setw(12) << r.estado
                  << std::setw(16) << str_com
                  << std::setw(16) << str_sem
                  << std::setw(24) << r.categoria
                  << "\n";
    }

    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "Resumo dos " << tabela.size() << " estados de pressao (P2, S < 0) observados:\n";
    std::cout << "  - AMBOS_VIRAM         : " << cont_ambos << " estados\n";
    std::cout << "  - SO_COM_TORCIDA_VIRA : " << cont_so_com << " estados (virada alcancada so com torcida)\n";
    std::cout << "  - SO_SEM_TORCIDA_VIRA : " << cont_so_sem << " estados\n";
    std::cout << "  - NENHUM_VIRA         : " << cont_nenhum << " estados (sem virada em nenhum contexto)\n";
    std::cout << "================================================================================\n\n";
}

/**
 * @brief Imprime os resultados da Etapa 4.3: Métricas de Colapso Psicológico.
 */
static void imprimir_metricas_colapso(const std::vector<MetricaColapsoGrupo>& metricas) {
    std::cout << "================================================================================\n";
    std::cout << "ETAPA 4.3: METRICAS DE COLAPSO PSICOLOGICO (a partir de data/eventos_ko.csv)\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(14) << "Contexto"
              << std::setw(15) << "Equipe"
              << std::right
              << std::setw(16) << "P(2o | 1o gol)"
              << std::setw(15) << "P(2o <=15m|1o)"
              << std::setw(20) << "P(Seg. | Qualquer)"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    for (const MetricaColapsoGrupo& m : metricas) {
        std::ostringstream col1, col2, col3;
        col1 << std::fixed << std::setprecision(1) << (m.prob_2o_dado_1o * 100.0)
             << "% (" << m.n_sofreu_2o_seguido << "/" << m.n_sofreu_1o_gol << ")";
        col2 << std::fixed << std::setprecision(1) << (m.prob_2o_ate_15min_dado_1o * 100.0)
             << "% (" << m.n_sofreu_2o_ate_15min << "/" << m.n_sofreu_1o_gol << ")";
        col3 << std::fixed << std::setprecision(1) << (m.prob_colapso_geral * 100.0)
             << "% (" << m.n_proximo_gol_tambem_sofrido << "/" << m.n_gols_sofridos_total << ")";

        std::cout << std::left
                  << std::setw(14) << m.contexto
                  << std::setw(15) << m.equipe_avaliada
                  << std::right
                  << std::setw(16) << col1.str()
                  << std::setw(15) << col2.str()
                  << std::setw(20) << col3.str()
                  << "\n";
    }
    std::cout << "================================================================================\n\n";
}

int main() {
    try {
        // 1. Leitura dos arquivos CSV
        const std::vector<JogoRow> jogos = ler_jogos_csv("data/jogos_ko.csv");
        const std::vector<EventoRow> eventos = ler_eventos_csv("data/eventos_ko.csv");
        const std::vector<TransicaoRow> transicoes = ler_transicoes_csv("data/transicoes.csv");

        std::cout << ">>> Leitura de CSVs concluida com sucesso:\n";
        std::cout << "    - data/jogos_ko.csv     : " << jogos.size() << " jogos\n";
        std::cout << "    - data/eventos_ko.csv   : " << eventos.size() << " eventos\n";
        std::cout << "    - data/transicoes.csv   : " << transicoes.size() << " transicoes\n\n";

        std::filesystem::create_directories("results");

        // 2. Construção dos Grafos por Contexto (COM_TORCIDA e SEM_TORCIDA)
        Grafo g_com = construir_grafo(transicoes, "COM_TORCIDA");
        Grafo g_sem = construir_grafo(transicoes, "SEM_TORCIDA");

        Grafo g_com_ida   = construir_grafo(transicoes, "COM_TORCIDA", 1);
        Grafo g_com_volta = construir_grafo(transicoes, "COM_TORCIDA", 2);
        Grafo g_sem_ida   = construir_grafo(transicoes, "SEM_TORCIDA", 1);
        Grafo g_sem_volta = construir_grafo(transicoes, "SEM_TORCIDA", 2);

        // 3. Execução da Ordenação Topológica (Kahn) e verificação de DAG
        ResultadoToposort topo_com = inspecionar_grafo_e_toposort("COM_TORCIDA (Ida + Volta)", g_com, false);
        if (topo_com.tem_ciclo) return 1;

        ResultadoToposort topo_sem = inspecionar_grafo_e_toposort("SEM_TORCIDA (Ida + Volta)", g_sem, false);
        if (topo_sem.tem_ciclo) return 1;

        inspecionar_grafo_e_toposort("COM_TORCIDA - Perna 1 (Ida)", g_com_ida, false);
        inspecionar_grafo_e_toposort("COM_TORCIDA - Perna 2 (Volta)", g_com_volta, false);
        inspecionar_grafo_e_toposort("SEM_TORCIDA - Perna 1 (Ida)", g_sem_ida, false);
        inspecionar_grafo_e_toposort("SEM_TORCIDA - Perna 2 (Volta)", g_sem_volta, false);

        // 4. Etapa 3: Programação Dinâmica (DP) de Resiliência sobre o DAG
        ResultadoDP dp_com = calcular_dp_resiliencia(g_com, topo_com);
        ResultadoDP dp_sem = calcular_dp_resiliencia(g_sem, topo_sem);

        imprimir_tabela_resiliencia_volta(g_com, dp_com, g_sem, dp_sem);

        salvar_csv_resiliencia("results/dp_resiliencia.csv", dp_com, dp_sem);
        salvar_csv_comparacao_volta("results/dp_comparacao_volta.csv", g_com, dp_com, g_sem, dp_sem);

        // 5. Etapa 4: Caminho de Virada (-ln p), Alcançabilidade (BFS/DFS) e Métricas de Colapso
        imprimir_caminhos_virada(g_com, topo_com, g_sem, topo_sem);
        salvar_csv_caminhos_virada("results/caminhos_virada.csv", g_com, topo_com, g_sem, topo_sem);

        std::vector<ComparacaoViradaEstado> tab_alc = comparar_alcancabilidade_virada(g_com, g_sem);
        imprimir_alcancabilidade_virada(tab_alc);
        salvar_csv_alcancabilidade("results/alcancabilidade_virada.csv", tab_alc);

        std::vector<MetricaColapsoGrupo> tab_colapso = calcular_metricas_colapso(jogos, eventos);
        imprimir_metricas_colapso(tab_colapso);
        salvar_csv_metricas_colapso("results/metricas_colapso.csv", tab_colapso);

        // 6. Etapa 5: Módulo B — Árvore de Decisão CART (k-fold CV, Baselines, Importâncias e Exportação DOT)
        executar_modulo_arvore(jogos);

        std::cout << ">>> Arquivos gerados em results/:\n";
        std::cout << "    - results/dp_resiliencia.csv\n";
        std::cout << "    - results/dp_comparacao_volta.csv\n";
        std::cout << "    - results/caminhos_virada.csv\n";
        std::cout << "    - results/alcancabilidade_virada.csv\n";
        std::cout << "    - results/metricas_colapso.csv\n";
        std::cout << "    - results/arvore_cv_comparacao.csv\n";
        std::cout << "    - results/arvore_importancia_atributos.csv\n";
        std::cout << "    - results/arvore_resultado.dot (.txt)\n";
        std::cout << "    - results/arvore_classificou.dot (.txt)\n\n";

    } catch (const std::exception& e) {
        std::cerr << "[EXCECAO] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
