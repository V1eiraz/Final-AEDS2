/**
 * @file caminho.cpp
 * @brief Implementação do Caminho Mais Provável com peso -ln(p) via Dijkstra e via DAG Toposort.
 */

#include "caminho.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <utility>

/**
 * @brief Reconstrói o caminho da origem ao destino usando o vetor de predecessores 'pai'.
 */
static void reconstruir_caminho(const Grafo& g,
                                int id_origem,
                                int id_destino,
                                const std::vector<int>& pai,
                                ResultadoCaminho& res) {
    std::vector<int> ids;
    int atual = id_destino;
    while (atual != -1) {
        ids.push_back(atual);
        if (atual == id_origem) {
            break;
        }
        atual = pai[atual];
    }

    std::reverse(ids.begin(), ids.end());
    res.caminho_ids = ids;
    res.caminho_nomes.reserve(ids.size());
    for (int id : ids) {
        res.caminho_nomes.push_back(g.obter_nome(id));
    }
}

ResultadoCaminho caminho_mais_provavel_dijkstra(const Grafo& g, int id_origem, int id_destino) {
    auto inicio = std::chrono::high_resolution_clock::now();

    ResultadoCaminho res;
    res.alcancavel = false;
    res.custo_neg_log = std::numeric_limits<double>::infinity();
    res.probabilidade_caminho = 0.0;
    res.tempo_ms = 0.0;

    int V = g.num_vertices();
    if (id_origem < 0 || id_origem >= V || id_destino < 0 || id_destino >= V) {
        return res;
    }

    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dist(V, INF);
    std::vector<int> pai(V, -1);

    // Fila de prioridades mínima (min-heap) armazenando pares (distancia_acumulada, vertice)
    using Par = std::pair<double, int>;
    std::priority_queue<Par, std::vector<Par>, std::greater<Par>> pq;

    dist[id_origem] = 0.0;
    pq.push({0.0, id_origem});

    while (!pq.empty()) {
        Par topo = pq.top();
        pq.pop();

        double d_u = topo.first;
        int u = topo.second;

        // Ignora entradas obsoletas na fila de prioridades (lazy deletion)
        if (d_u > dist[u]) {
            continue;
        }

        // Parada antecipada opcional: quando o destino sai da min-heap, sua distância é ótima
        if (u == id_destino) {
            break;
        }

        for (const Aresta& a : g.vizinhos(u)) {
            if (a.probabilidade <= 0.0) {
                continue;
            }
            // Peso não-negativo: w = -ln(p) >= 0
            double peso = -std::log(a.probabilidade);
            if ( peso < 0.0 ) {
                peso = 0.0; // Evita -0.0 por arredondamento quando p == 1.0
            }

            int v = a.destino;
            if (dist[u] + peso < dist[v]) {
                dist[v] = dist[u] + peso;
                pai[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duracao = fim - inicio;
    res.tempo_ms = duracao.count();

    if (dist[id_destino] < INF) {
        res.alcancavel = true;
        res.custo_neg_log = dist[id_destino];
        res.probabilidade_caminho = std::exp(-dist[id_destino]);
        reconstruir_caminho(g, id_origem, id_destino, pai, res);
    }

    return res;
}

ResultadoCaminho caminho_mais_provavel_dag(const Grafo& g,
                                           const ResultadoToposort& topo,
                                           int id_origem,
                                           int id_destino) {
    auto inicio = std::chrono::high_resolution_clock::now();

    ResultadoCaminho res;
    res.alcancavel = false;
    res.custo_neg_log = std::numeric_limits<double>::infinity();
    res.probabilidade_caminho = 0.0;
    res.tempo_ms = 0.0;

    int V = g.num_vertices();
    if (id_origem < 0 || id_origem >= V || id_destino < 0 || id_destino >= V) {
        return res;
    }

    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dist(V, INF);
    std::vector<int> pai(V, -1);

    dist[id_origem] = 0.0;

    // Relaxa as arestas diretamente seguindo a ordem topológica: O(V + E)
    for (int u : topo.ordem) {
        if (dist[u] == INF) {
            continue;
        }
        for (const Aresta& a : g.vizinhos(u)) {
            if (a.probabilidade <= 0.0) {
                continue;
            }
            double peso = -std::log(a.probabilidade);
            if (peso < 0.0) {
                peso = 0.0;
            }
            int v = a.destino;
            if (dist[u] + peso < dist[v]) {
                dist[v] = dist[u] + peso;
                pai[v] = u;
            }
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duracao = fim - inicio;
    res.tempo_ms = duracao.count();

    if (dist[id_destino] < INF) {
        res.alcancavel = true;
        res.custo_neg_log = dist[id_destino];
        res.probabilidade_caminho = std::exp(-dist[id_destino]);
        reconstruir_caminho(g, id_origem, id_destino, pai, res);
    }

    return res;
}

std::string formatar_caminho(const ResultadoCaminho& res) {
    if (!res.alcancavel || res.caminho_nomes.empty()) {
        return "INALCANCAVEL";
    }
    std::ostringstream oss;
    for (std::size_t i = 0; i < res.caminho_nomes.size(); ++i) {
        if (i > 0) {
            oss << " -> ";
        }
        oss << res.caminho_nomes[i];
    }
    return oss.str();
}

void salvar_csv_caminhos_virada(const std::string& caminho_csv,
                                const Grafo& g_com,
                                const ResultadoToposort& topo_com,
                                const Grafo& g_sem,
                                const ResultadoToposort& topo_sem) {
    std::ofstream arq(caminho_csv);
    if (!arq.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo CSV: " + caminho_csv);
    }

    arq << "contexto,estado_origem,janela,saldo,n_obs,alcancavel,prob_caminho_mais_provavel,"
        << "custo_neg_log,tempo_dijkstra_ms,tempo_dag_ms,caminho\n";
    arq << std::fixed << std::setprecision(6);

    auto exportar_contexto = [&](const std::string& nome_ctx,
                                 const Grafo& g,
                                 const ResultadoToposort& topo) {
        int id_class = g.obter_id("CLASSIFICADO");
        for (int t = 0; t <= 7; ++t) {
            for (int s = -3; s <= -1; ++s) {
                std::string estado = "P2|S" + std::to_string(s) + "|T" + std::to_string(t);
                int id_orig = g.obter_id(estado);
                if (id_orig == -1) {
                    continue;
                }

                ResultadoCaminho rc_dijk = caminho_mais_provavel_dijkstra(g, id_orig, id_class);
                ResultadoCaminho rc_dag  = caminho_mais_provavel_dag(g, topo, id_orig, id_class);

                arq << nome_ctx << ","
                    << estado << ","
                    << t << ","
                    << s << ","
                    << g.observacoes_estado(id_orig) << ","
                    << (rc_dijk.alcancavel ? 1 : 0) << ","
                    << rc_dijk.probabilidade_caminho << ","
                    << (rc_dijk.alcancavel ? rc_dijk.custo_neg_log : -1.0) << ","
                    << rc_dijk.tempo_ms << ","
                    << rc_dag.tempo_ms << ","
                    << "\"" << formatar_caminho(rc_dijk) << "\"\n";
            }
        }
    };

    exportar_contexto("COM_TORCIDA", g_com, topo_com);
    exportar_contexto("SEM_TORCIDA", g_sem, topo_sem);
}
