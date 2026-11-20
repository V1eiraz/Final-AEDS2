/**
 * @file sintetico.cpp
 * @brief Implementação dos geradores de dados sintéticos e benchmark de escalabilidade.
 */

#include "sintetico.hpp"

#include "alcancabilidade.hpp"
#include "caminho.hpp"
#include "dp.hpp"
#include "toposort.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

/**
 * @brief Gerador pseudoaleatório Xorshift32 leve e determinístico.
 */
struct Xorshift32 {
    std::uint32_t estado;
    explicit Xorshift32(unsigned int semente)
        : estado(semente == 0 ? 42u : static_cast<std::uint32_t>(semente)) {}

    std::uint32_t proximo() {
        std::uint32_t x = estado;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        estado = x;
        return x;
    }

    int inteiro(int min_val, int max_val) {
        if (max_val <= min_val) return min_val;
        std::uint32_t span = static_cast<std::uint32_t>(max_val - min_val + 1);
        return min_val + static_cast<int>(proximo() % span);
    }

    double uniforme(double min_val, double max_val) {
        double u = static_cast<double>(proximo()) / static_cast<double>(0xFFFFFFFFu);
        return min_val + u * (max_val - min_val);
    }
};

/**
 * @brief Calcula média e desvio-padrão amostral de um vetor de tempos em milissegundos.
 */
void calcular_media_desvio(const std::vector<double>& tempos, double& media, double& desvio) {
    if (tempos.empty()) {
        media = 0.0;
        desvio = 0.0;
        return;
    }
    double soma = 0.0;
    for (double t : tempos) {
        soma += t;
    }
    media = soma / static_cast<double>(tempos.size());

    if (tempos.size() <= 1) {
        desvio = 0.0;
        return;
    }
    double sq = 0.0;
    for (double t : tempos) {
        double d = t - media;
        sq += d * d;
    }
    desvio = std::sqrt(sq / static_cast<double>(tempos.size() - 1));
}

} // namespace

Grafo gerar_dag_sintetico(int V, unsigned int semente) {
    if (V < 10) {
        V = 10;
    }

    Grafo g;
    g.inicializar_vertices_sinteticos(V);

    Xorshift32 rng(semente);
    int id_classificado = V - 2;
    int id_eliminado = V - 1;
    int ultimo_intermediario = V - 3;

    // Para cada vértice intermediário u in [0, V-3], criamos 3 arestas direcionadas
    // para vértices posteriores (v > u), garantindo aciclicidade estrita (DAG) e E ~ 3*V.
    for (int u = 0; u <= ultimo_intermediario; ++u) {
        int restantes = ultimo_intermediario - u;

        if (restantes > 15) {
            // Conecta para 3 estados seguintes em janelas distintas (garante alcance até o fim)
            int v1 = u + rng.inteiro(1, 5);
            int v2 = u + rng.inteiro(6, 10);
            int v3 = u + rng.inteiro(11, 15);

            g.adicionar_aresta_id(u, v1, rng.inteiro(1, 10));
            g.adicionar_aresta_id(u, v2, rng.inteiro(1, 10));
            g.adicionar_aresta_id(u, v3, rng.inteiro(1, 10));
        } else {
            // Última camada antes dos terminais: transita para CLASSIFICADO e ELIMINADO
            if (u + 1 <= ultimo_intermediario) {
                g.adicionar_aresta_id(u, u + 1, rng.inteiro(1, 10));
            }
            g.adicionar_aresta_id(u, id_classificado, rng.inteiro(1, 10));
            g.adicionar_aresta_id(u, id_eliminado, rng.inteiro(1, 10));
        }
    }

    g.recalcular_probabilidades();
    return g;
}

