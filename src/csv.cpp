/**
 * @file csv.cpp
 * @brief Implementação do leitor de arquivos CSV sem dependências externas.
 *
 * Contém funções auxiliares para remover '\r' (finais de linha CRLF),
 * separar colunas por vírgula e converter os campos para tipos nativos C++.
 */

#include "csv.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

void remover_cr(std::string& s) {
    if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
}

std::vector<std::string> dividir_linha_csv(const std::string& linha_bruta) {
    std::string linha = linha_bruta;
    remover_cr(linha);

    std::vector<std::string> campos;
    std::stringstream ss(linha);
    std::string item;

    while (std::getline(ss, item, ',')) {
        remover_cr(item);
        campos.push_back(item);
    }

    // Se a linha terminar exatamente com uma vírgula (ex.: campo 'classificou' vazio
    // antes da adição das colunas de valor, ou 'descricao' vazia no final da linha),
    // std::getline não gera o último token vazio automaticamente.
    if (!linha.empty() && linha.back() == ',') {
        campos.push_back("");
    }

    return campos;
}

/**
 * @brief Converte string para int, retornando valor_padrao se a string estiver vazia.
 */
static int para_int_seguro(const std::string& s, int valor_padrao = -1) {
    if (s.empty()) {
        return valor_padrao;
    }
    return std::stoi(s);
}

/**
 * @brief Converte string para double, retornando valor_padrao se a string estiver vazia.
 */
static double para_double_seguro(const std::string& s, double valor_padrao = 0.0) {
    if (s.empty()) {
        return valor_padrao;
    }
    return std::stod(s);
}

std::vector<JogoRow> ler_jogos_csv(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        throw std::runtime_error("Erro ao abrir arquivo CSV de jogos: " + caminho);
    }

    std::vector<JogoRow> jogos;
    std::string linha;

    // Lê e descarta o cabeçalho
    if (!std::getline(arquivo, linha)) {
        return jogos;
    }

    int num_linha = 1;
    while (std::getline(arquivo, linha)) {
        num_linha++;
        remover_cr(linha);
        if (linha.empty()) {
            continue;
        }

        std::vector<std::string> c = dividir_linha_csv(linha);
        if (c.size() < 25) {
            throw std::runtime_error("Linha " + std::to_string(num_linha) +
                                     " de jogos_ko.csv possui apenas " +
                                     std::to_string(c.size()) + " colunas (esperado: 25).");
        }

        JogoRow j;
        j.game_id            = para_int_seguro(c[0]);
        j.season             = para_int_seguro(c[1]);
        j.fase               = c[2];
        j.perna              = para_int_seguro(c[3]);
        j.id_par             = para_int_seguro(c[4], -1); // vazio na ida e em jogo único
        j.date               = c[5];
        j.home_club_id       = para_int_seguro(c[6]);
        j.home_club_name     = c[7];
        j.away_club_id       = para_int_seguro(c[8]);
        j.away_club_name     = c[9];
        j.home_club_goals    = para_int_seguro(c[10]);
        j.away_club_goals    = para_int_seguro(c[11]);
        j.stadium            = c[12];
        j.publico            = para_int_seguro(c[13]);
        j.contexto           = c[14];
        j.agg_antes_mandante = para_int_seguro(c[15]);
        j.sofreu_primeiro    = para_int_seguro(c[16]);
        j.min_primeiro_gol   = para_int_seguro(c[17]);
        j.vermelhos_mandante = para_int_seguro(c[18]);
        j.resultado          = c[19];
        j.classificou        = para_int_seguro(c[20], -1); // vazio nos jogos de ida
        j.valor_mandante_mi  = para_double_seguro(c[21]);
        j.valor_visitante_mi = para_double_seguro(c[22]);
        j.dif_valor_mi       = para_double_seguro(c[23]);
        j.mandante_favorito  = para_int_seguro(c[24]);

        jogos.push_back(j);
    }

    return jogos;
}

std::vector<EventoRow> ler_eventos_csv(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        throw std::runtime_error("Erro ao abrir arquivo CSV de eventos: " + caminho);
    }

    std::vector<EventoRow> eventos;
    std::string linha;

    // Lê e descarta o cabeçalho
    if (!std::getline(arquivo, linha)) {
        return eventos;
    }

    int num_linha = 1;
    while (std::getline(arquivo, linha)) {
        num_linha++;
        remover_cr(linha);
        if (linha.empty()) {
            continue;
        }

        std::vector<std::string> c = dividir_linha_csv(linha);
        if (c.size() < 6) {
            throw std::runtime_error("Linha " + std::to_string(num_linha) +
                                     " de eventos_ko.csv possui menos de 6 colunas.");
        }

        EventoRow e;
        // 'minuto' pode vir como inteiro ou ponto flutuante (ex.: "45" ou "45.0")
        e.game_id   = para_int_seguro(c[0]);
        e.minuto    = static_cast<int>(para_double_seguro(c[1]));
        e.tipo      = c[2];
        e.lado      = c[3];
        e.player_id = para_int_seguro(c[4], -1);
        e.descricao = c[5];

        eventos.push_back(e);
    }

    return eventos;
}

std::vector<TransicaoRow> ler_transicoes_csv(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        throw std::runtime_error("Erro ao abrir arquivo CSV de transicoes: " + caminho);
    }

    std::vector<TransicaoRow> transicoes;
    std::string linha;

    // Lê e descarta o cabeçalho
    if (!std::getline(arquivo, linha)) {
        return transicoes;
    }

    int num_linha = 1;
    while (std::getline(arquivo, linha)) {
        num_linha++;
        remover_cr(linha);
        if (linha.empty()) {
            continue;
        }

        std::vector<std::string> c = dividir_linha_csv(linha);
        if (c.size() < 4) {
            throw std::runtime_error("Linha " + std::to_string(num_linha) +
                                     " de transicoes.csv possui menos de 4 colunas.");
        }

        TransicaoRow t;
        t.game_id  = para_int_seguro(c[0]);
        t.contexto = c[1];
        t.origem   = c[2];
        t.destino  = c[3];

        transicoes.push_back(t);
    }

    return transicoes;
}
