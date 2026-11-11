/**
 * @file main.cpp
 * @brief Ponto de entrada do programa principal — Etapa 2: Leitura CSV, Grafo (DAG) e Kahn.
 *
 * Nota metodológica: A amostra é observacional e de tamanho reduzido (139 jogos).
 * Todos os resultados descrevem associações empíricas ("está associado a"), e não causalidade.
 */

#include "main.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

/**
 * @brief Imprime o resumo estrutural do grafo e sua ordenação topológica de Kahn.
 */
static bool inspecionar_grafo_e_toposort(const std::string& titulo,
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
        return false;
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
    return true;
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

        // 2. Construção dos Grafos por Contexto (COM_TORCIDA e SEM_TORCIDA)
        Grafo g_com = construir_grafo(transicoes, "COM_TORCIDA");
        Grafo g_sem = construir_grafo(transicoes, "SEM_TORCIDA");

        // E também recortes por perna dentro de cada contexto (conforme item 5.2.2)
        Grafo g_com_ida   = construir_grafo(transicoes, "COM_TORCIDA", 1);
        Grafo g_com_volta = construir_grafo(transicoes, "COM_TORCIDA", 2);
        Grafo g_sem_ida   = construir_grafo(transicoes, "SEM_TORCIDA", 1);
        Grafo g_sem_volta = construir_grafo(transicoes, "SEM_TORCIDA", 2);

        // 3. Execução da Ordenação Topológica (Kahn) e verificação de DAG
        if (!inspecionar_grafo_e_toposort("COM_TORCIDA (Ida + Volta)", g_com, true)) return 1;
        if (!inspecionar_grafo_e_toposort("SEM_TORCIDA (Ida + Volta)", g_sem, true)) return 1;

        // Resumo dos subgrafos por perna
        inspecionar_grafo_e_toposort("COM_TORCIDA - Perna 1 (Ida)", g_com_ida, false);
        inspecionar_grafo_e_toposort("COM_TORCIDA - Perna 2 (Volta)", g_com_volta, false);
        inspecionar_grafo_e_toposort("SEM_TORCIDA - Perna 1 (Ida)", g_sem_ida, false);
        inspecionar_grafo_e_toposort("SEM_TORCIDA - Perna 2 (Volta)", g_sem_volta, false);

    } catch (const std::exception& e) {
        std::cerr << "[EXCECAO] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
