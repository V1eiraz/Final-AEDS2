/**
 * @file toposort.cpp
 * @brief Implementação do algoritmo de Kahn (BFS por grau de entrada) para ordenação topológica.
 */

#include "toposort.hpp"

#include <chrono>
#include <queue>

ResultadoToposort ordenacao_topologica_kahn(const Grafo& g) {
    auto inicio = std::chrono::high_resolution_clock::now();

    int V = g.num_vertices();
    std::vector<int> in_degree(V, 0);

    // 1. Calcula o grau de entrada de cada vértice percorrendo as listas de adjacência: O(V + E)
    for (int u = 0; u < V; ++u) {
        for (const Aresta& a : g.vizinhos(u)) {
            in_degree[a.destino] += 1;
        }
    }

    // 2. Insere na fila todos os vértices iniciais (grau de entrada == 0)
    std::queue<int> fila;
    for (int u = 0; u < V; ++u) {
        if (in_degree[u] == 0) {
            fila.push(u);
        }
    }

    // 3. Extrai vértices da fila e reduz o grau de entrada dos vizinhos
    std::vector<int> ordem;
    ordem.reserve(V);

    while (!fila.empty()) {
        int u = fila.front();
        fila.pop();
        ordem.push_back(u);

        for (const Aresta& a : g.vizinhos(u)) {
            int v = a.destino;
            in_degree[v] -= 1;
            if (in_degree[v] == 0) {
                fila.push(v);
            }
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duracao = fim - inicio;

    ResultadoToposort res;
    res.ordem = ordem;
    // Se nem todos os V vértices entraram na ordenação, existe pelo menos um ciclo no grafo
    res.tem_ciclo = (static_cast<int>(ordem.size()) != V);
    res.tempo_ms = duracao.count();

    return res;
}
