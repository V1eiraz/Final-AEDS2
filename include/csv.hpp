/**
 * @file csv.hpp
 * @brief Declarações das estruturas e funções para leitura dos arquivos CSV do projeto.
 *
 * Este módulo lê os arquivos data/jogos_ko.csv, data/eventos_ko.csv e
 * data/transicoes.csv sem usar bibliotecas externas, tratando quebras de
 * linha no padrão Windows (CRLF / '\r\n').
 */

#ifndef CSV_HPP
#define CSV_HPP

#include <string>
#include <vector>

/**
 * @brief Representa uma linha do arquivo data/jogos_ko.csv (sempre na ótica do mandante).
 */
struct JogoRow {
    int game_id;
    int season;
    std::string fase;
    int perna;                  // 1 = ida, 2 = volta, 0 = jogo único
    int id_par;                 // -1 se vazio (ida ou jogo único)
    std::string date;
    int home_club_id;
    std::string home_club_name;
    int away_club_id;
    std::string away_club_name;
    int home_club_goals;
    int away_club_goals;
    std::string stadium;
    int publico;
    std::string contexto;       // COM_TORCIDA, SEM_TORCIDA ou NEUTRO
    int agg_antes_mandante;
    int sofreu_primeiro;        // 1 se sofreu o 1º gol, 0 caso contrário
    int min_primeiro_gol;       // -1 se o jogo terminou 0x0
    int vermelhos_mandante;
    std::string resultado;      // "V", "E" ou "D"
    int classificou;            // -1 se vazio (jogos de ida), 1 se classificou, 0 se eliminou
    double valor_mandante_mi;
    double valor_visitante_mi;
    double dif_valor_mi;
    int mandante_favorito;      // 1 se dif_valor_mi > 0, 0 caso contrário
};

/**
 * @brief Representa uma linha do arquivo data/eventos_ko.csv.
 */
struct EventoRow {
    int game_id;
    int minuto;
    std::string tipo;           // GOAL, YELLOW, RED, SUB
    std::string lado;           // H (mandante) ou A (visitante)
    int player_id;
    std::string descricao;
};

/**
 * @brief Representa uma linha (aresta observada) do arquivo data/transicoes.csv.
 */
struct TransicaoRow {
    int game_id;
    std::string contexto;       // COM_TORCIDA ou SEM_TORCIDA
    std::string origem;         // Ex.: "P2|S-1|T4"
    std::string destino;        // Ex.: "P2|S0|T5" ou "CLASSIFICADO"
};

/**
 * @brief Remove o caractere '\r' ao final de uma string, caso exista (tratamento de CRLF).
 * @param s Referência para a string a ser limpa in-place.
 */
void remover_cr(std::string& s);

/**
 * @brief Divide uma linha CSV separada por vírgulas em um vetor de strings.
 * @param linha Linha bruta lida do arquivo.
 * @return Vetor com os campos individuais já sem '\r'.
 */
std::vector<std::string> dividir_linha_csv(const std::string& linha);

/**
 * @brief Lê todos os jogos de data/jogos_ko.csv.
 * @param caminho Caminho para o arquivo CSV.
 * @return Vetor com os 139 registros de jogos.
 */
std::vector<JogoRow> ler_jogos_csv(const std::string& caminho);

/**
 * @brief Lê todos os eventos de data/eventos_ko.csv.
 * @param caminho Caminho para o arquivo CSV.
 * @return Vetor com os 1.962 registros de eventos.
 */
std::vector<EventoRow> ler_eventos_csv(const std::string& caminho);

/**
 * @brief Lê todas as transições de estados de data/transicoes.csv.
 * @param caminho Caminho para o arquivo CSV.
 * @return Vetor com as 984 transições observadas.
 */
std::vector<TransicaoRow> ler_transicoes_csv(const std::string& caminho);

#endif // CSV_HPP
