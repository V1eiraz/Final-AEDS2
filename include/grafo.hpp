/**
 * @file grafo.hpp
 * @brief Estrutura de Grafo Direcionado Ponderado (Lista de Adjacência) para os Estados de Jogo.
 *
 * Modela a evolução de um confronto de mata-mata da UCL como um Grafo Acíclico
 * Direcionado (DAG), onde cada vértice é um estado "P{perna}|S{saldo}|T{janela}"
 * (ou estado terminal CLASSIFICADO, ELIMINADO, FIM_IDA) e cada aresta acumula
 * a contagem de transições observadas e sua probabilidade empírica.
 */

#ifndef GRAFO_HPP
#define GRAFO_HPP

#include "csv.hpp"

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Representa uma aresta direcionada saindo de um vértice u para 'destino'.
 */
struct Aresta {
    int destino;            // ID inteiro do vértice de destino (0 .. V-1)
    int contagem;           // Número de vezes que a transição (u -> destino) foi observada
    double probabilidade;   // Probabilidade empírica: contagem / soma das saídas de u
};

/**
 * @brief Grafo direcionado ponderado implementado do zero com lista de adjacência.
 */
class Grafo {
public:
    Grafo() = default;

    /**
     * @brief Obtém o ID inteiro de um estado pelo nome, criando um novo vértice se não existir.
     * @param nome String identificadora do estado (ex.: "P2|S-1|T4").
     * @return ID inteiro no intervalo [0, V-1].
     */
    int obter_ou_criar_vertice(const std::string& nome);

    /**
     * @brief Consulta o ID inteiro de um estado sem criar novos vértices.
     * @param nome String identificadora do estado.
     * @return ID do vértice ou -1 caso não pertença ao grafo.
     */
    int obter_id(const std::string& nome) const;

    /**
     * @brief Retorna o nome (string do estado) associado a um ID inteiro.
     * @param id Índice do vértice no intervalo [0, V-1].
     * @return Referência constante para o nome do estado.
     */
    const std::string& obter_nome(int id) const;

    /**
     * @brief Registra uma ocorrência da transição (origem -> destino).
     *
     * Se a aresta já existir, incrementa sua contagem em +1. Caso contrário,
     * insere uma nova aresta na lista de adjacência de 'origem'.
     * Atualiza também as contagens de saída/entrada e recalcula probabilidades da origem.
     */
    void adicionar_transicao(const std::string& origem, const std::string& destino);

    /**
     * @brief Recalcula as probabilidades de todas as arestas do grafo.
     */
    void recalcular_probabilidades();

    /**
     * @brief Número total de vértices (V).
     */
    int num_vertices() const;

    /**
     * @brief Número de arestas direcionadas distintas (E).
     */
    int num_arestas_distintas() const;

    /**
     * @brief Soma das contagens de todas as transições adicionadas ao grafo.
     */
    int total_transicoes_observadas() const;

    /**
     * @brief Retorna o número de observações (n) de um vértice u.
     *
     * Para estados não-terminais, equivale ao total de transições que saem de u.
     * Para estados terminais (grau de saída 0), equivale ao total de transições que entram em u.
     */
    int observacoes_estado(int u) const;

    /**
     * @brief Retorna o total de transições que saem do vértice u.
     */
    int total_saidas(int u) const;

    /**
     * @brief Retorna o total de transições que chegam ao vértice u.
     */
    int total_entradas(int u) const;

    /**
     * @brief Retorna o grau de entrada (número de arestas distintas que chegam em u).
     */
    int grau_entrada(int u) const;

    /**
     * @brief Retorna a lista de arestas que saem do vértice u.
     */
    const std::vector<Aresta>& vizinhos(int u) const;

private:
    std::unordered_map<std::string, int> nome_para_id_;
    std::vector<std::string> id_para_nome_;
    std::vector<std::vector<Aresta>> adj_;
    std::vector<int> total_saidas_;     // Soma das contagens que saem de cada vértice
    std::vector<int> total_entradas_;   // Soma das contagens que entram em cada vértice
    std::vector<int> grau_entrada_;     // Quantidade de arestas distintas que entram no vértice
};

/**
 * @brief Constrói um Grafo filtrando as transições por contexto e (opcionalmente) por perna.
 *
 * @param transicoes Vetor de transições lidas de data/transicoes.csv.
 * @param filtro_contexto "COM_TORCIDA", "SEM_TORCIDA" ou "TODOS" (para não filtrar contexto).
 * @param filtro_perna -1 para ambas as pernas, 1 para apenas ida ("P1|..."), 2 para apenas volta ("P2|...").
 * @return Objeto Grafo populado e com probabilidades calculadas.
 */
Grafo construir_grafo(const std::vector<TransicaoRow>& transicoes,
                      const std::string& filtro_contexto,
                      int filtro_perna = -1);

#endif // GRAFO_HPP
