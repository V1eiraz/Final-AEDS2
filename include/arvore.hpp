/**
 * @file arvore.hpp
 * @brief Árvore de Decisão CART (Classification and Regression Trees) implementada do zero.
 *
 * Suporta:
 *   - Critérios de impureza: Gini e Entropia de Shannon;
 *   - Divisões binárias por limiar (atributos numéricos) e por igualdade (atributos categóricos);
 *   - Pré-poda por profundidade máxima (max_depth) e mínimo de amostras por folha (min_samples_leaf);
 *   - Validação cruzada k-fold (k = 5) com embaralhamento Fisher-Yates próprio e semente fixa;
 *   - Matriz de confusão, acurácia média e desvio-padrão entre folds;
 *   - Comparação com Baselines obrigatórios (Classe Majoritária e Árvore Só com Controle);
 *   - Cálculo da importância dos atributos por redução ponderada de impureza;
 *   - Exportação da árvore treinada em texto e em formato DOT (Graphviz).
 */

#ifndef ARVORE_HPP
#define ARVORE_HPP

#include "csv.hpp"

#include <memory>
#include <string>
#include <vector>

/**
 * @brief Critério de impureza utilizado para escolher o melhor corte (split) na árvore.
 */
enum class CriterioImpureza {
    GINI,
    ENTROPIA
};

/**
 * @brief Define quais grupos de atributos preditores serão disponibilizados para a árvore.
 */
enum class ConjuntoAtributos {
    COMPLETO,       // 7 atributos de pressão + 2 de controle (9 atributos)
    SO_CONTROLE,    // Apenas dif_valor_mi e mandante_favorito (Baseline de força)
    SO_PRESSAO      // Apenas os 7 atributos de pressão psicológica/contexto
};

/**
 * @brief Hiperparâmetros de treinamento e poda da Árvore CART.
 */
struct ConfigArvore {
    CriterioImpureza criterio = CriterioImpureza::GINI;
    int max_depth = 3;              // Profundidade máxima da árvore (poda)
    int min_samples_leaf = 5;       // Mínimo de amostras exigido em cada folha
    double min_impurity_decrease = 1e-7;
};

/**
 * @brief Uma amostra (linha de jogo) preparada para a Árvore de Decisão, sem vazamento de dados.
 *
 * Índices dos atributos numéricos (num_vals):
 *   0: perna (0, 1 ou 2)
 *   1: agg_antes_mandante (-3 a +5)
 *   2: sofreu_primeiro (0 ou 1)
 *   3: min_primeiro_gol (1 a 120; jogos 0x0 sem gol recebem 121)
 *   4: vermelhos_mandante (0, 1, ...)
 *   5: dif_valor_mi (valor mandante - visitante em milhões de euros)
 *   6: mandante_favorito (0 ou 1)
 *
 * Índices dos atributos categóricos (cat_vals):
 *   0: fase ("R16", "QF", "SF", "F")
 *   1: contexto ("COM_TORCIDA", "SEM_TORCIDA", "NEUTRO")
 */
struct AmostraArvore {
    int game_id;
    std::vector<double> num_vals;       // 7 atributos numéricos
    std::vector<std::string> cat_vals;  // 2 atributos categóricos
    int rotulo;                         // 0 ou 1 (alvo de classificação)
};

/**
 * @brief Metadados de cada um dos 9 atributos preditores.
 */
struct InfoAtributo {
    int id_global;              // 0 a 8
    std::string nome;           // Nome legível do atributo
    bool eh_categorico;         // true se categórico, false se numérico
    int indice_vetor;           // Índice dentro de num_vals ou cat_vals
    bool eh_controle;           // true se for atributo de controle financeiro, false se pressão
};

/**
 * @brief Nó da Árvore de Decisão CART.
 */
struct NoArvore {
    int id_no = 0;
    int profundidade = 0;
    bool eh_folha = true;

    // Estatísticas do nó
    int n_amostras = 0;
    int n_classe_0 = 0;
    int n_classe_1 = 0;
    int classe_predita = 0;
    double prob_classe_1 = 0.0;
    double impureza = 0.0;

    // Regra de decisão (apenas se !eh_folha)
    int id_atributo = -1;
    std::string nome_atributo;
    bool eh_categorico = false;
    double limiar_numerico = 0.0;           // Ramo esquerdo: valor <= limiar_numerico
    std::string valor_categorico;           // Ramo esquerdo: valor == valor_categorico
    double reducao_impureza_ponderada = 0.0;

    std::unique_ptr<NoArvore> esq;
    std::unique_ptr<NoArvore> dir;
};

/**
 * @brief Classe da Árvore de Decisão CART implementada do zero.
 */
class ArvoreCART {
public:
    ArvoreCART() = default;

    /**
     * @brief Treina a árvore sobre um conjunto de amostras usando os atributos permitidos.
     */
    void treinar(const std::vector<AmostraArvore>& amostras,
                 const ConfigArvore& config,
                 ConjuntoAtributos conj = ConjuntoAtributos::COMPLETO);

