/**
 * @file grafo.cpp
 * @brief Implementação da classe Grafo e da função construtora por contexto/perna.
 */

#include "grafo.hpp"

#include <stdexcept>

int Grafo::obter_ou_criar_vertice(const std::string& nome) {
    auto it = nome_para_id_.find(nome);
    if (it != nome_para_id_.end()) {
        return it->second;
    }

    int novo_id = static_cast<int>(id_para_nome_.size());
    nome_para_id_[nome] = novo_id;
    id_para_nome_.push_back(nome);
    adj_.emplace_back();
    total_saidas_.push_back(0);
    total_entradas_.push_back(0);
    grau_entrada_.push_back(0);

    return novo_id;
}

int Grafo::obter_id(const std::string& nome) const {
    auto it = nome_para_id_.find(nome);
    if (it == nome_para_id_.end()) {
        return -1;
    }
    return it->second;
}

const std::string& Grafo::obter_nome(int id) const {
    if (id < 0 || id >= static_cast<int>(id_para_nome_.size())) {
        throw std::out_of_range("ID de vertice fora do intervalo valido.");
    }
    return id_para_nome_[id];
}

void Grafo::adicionar_transicao(const std::string& origem, const std::string& destino) {
    int u = obter_ou_criar_vertice(origem);
    int v = obter_ou_criar_vertice(destino);

    // Verifica se a aresta (u -> v) já existe na lista de adjacência de u
    bool encontrada = false;
    for (Aresta& aresta : adj_[u]) {
        if (aresta.destino == v) {
            aresta.contagem += 1;
            encontrada = true;
            break;
        }
    }

    if (!encontrada) {
        adj_[u].push_back(Aresta{v, 1, 0.0});
        grau_entrada_[v] += 1;
    }

    total_saidas_[u] += 1;
    total_entradas_[v] += 1;

    // Atualiza probabilidades das arestas que saem de u
    double denom = static_cast<double>(total_saidas_[u]);
    for (Aresta& aresta : adj_[u]) {
        aresta.probabilidade = static_cast<double>(aresta.contagem) / denom;
    }
}

void Grafo::recalcular_probabilidades() {
    for (std::size_t u = 0; u < adj_.size(); ++u) {
        if (total_saidas_[u] <= 0) {
            continue;
        }
        double denom = static_cast<double>(total_saidas_[u]);
        for (Aresta& aresta : adj_[u]) {
            aresta.probabilidade = static_cast<double>(aresta.contagem) / denom;
        }
    }
}

int Grafo::num_vertices() const {
    return static_cast<int>(id_para_nome_.size());
}

int Grafo::num_arestas_distintas() const {
    int total = 0;
    for (const auto& lista : adj_) {
        total += static_cast<int>(lista.size());
    }
    return total;
}

int Grafo::total_transicoes_observadas() const {
    int soma = 0;
    for (int s : total_saidas_) {
        soma += s;
    }
    return soma;
}

int Grafo::observacoes_estado(int u) const {
    if (total_saidas_[u] > 0) {
        return total_saidas_[u];
    }
    return total_entradas_[u];
}

int Grafo::total_saidas(int u) const {
    return total_saidas_[u];
}

int Grafo::total_entradas(int u) const {
    return total_entradas_[u];
}

int Grafo::grau_entrada(int u) const {
    return grau_entrada_[u];
}

const std::vector<Aresta>& Grafo::vizinhos(int u) const {
    return adj_[u];
}

Grafo construir_grafo(const std::vector<TransicaoRow>& transicoes,
                      const std::string& filtro_contexto,
                      int filtro_perna) {
    Grafo g;
    std::string prefixo_perna = "";
    if (filtro_perna == 1) {
        prefixo_perna = "P1|";
    } else if (filtro_perna == 2) {
        prefixo_perna = "P2|";
    }

    for (const TransicaoRow& t : transicoes) {
        if (filtro_contexto != "TODOS" && t.contexto != filtro_contexto) {
            continue;
        }
        if (!prefixo_perna.empty() && t.origem.rfind(prefixo_perna, 0) != 0) {
            continue;
        }
        g.adicionar_transicao(t.origem, t.destino);
    }

    g.recalcular_probabilidades();
    return g;
}