std::vector<AmostraArvore> gerar_amostras_sinteticas_arvore(int num_amostras,
                                                            unsigned int semente) {
    Xorshift32 rng(semente);
    std::vector<AmostraArvore> amostras;
    amostras.reserve(num_amostras);

    const std::vector<std::string> fases = {"R16", "QF", "SF", "F"};
    const std::vector<std::string> contextos = {"COM_TORCIDA", "SEM_TORCIDA", "NEUTRO"};

    for (int i = 0; i < num_amostras; ++i) {
        AmostraArvore a;
        a.game_id = 1000000 + i;

        int perna = rng.inteiro(1, 2);
        int agg_antes = (perna == 1) ? 0 : rng.inteiro(-3, 3);
        int sofreu_prim = rng.inteiro(0, 1);
        double min_gol = (rng.inteiro(1, 100) <= 6) ? 121.0 : rng.uniforme(1.0, 115.0);
        int vermelhos = (rng.inteiro(1, 100) <= 10) ? 1 : 0;
        double dif_valor = rng.uniforme(-450.0, 450.0);
        int favorito = (dif_valor > 0.0) ? 1 : 0;

        std::string fase = fases[rng.inteiro(0, 3)];
        std::string ctx = contextos[rng.inteiro(0, 2)];

        a.num_vals = {
            static_cast<double>(perna),
            static_cast<double>(agg_antes),
            static_cast<double>(sofreu_prim),
            min_gol,
            static_cast<double>(vermelhos),
            dif_valor,
            static_cast<double>(favorito)
        };
        a.cat_vals = {fase, ctx};

        // Regra latente baseada nas relações reais observadas + ruído de 15%
        double score = 0.0;
        score += (sofreu_prim == 0) ? 1.2 : -1.2;
        score += 0.45 * static_cast<double>(agg_antes);
        score += 0.002 * dif_valor;
        score += (ctx == "COM_TORCIDA") ? 0.3 : -0.1;
        score += rng.uniforme(-0.9, 0.9);

        a.rotulo = (score >= 0.0) ? 1 : 0;
        amostras.push_back(a);
    }

    return amostras;
}

