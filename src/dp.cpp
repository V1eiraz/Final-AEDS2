/**
 * @file dp.cpp
 * @brief Implementação da Programação Dinâmica sobre o DAG para o Índice de Resiliência.
 */

#include "dp.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <set>
#include <stdexcept>

bool extrair_componentes_estado(const std::string& nome, int& perna, int& saldo, int& janela) {
    perna = 0;
    saldo = 0;
    janela = 8;

    // Formato esperado: "P{perna}|S{saldo}|T{janela}", ex.: "P2|S-1|T4"
    std::size_t pos1 = nome.find('|');
    if (pos1 == std::string::npos) {
        return false; // Estado terminal (CLASSIFICADO, ELIMINADO, FIM_IDA)
    }
    std::size_t pos2 = nome.find('|', pos1 + 1);
    if (pos2 == std::string::npos) {
        return false;
    }

    std::string str_p = nome.substr(0, pos1);
    std::string str_s = nome.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string str_t = nome.substr(pos2 + 1);

    if (str_p.size() < 2 || str_s.size() < 2 || str_t.size() < 2) {
        return false;
    }

    perna = std::stoi(str_p.substr(1));
    saldo = std::stoi(str_s.substr(1));
    janela = std::stoi(str_t.substr(1));
    return true;
}

ResultadoDP calcular_dp_resiliencia(const Grafo& g,
                                    const ResultadoToposort& topo,
                                    bool montar_tabela) {
    if (topo.tem_ciclo) {
        throw std::runtime_error("Nao e possivel executar DP em grafo com ciclo.");
    }

    auto inicio = std::chrono::high_resolution_clock::now();

    int V = g.num_vertices();
    std::vector<double> dp(V, 0.0);

    // 1. Condições de contorno nos estados terminais
    int id_classificado = g.obter_id("CLASSIFICADO");
    if (id_classificado != -1) {
        dp[id_classificado] = 1.0;
    }
    // ELIMINADO e FIM_IDA permanecem com 0.0

    // 2. Processamento em ordem topológica REVERSA (do fim para o início: T7 -> T6 -> ... -> T0)
    // Como toda aresta (u -> v) tem v aparecendo depois de u na ordem topológica,
    // ao percorrer de trás para frente garantimos que dp[v] já está 100% calculado quando visitamos u.
    for (int idx = static_cast<int>(topo.ordem.size()) - 1; idx >= 0; --idx) {
        int u = topo.ordem[idx];

        // Se for vértice terminal (sem arestas de saída), mantém seu valor base
        const std::vector<Aresta>& arestas_saida = g.vizinhos(u);
        if (arestas_saida.empty()) {
            continue;
        }

        double soma_prob = 0.0;
        for (const Aresta& a : arestas_saida) {
            soma_prob += a.probabilidade * dp[a.destino];
        }
        dp[u] = soma_prob;
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duracao = fim - inicio;

    // 3. Monta a tabela estruturada e ordena por (perna, janela, saldo) para leitura clara
    ResultadoDP res;
    res.prob_por_id = std::move(dp);
    res.tempo_ms = duracao.count();

    if (montar_tabela) {
        res.tabela.reserve(V);
        for (int u = 0; u < V; ++u) {
            RegistroEstadoDP reg;
            reg.id_vertice = u;
            reg.nome_estado = g.obter_nome(u);
            extrair_componentes_estado(reg.nome_estado, reg.perna, reg.saldo, reg.janela);
            reg.n_observacoes = g.observacoes_estado(u);
            reg.prob_classificar = res.prob_por_id[u];
            res.tabela.push_back(reg);
        }

        std::sort(res.tabela.begin(), res.tabela.end(),
                  [](const RegistroEstadoDP& a, const RegistroEstadoDP& b) {
                      int pa = (a.perna == 0) ? 99 : a.perna;
                      int pb = (b.perna == 0) ? 99 : b.perna;
                      if (pa != pb) return pa < pb;
                      if (a.janela != b.janela) return a.janela < b.janela;
                      if (a.saldo != b.saldo) return a.saldo < b.saldo;
                      return a.nome_estado < b.nome_estado;
                  });
    }

    return res;
}

void salvar_csv_resiliencia(const std::string& caminho_csv,
                            const ResultadoDP& dp_com,
                            const ResultadoDP& dp_sem) {
    std::ofstream arq(caminho_csv);
    if (!arq.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo CSV: " + caminho_csv);
    }

    arq << "contexto,estado,perna,saldo,janela,n_obs,prob_classificar\n";
    arq << std::fixed << std::setprecision(6);

    for (const RegistroEstadoDP& r : dp_com.tabela) {
        arq << "COM_TORCIDA," << r.nome_estado << "," << r.perna << ","
            << r.saldo << "," << r.janela << "," << r.n_observacoes << ","
            << r.prob_classificar << "\n";
    }

    for (const RegistroEstadoDP& r : dp_sem.tabela) {
        arq << "SEM_TORCIDA," << r.nome_estado << "," << r.perna << ","
            << r.saldo << "," << r.janela << "," << r.n_observacoes << ","
            << r.prob_classificar << "\n";
    }
}

void salvar_csv_comparacao_volta(const std::string& caminho_csv,
                                 const Grafo& g_com,
                                 const ResultadoDP& dp_com,
                                 const Grafo& g_sem,
                                 const ResultadoDP& dp_sem) {
    std::ofstream arq(caminho_csv);
    if (!arq.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo CSV: " + caminho_csv);
    }

    arq << "estado,janela,saldo,n_com_torcida,prob_com_torcida,n_sem_torcida,prob_sem_torcida,diferenca_com_menos_sem\n";
    arq << std::fixed << std::setprecision(6);

    // Percorre todas as janelas T0..T7 e saldos S-3..S+3 da Perna 2 (Volta)
    for (int t = 0; t <= 7; ++t) {
        for (int s = -3; s <= 3; ++s) {
            std::string nome = "P2|S" + std::to_string(s) + "|T" + std::to_string(t);
            int id_com = g_com.obter_id(nome);
            int id_sem = g_sem.obter_id(nome);

            if (id_com == -1 && id_sem == -1) {
                continue; // Estado nunca observado em nenhum dos dois contextos
            }

            int n_com = (id_com != -1) ? g_com.observacoes_estado(id_com) : 0;
            double p_com = (id_com != -1) ? dp_com.prob_por_id[id_com] : -1.0;

            int n_sem = (id_sem != -1) ? g_sem.observacoes_estado(id_sem) : 0;
            double p_sem = (id_sem != -1) ? dp_sem.prob_por_id[id_sem] : -1.0;

            double diff = (id_com != -1 && id_sem != -1) ? (p_com - p_sem) : 0.0;

            arq << nome << "," << t << "," << s << ","
                << n_com << "," << p_com << ","
                << n_sem << "," << p_sem << ","
                << diff << "\n";
        }
    }
}