    /**
     * @brief Prediz a classe (0 ou 1) para uma nova amostra.
     */
    int predizer(const AmostraArvore& amostra) const;

    /**
     * @brief Calcula a importância normalizada (soma = 1.0) de cada um dos 9 atributos.
     */
    std::vector<double> importancia_atributos() const;

    /**
     * @brief Retorna ponteiro constante para a raiz da árvore.
     */
    const NoArvore* raiz() const { return raiz_.get(); }

    /**
     * @brief Número total de nós e de folhas da árvore treinada.
     */
    int num_nos() const { return total_nos_; }
    int num_folhas() const { return total_folhas_; }

    /**
     * @brief Exporta a estrutura da árvore em texto indentado legível.
     */
    std::string exportar_texto(const std::string& nome_classe_0,
                               const std::string& nome_classe_1) const;

    /**
     * @brief Exporta a árvore no formato DOT (Graphviz) para geração de figura do artigo.
     */
    void salvar_dot(const std::string& caminho_dot,
                    const std::string& titulo,
                    const std::string& nome_classe_0,
                    const std::string& nome_classe_1) const;

private:
    std::unique_ptr<NoArvore> raiz_;
    ConfigArvore config_;
    std::vector<InfoAtributo> catálogo_atributos_;
    std::vector<int> atributos_ativos_;
    int n_total_treino_ = 0;
    int total_nos_ = 0;
    int total_folhas_ = 0;
    std::vector<double> soma_reducao_impureza_; // Acumulador por id_global (0..8)

    double calcular_impureza(int n0, int n1) const;

    std::unique_ptr<NoArvore> construir_recursivo(const std::vector<AmostraArvore>& amostras,
                                                  const std::vector<int>& indices,
                                                  int profundidade);
};

/**
 * @brief Resultado da Validação Cruzada k-fold (k = 5) para um modelo ou baseline.
 */
struct ResultadoValidacaoCruzada {
    std::string nome_alvo;          // "RESULTADO (V vs Nao-V)" ou "CLASSIFICOU (1 vs 0)"
    std::string nome_modelo;        // Ex.: "Baseline 1: Classe Majoritaria", "CART Completo (Gini)"
    int n_amostras;
    int k_folds;
    std::vector<double> acuracia_por_fold;
    double acuracia_media;
    double acuracia_desvio_padrao;

    // Matriz de confusão acumulada nos k folds de teste
    int vp; // Verdadeiros Positivos (real=1, pred=1)
    int vn; // Verdadeiros Negativos (real=0, pred=0)
    int fp; // Falsos Positivos      (real=0, pred=1)
    int fn; // Falsos Negativos      (real=1, pred=0)
    double precisao;
    double recall;
    double f1_score;
};

/**
 * @brief Retorna a lista estática com os metadados dos 9 atributos preditores.
 */
std::vector<InfoAtributo> obter_catalogo_atributos();

/**
 * @brief Constrói o dataset para a Árvore 1: Resultado da partida (1 = Venceu 'V', 0 = Não venceu 'E'/'D').
 * Usa todos os 139 jogos de data/jogos_ko.csv.
 */
std::vector<AmostraArvore> preparar_amostras_resultado(const std::vector<JogoRow>& jogos);

/**
 * @brief Constrói o dataset para a Árvore 2: Classificação (1 = Classificou, 0 = Eliminado).
 * Usa os 75 jogos decisivos (volta + jogo único) onde classificou != -1.
 */
std::vector<AmostraArvore> preparar_amostras_classificou(const std::vector<JogoRow>& jogos);

/**
 * @brief Embaralha um vetor de índices in-place usando Fisher-Yates e gerador pseudoaleatório próprio (sem std::random_shuffle).
 */
void embaralhar_indices_proprio(std::vector<int>& indices, unsigned int semente);

/**
 * @brief Executa validação cruzada k-fold (k = 5) para o Baseline da Classe Majoritária.
 */
ResultadoValidacaoCruzada validar_kfold_classe_majoritaria(const std::vector<AmostraArvore>& amostras,
                                                           const std::string& nome_alvo,
                                                           int k = 5,
                                                           unsigned int semente = 42);

/**
 * @brief Executa validação cruzada k-fold (k = 5) para a Árvore CART com a configuração dada.
 */
ResultadoValidacaoCruzada validar_kfold_arvore(const std::vector<AmostraArvore>& amostras,
                                               const std::string& nome_alvo,
                                               const std::string& nome_modelo,
                                               const ConfigArvore& config,
                                               ConjuntoAtributos conj,
                                               int k = 5,
                                               unsigned int semente = 42);

/**
 * @brief Executa toda a bateria de experimentos da Etapa 5 (Módulo B), imprime no terminal e salva CSVs/DOTs.
 */
void executar_modulo_arvore(const std::vector<JogoRow>& jogos);

#endif // ARVORE_HPP