std::vector<RegistroEscalabilidade> executar_experimentos_escalabilidade(
    const std::string& caminho_csv) {
    std::vector<RegistroEscalabilidade> resultados;

    const std::vector<int> tamanhos_grafo = {100, 1000, 10000, 100000};
    const int REPS_GRAFO = 5;

    std::cout << "========================================================================================\n";
    std::cout << "ETAPA 6: TESTES DE ESCALABILIDADE ASSINTOTICA (DADOS SINTETICOS)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << ">>> 6.1 Escalabilidade dos Algoritmos em Grafo (DAG: V = 10^2 a 10^5, E ~ 3*V)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(22) << "Algoritmo"
              << std::right
              << std::setw(10) << "V"
              << std::setw(12) << "E"
              << std::setw(16) << "Tempo Medio(ms)"
              << std::setw(14) << "Desvio(ms)"
              << std::setw(18) << "Complexidade"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (int V : tamanhos_grafo) {
        Grafo g = gerar_dag_sintetico(V, 12345u + static_cast<unsigned int>(V));
        int E = g.num_arestas_distintas();
        int id_origem = 0;
        int id_destino = V - 2; // CLASSIFICADO

        std::vector<double> t_kahn, t_dp, t_dag_sp, t_dijk, t_bfs, t_dfs;
        ResultadoToposort topo_ref;

        for (int r = 0; r < REPS_GRAFO; ++r) {
            ResultadoToposort topo = ordenacao_topologica_kahn(g);
            t_kahn.push_back(topo.tempo_ms);
            if (r == 0) topo_ref = topo;

            ResultadoDP dp = calcular_dp_resiliencia(g, topo_ref, false);
            t_dp.push_back(dp.tempo_ms);

            ResultadoCaminho rc_dag = caminho_mais_provavel_dag(g, topo_ref, id_origem, id_destino);
            t_dag_sp.push_back(rc_dag.tempo_ms);

            ResultadoCaminho rc_dijk = caminho_mais_provavel_dijkstra(g, id_origem, id_destino);
            t_dijk.push_back(rc_dijk.tempo_ms);

            ResultadoBusca rb_bfs = bfs_alcancabilidade(g, id_origem);
            t_bfs.push_back(rb_bfs.tempo_ms);

            ResultadoBusca rb_dfs = dfs_alcancabilidade(g, id_origem);
            t_dfs.push_back(rb_dfs.tempo_ms);
        }

        auto registrar = [&](const std::string& nome_alg,
                             const std::vector<double>& tempos,
                             const std::string& comp) {
            double media = 0.0, desvio = 0.0;
            calcular_media_desvio(tempos, media, desvio);
            RegistroEscalabilidade reg{
                nome_alg, "GRAFO_DAG", V, E, REPS_GRAFO, media, desvio, comp
            };
            resultados.push_back(reg);

            std::cout << std::left  << std::setw(22) << nome_alg
                      << std::right << std::setw(10) << V
                      << std::setw(12) << E
                      << std::fixed << std::setprecision(4)
                      << std::setw(16) << media
                      << std::setw(14) << desvio
                      << std::setw(18) << comp
                      << "\n";
        };

        registrar("Kahn_Toposort",    t_kahn,   "O(V + E)");
        registrar("DP_Resiliencia",   t_dp,     "O(V + E)");
        registrar("DAG_CaminhoMin",   t_dag_sp, "O(V + E)");
        registrar("Dijkstra_MinHeap", t_dijk,   "O((V+E) log V)");
        registrar("BFS_Alcance",      t_bfs,    "O(V + E)");
        registrar("DFS_Alcance",      t_dfs,    "O(V + E)");
        std::cout << "  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .\n";
    }

    // 6.2 Escalabilidade do Treinamento da Árvore CART: n in {10^2, 10^3, 10^4}
    const std::vector<int> tamanhos_arvore = {100, 1000, 10000};
    const int REPS_ARVORE = 5;
    ConfigArvore cfg_escala{CriterioImpureza::GINI, 3, 5, 1e-7};

    std::cout << "\n>>> 6.2 Escalabilidade do Treinamento da Arvore CART (n = 10^2 a 10^4, 9 atributos)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(22) << "Algoritmo"
              << std::right
              << std::setw(10) << "Amostras(n)"
              << std::setw(12) << "Atributos"
              << std::setw(16) << "Tempo Medio(ms)"
              << std::setw(14) << "Desvio(ms)"
              << std::setw(18) << "Complexidade"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (int n : tamanhos_arvore) {
        std::vector<AmostraArvore> amostras = gerar_amostras_sinteticas_arvore(n, 54321u + static_cast<unsigned int>(n));
        std::vector<double> t_cart;

        for (int r = 0; r < REPS_ARVORE; ++r) {
            ArvoreCART arv;
            auto ini = std::chrono::high_resolution_clock::now();
            arv.treinar(amostras, cfg_escala, ConjuntoAtributos::COMPLETO);
            auto fim = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> dur = fim - ini;
            t_cart.push_back(dur.count());
        }

        double media = 0.0, desvio = 0.0;
        calcular_media_desvio(t_cart, media, desvio);
        RegistroEscalabilidade reg{
            "CART_Treino", "ARVORE_CART", n, 9, REPS_ARVORE, media, desvio, "O(N * M * log N)"
        };
        resultados.push_back(reg);

        std::cout << std::left  << std::setw(22) << reg.algoritmo
                  << std::right << std::setw(10) << n
                  << std::setw(12) << 9
                  << std::fixed << std::setprecision(4)
                  << std::setw(16) << media
                  << std::setw(14) << desvio
                  << std::setw(18) << reg.complexidade_teorica
                  << "\n";
    }
    std::cout << "========================================================================================\n\n";

    // Grava o CSV results/escalabilidade.csv
    std::ofstream arq(caminho_csv);
    if (!arq.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo CSV de escalabilidade: " + caminho_csv);
    }

    arq << "algoritmo,categoria,tamanho,secundario,repeticoes,tempo_medio_ms,tempo_desvio_ms,complexidade_teorica\n";
    arq << std::fixed << std::setprecision(6);
    for (const RegistroEscalabilidade& r : resultados) {
        arq << r.algoritmo << ","
            << r.categoria << ","
            << r.tamanho_entrada << ","
            << r.tamanho_secundario << ","
            << r.repeticoes << ","
            << r.tempo_medio_ms << ","
            << r.tempo_desvio_ms << ","
            << "\"" << r.complexidade_teorica << "\"\n";
    }

    return resultados;
}
