/**
 * @file alcancabilidade.cpp
 * @brief Implementação de BFS e DFS para análise de alcançabilidade de viradas no DAG.
 */

#include "alcancabilidade.hpp"

#include <chrono>
#include <fstream>
#include <queue>
#include <stdexcept>

ResultadoBusca bfs_alcancabilidade(const Grafo& g, int id_origem) {
    auto inicio = std::chrono::high_resolution_clock::now();

    int V = g.num_vertices();
    ResultadoBusca res;
    res.visitado.assign(V, false);
    res.tempo_ms = 0.0;

    if (id_origem < 0 || id_origem >= V) {
        return res;
    }

    std::queue<int> fila;
    res.visitado[id_origem] = true;
    fila.push(id_origem);

    while (!fila.empty()) {
        int u = fila.front();
        fila.pop();
        res.ordem_visita.push_back(u);

        for (const Aresta& a : g.vizinhos(u)) {
            int v = a.destino;
            if (!res.visitado[v]) {
                res.visitado[v] = true;
                fila.push(v);
            }
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duracao = fim - inicio;
    res.tempo_ms = duracao.count();

    return res;
}

/**
 * @brief Função auxiliar recursiva para a Busca em Profundidade (DFS).
 */
static void dfs_recursivo(const Grafo& g, int u, ResultadoBusca& res) {
    res.visitado[u] = true;
    res.ordem_visita.push_back(u);

    for (const Aresta& a : g.vizinhos(u)) {
        int v = a.destino;
        if (!res.visitado[v]) {
            dfs_recursivo(g, v, res);
        }
    }
}

ResultadoBusca dfs_alcancabilidade(const Grafo& g, int id_origem) {
    auto inicio = std::chrono::high_resolution_clock::now();

    int V = g.num_vertices();
    ResultadoBusca res;
    res.visitado.assign(V, false);
    res.tempo_ms = 0.0;

    if (id_origem >= 0 && id_origem < V) {
        dfs_recursivo(g, id_origem, res);
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duracao = fim - inicio;
    res.tempo_ms = duracao.count();

    return res;
}

std::vector<ComparacaoViradaEstado> comparar_alcancabilidade_virada(const Grafo& g_com,
                                                                    const Grafo& g_sem) {
    std::vector<ComparacaoViradaEstado> resultado;

    int id_class_com = g_com.obter_id("CLASSIFICADO");
    int id_class_sem = g_sem.obter_id("CLASSIFICADO");

    for (int t = 0; t <= 7; ++t) {
        for (int s = -3; s <= -1; ++s) {
            std::string nome = "P2|S" + std::to_string(s) + "|T" + std::to_string(t);
            int id_com = g_com.obter_id(nome);
            int id_sem = g_sem.obter_id(nome);

            if (id_com == -1 && id_sem == -1) {
                continue;
            }

            ComparacaoViradaEstado item;
            item.estado = nome;
            item.janela = t;
            item.saldo = s;

            // Contexto COM_TORCIDA
            item.existe_com_torcida = (id_com != -1);
            item.n_com_torcida = (id_com != -1) ? g_com.observacoes_estado(id_com) : 0;
            item.vira_com_torcida = false;
            if (id_com != -1 && id_class_com != -1) {
                ResultadoBusca bfs_c = bfs_alcancabilidade(g_com, id_com);
                ResultadoBusca dfs_c = dfs_alcancabilidade(g_com, id_com);
                // Verifica consistência entre BFS e DFS
                if (bfs_c.visitado[id_class_com] != dfs_c.visitado[id_class_com]) {
                    throw std::runtime_error("Inconsistencia entre BFS e DFS para " + nome);
                }
                item.vira_com_torcida = bfs_c.visitado[id_class_com];
            }

            // Contexto SEM_TORCIDA
            item.existe_sem_torcida = (id_sem != -1);
            item.n_sem_torcida = (id_sem != -1) ? g_sem.observacoes_estado(id_sem) : 0;
            item.vira_sem_torcida = false;
            if (id_sem != -1 && id_class_sem != -1) {
                ResultadoBusca bfs_s = bfs_alcancabilidade(g_sem, id_sem);
                ResultadoBusca dfs_s = dfs_alcancabilidade(g_sem, id_sem);
                if (bfs_s.visitado[id_class_sem] != dfs_s.visitado[id_class_sem]) {
                    throw std::runtime_error("Inconsistencia entre BFS e DFS para " + nome);
                }
                item.vira_sem_torcida = bfs_s.visitado[id_class_sem];
            }

            if (item.vira_com_torcida && item.vira_sem_torcida) {
                item.categoria = "AMBOS_VIRAM";
            } else if (item.vira_com_torcida && !item.vira_sem_torcida) {
                item.categoria = "SO_COM_TORCIDA_VIRA";
            } else if (!item.vira_com_torcida && item.vira_sem_torcida) {
                item.categoria = "SO_SEM_TORCIDA_VIRA";
            } else {
                item.categoria = "NENHUM_VIRA";
            }

            resultado.push_back(item);
        }
    }

    return resultado;
}

void salvar_csv_alcancabilidade(const std::string& caminho_csv,
                                const std::vector<ComparacaoViradaEstado>& tabela) {
    std::ofstream arq(caminho_csv);
    if (!arq.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo CSV: " + caminho_csv);
    }

    arq << "estado,janela,saldo,existe_com_torcida,n_com_torcida,vira_com_torcida,"
        << "existe_sem_torcida,n_sem_torcida,vira_sem_torcida,categoria\n";

    for (const ComparacaoViradaEstado& r : tabela) {
        arq << r.estado << ","
            << r.janela << ","
            << r.saldo << ","
            << (r.existe_com_torcida ? 1 : 0) << ","
            << r.n_com_torcida << ","
            << (r.vira_com_torcida ? 1 : 0) << ","
            << (r.existe_sem_torcida ? 1 : 0) << ","
            << r.n_sem_torcida << ","
            << (r.vira_sem_torcida ? 1 : 0) << ","
            << r.categoria << "\n";
    }
}
