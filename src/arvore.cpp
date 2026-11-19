/**
 * @file arvore.cpp
 * @brief Implementação do zero da Árvore de Decisão CART, Validação Cruzada k-fold e Exportação DOT.
 */

#include "arvore.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

std::vector<InfoAtributo> obter_catalogo_atributos() {
    // 7 atributos de pressão + 2 atributos de controle financeiro (total = 9)
    return {
        {0, "perna",              false, 0, false},
        {1, "agg_antes_mandante", false, 1, false},
        {2, "sofreu_primeiro",    false, 2, false},
        {3, "min_primeiro_gol",   false, 3, false},
        {4, "vermelhos_mandante", false, 4, false},
        {5, "dif_valor_mi",       false, 5, true},
        {6, "mandante_favorito",  false, 6, true},
        {7, "fase",               true,  0, false},
        {8, "contexto",           true,  1, false}
    };
}

/**
 * @brief Converte uma linha de JogoRow em AmostraArvore sem vazamento de dados (Data Leakage).
 *
 * Importante: home_club_goals, away_club_goals, resultado e classificou NÃO entram nos vetores preditores.
 * Para min_primeiro_gol == -1 (jogos 0x0 sem gols), atribuímos o valor sentinela 121.0
 * (após o fim do jogo), evitando que um corte como "min_primeiro_gol <= 20" agrupe jogos 0x0
 * com partidas que tiveram gol nos minutos iniciais.
 */
static AmostraArvore criar_amostra_base(const JogoRow& j, int rotulo) {
    AmostraArvore a;
    a.game_id = j.game_id;
    a.rotulo = rotulo;

    double min_gol_tratado = (j.min_primeiro_gol < 0)
                                 ? 121.0
                                 : static_cast<double>(j.min_primeiro_gol);

    a.num_vals = {
        static_cast<double>(j.perna),
        static_cast<double>(j.agg_antes_mandante),
        static_cast<double>(j.sofreu_primeiro),
        min_gol_tratado,
        static_cast<double>(j.vermelhos_mandante),
        j.dif_valor_mi,
        static_cast<double>(j.mandante_favorito)
    };

    a.cat_vals = {
        j.fase,
        j.contexto
    };

    return a;
}

std::vector<AmostraArvore> preparar_amostras_resultado(const std::vector<JogoRow>& jogos) {
    std::vector<AmostraArvore> amostras;
    amostras.reserve(jogos.size());
    for (const JogoRow& j : jogos) {
        int rotulo = (j.resultado == "V") ? 1 : 0;
        amostras.push_back(criar_amostra_base(j, rotulo));
    }
    return amostras;
}

std::vector<AmostraArvore> preparar_amostras_classificou(const std::vector<JogoRow>& jogos) {
    std::vector<AmostraArvore> amostras;
    for (const JogoRow& j : jogos) {
        if (j.classificou == 0 || j.classificou == 1) {
            amostras.push_back(criar_amostra_base(j, j.classificou));
        }
    }
    return amostras;
}

void embaralhar_indices_proprio(std::vector<int>& indices, unsigned int semente) {
    // Gerador pseudoaleatório Xorshift32 + Algoritmo de Fisher-Yates implementado do zero
    std::uint32_t estado = (semente == 0) ? 42u : static_cast<std::uint32_t>(semente);
    auto proximo_uint32 = [&estado]() -> std::uint32_t {
        std::uint32_t x = estado;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        estado = x;
        return x;
    };

    int n = static_cast<int>(indices.size());
    for (int i = n - 1; i > 0; --i) {
        int j = static_cast<int>(proximo_uint32() % static_cast<std::uint32_t>(i + 1));
        std::swap(indices[i], indices[j]);
    }
}

double ArvoreCART::calcular_impureza(int n0, int n1) const {
    int total = n0 + n1;
    if (total <= 0) {
        return 0.0;
    }
    double p0 = static_cast<double>(n0) / static_cast<double>(total);
    double p1 = static_cast<double>(n1) / static_cast<double>(total);

    if (config_.criterio == CriterioImpureza::GINI) {
        // Impureza de Gini: G = 1 - p0^2 - p1^2
        return 1.0 - (p0 * p0) - (p1 * p1);
    } else {
        // Entropia de Shannon: H = -p0*log2(p0) - p1*log2(p1)
        double h = 0.0;
        if (p0 > 0.0) h -= p0 * std::log2(p0);
        if (p1 > 0.0) h -= p1 * std::log2(p1);
        return h;
    }
}

void ArvoreCART::treinar(const std::vector<AmostraArvore>& amostras,
                         const ConfigArvore& config,
                         ConjuntoAtributos conj) {
    config_ = config;
    catálogo_atributos_ = obter_catalogo_atributos();
    atributos_ativos_.clear();

    for (const InfoAtributo& info : catálogo_atributos_) {
        if (conj == ConjuntoAtributos::COMPLETO) {
            atributos_ativos_.push_back(info.id_global);
        } else if (conj == ConjuntoAtributos::SO_CONTROLE && info.eh_controle) {
            atributos_ativos_.push_back(info.id_global);
        } else if (conj == ConjuntoAtributos::SO_PRESSAO && !info.eh_controle) {
            atributos_ativos_.push_back(info.id_global);
        }
    }

    n_total_treino_ = static_cast<int>(amostras.size());
    total_nos_ = 0;
    total_folhas_ = 0;
    soma_reducao_impureza_.assign(catálogo_atributos_.size(), 0.0);

    std::vector<int> todos_indices(n_total_treino_);
    for (int i = 0; i < n_total_treino_; ++i) {
        todos_indices[i] = i;
    }

    raiz_ = construir_recursivo(amostras, todos_indices, 0);
}

std::unique_ptr<NoArvore> ArvoreCART::construir_recursivo(const std::vector<AmostraArvore>& amostras,
                                                          const std::vector<int>& indices,
                                                          int profundidade) {
    auto no = std::make_unique<NoArvore>();
    no->id_no = total_nos_++;
    no->profundidade = profundidade;
    no->n_amostras = static_cast<int>(indices.size());

    int n0 = 0;
    int n1 = 0;
    for (int idx : indices) {
        if (amostras[idx].rotulo == 1) {
            n1++;
        } else {
            n0++;
        }
    }

    no->n_classe_0 = n0;
    no->n_classe_1 = n1;
    no->classe_predita = (n1 >= n0) ? 1 : 0;
    no->prob_classe_1 = (no->n_amostras > 0)
                            ? (static_cast<double>(n1) / static_cast<double>(no->n_amostras))
                            : 0.0;
    no->impureza = calcular_impureza(n0, n1);

    // Critérios de parada (pré-poda):
    // 1. Nó puro (impureza == 0)
    // 2. Atingiu a profundidade máxima (max_depth)
    // 3. Não possui pelo menos 2 * min_samples_leaf amostras para dividir em duas folhas válidas
    if (n0 == 0 || n1 == 0 ||
        profundidade >= config_.max_depth ||
        no->n_amostras < 2 * config_.min_samples_leaf) {
        no->eh_folha = true;
        total_folhas_++;
        return no;
    }

    double melhor_ganho = -1.0;
    int melhor_id_attr = -1;
    bool melhor_eh_cat = false;
    double melhor_limiar = 0.0;
    std::string melhor_val_cat;
    std::vector<int> melhores_esq;
    std::vector<int> melhores_dir;

    // Busca exaustiva pelo melhor split binário entre todos os atributos ativos
    for (int id_attr : atributos_ativos_) {
        const InfoAtributo& info = catálogo_atributos_[id_attr];

        if (!info.eh_categorico) {
            // Coleta todos os valores distintos desse atributo numérico no nó atual
            std::vector<double> valores;
            valores.reserve(indices.size());
            for (int idx : indices) {
                valores.push_back(amostras[idx].num_vals[info.indice_vetor]);
            }
            std::sort(valores.begin(), valores.end());
            valores.erase(std::unique(valores.begin(), valores.end()), valores.end());

            if (valores.size() <= 1) {
                continue;
            }

            // Testa cada ponto médio entre valores consecutivos como limiar candidato
            for (std::size_t i = 0; i + 1 < valores.size(); ++i) {
                double limiar = 0.5 * (valores[i] + valores[i + 1]);

                int esq_n0 = 0, esq_n1 = 0;
                int dir_n0 = 0, dir_n1 = 0;
                for (int idx : indices) {
                    double v = amostras[idx].num_vals[info.indice_vetor];
                    int y = amostras[idx].rotulo;
                    if (v <= limiar) {
                        if (y == 1) esq_n1++; else esq_n0++;
                    } else {
                        if (y == 1) dir_n1++; else dir_n0++;
                    }
                }

                int n_esq = esq_n0 + esq_n1;
                int n_dir = dir_n0 + dir_n1;
                if (n_esq < config_.min_samples_leaf || n_dir < config_.min_samples_leaf) {
                    continue;
                }

                double imp_esq = calcular_impureza(esq_n0, esq_n1);
                double imp_dir = calcular_impureza(dir_n0, dir_n1);
                double peso_esq = static_cast<double>(n_esq) / static_cast<double>(no->n_amostras);
                double peso_dir = static_cast<double>(n_dir) / static_cast<double>(no->n_amostras);
                double ganho = no->impureza - (peso_esq * imp_esq + peso_dir * imp_dir);

                if (ganho > melhor_ganho) {
                    melhor_ganho = ganho;
                    melhor_id_attr = id_attr;
                    melhor_eh_cat = false;
                    melhor_limiar = limiar;
                }
            }
        } else {
            // Atributo categórico: testa divisão binária por igualdade (x == cat vs. x != cat)
            std::set<std::string> categorias;
            for (int idx : indices) {
                categorias.insert(amostras[idx].cat_vals[info.indice_vetor]);
            }
            if (categorias.size() <= 1) {
                continue;
            }

            for (const std::string& cat : categorias) {
                int esq_n0 = 0, esq_n1 = 0;
                int dir_n0 = 0, dir_n1 = 0;
                for (int idx : indices) {
                    const std::string& v = amostras[idx].cat_vals[info.indice_vetor];
                    int y = amostras[idx].rotulo;
                    if (v == cat) {
                        if (y == 1) esq_n1++; else esq_n0++;
                    } else {
                        if (y == 1) dir_n1++; else dir_n0++;
                    }
                }

                int n_esq = esq_n0 + esq_n1;
                int n_dir = dir_n0 + dir_n1;
                if (n_esq < config_.min_samples_leaf || n_dir < config_.min_samples_leaf) {
                    continue;
                }

                double imp_esq = calcular_impureza(esq_n0, esq_n1);
                double imp_dir = calcular_impureza(dir_n0, dir_n1);
                double peso_esq = static_cast<double>(n_esq) / static_cast<double>(no->n_amostras);
                double peso_dir = static_cast<double>(n_dir) / static_cast<double>(no->n_amostras);
                double ganho = no->impureza - (peso_esq * imp_esq + peso_dir * imp_dir);

                if (ganho > melhor_ganho) {
                    melhor_ganho = ganho;
                    melhor_id_attr = id_attr;
                    melhor_eh_cat = true;
                    melhor_val_cat = cat;
                }
            }
        }
    }

    // Se nenhum corte válido superar a redução mínima de impureza, vira folha
    if (melhor_id_attr == -1 || melhor_ganho < config_.min_impurity_decrease) {
        no->eh_folha = true;
        total_folhas_++;
        return no;
    }

    // Particiona os índices de acordo com o melhor corte encontrado
    const InfoAtributo& info_escolhido = catálogo_atributos_[melhor_id_attr];
    for (int idx : indices) {
        bool vai_esq = false;
        if (!melhor_eh_cat) {
            vai_esq = (amostras[idx].num_vals[info_escolhido.indice_vetor] <= melhor_limiar);
        } else {
            vai_esq = (amostras[idx].cat_vals[info_escolhido.indice_vetor] == melhor_val_cat);
        }
        if (vai_esq) {
            melhores_esq.push_back(idx);
        } else {
            melhores_dir.push_back(idx);
        }
    }

    no->eh_folha = false;
    no->id_atributo = melhor_id_attr;
    no->nome_atributo = info_escolhido.nome;
    no->eh_categorico = melhor_eh_cat;
    no->limiar_numerico = melhor_limiar;
    no->valor_categorico = melhor_val_cat;

    // Redução de impureza ponderada pela proporção de amostras que chegam a este nó
    double peso_no = (n_total_treino_ > 0)
                         ? (static_cast<double>(no->n_amostras) / static_cast<double>(n_total_treino_))
                         : 0.0;
    no->reducao_impureza_ponderada = peso_no * melhor_ganho;
    soma_reducao_impureza_[melhor_id_attr] += no->reducao_impureza_ponderada;

    no->esq = construir_recursivo(amostras, melhores_esq, profundidade + 1);
    no->dir = construir_recursivo(amostras, melhores_dir, profundidade + 1);

    return no;
}

int ArvoreCART::predizer(const AmostraArvore& amostra) const {
    const NoArvore* atual = raiz_.get();
    while (atual != nullptr && !atual->eh_folha) {
        const InfoAtributo& info = catálogo_atributos_[atual->id_atributo];
        bool vai_esq = false;
        if (!atual->eh_categorico) {
            vai_esq = (amostra.num_vals[info.indice_vetor] <= atual->limiar_numerico);
        } else {
            vai_esq = (amostra.cat_vals[info.indice_vetor] == atual->valor_categorico);
        }
        atual = vai_esq ? atual->esq.get() : atual->dir.get();
    }
    return (atual != nullptr) ? atual->classe_predita : 0;
}

std::vector<double> ArvoreCART::importancia_atributos() const {
    std::vector<double> imp(soma_reducao_impureza_.size(), 0.0);
    double soma_total = 0.0;
    for (double v : soma_reducao_impureza_) {
        soma_total += v;
    }
    if (soma_total > 0.0) {
        for (std::size_t i = 0; i < imp.size(); ++i) {
            imp[i] = soma_reducao_impureza_[i] / soma_total;
        }
    }
    return imp;
}

static void exportar_texto_recursivo(const NoArvore* no,
                                     const std::string& prefixo,
                                     const std::string& nome_c0,
                                     const std::string& nome_c1,
                                     std::ostringstream& oss) {
    if (no == nullptr) {
        return;
    }

    oss << std::fixed << std::setprecision(3);
    if (no->eh_folha) {
        std::string rot = (no->classe_predita == 1) ? nome_c1 : nome_c0;
        oss << prefixo << "--> [FOLHA id=" << no->id_no << "] Predicao: " << rot
            << " | P(" << nome_c1 << ")=" << std::setprecision(1) << (no->prob_classe_1 * 100.0) << "%"
            << " (n=" << no->n_amostras << ": " << no->n_classe_0 << " " << nome_c0
            << ", " << no->n_classe_1 << " " << nome_c1
            << " | imp=" << std::setprecision(3) << no->impureza << ")\n";
        return;
    }

    std::ostringstream regra_esq, regra_dir;
    regra_esq << std::fixed << std::setprecision(2);
    regra_dir << std::fixed << std::setprecision(2);
    if (!no->eh_categorico) {
        regra_esq << no->nome_atributo << " <= " << no->limiar_numerico;
        regra_dir << no->nome_atributo << " >  " << no->limiar_numerico;
    } else {
        regra_esq << no->nome_atributo << " == " << no->valor_categorico;
        regra_dir << no->nome_atributo << " != " << no->valor_categorico;
    }

    oss << prefixo << "[NO id=" << no->id_no << ", prof=" << no->profundidade
        << ", n=" << no->n_amostras << " (" << no->n_classe_0 << "/" << no->n_classe_1 << ")"
        << ", imp=" << std::setprecision(3) << no->impureza << "]\n";

    oss << prefixo << "  |-- Se (" << regra_esq.str() << "):\n";
    exportar_texto_recursivo(no->esq.get(), prefixo + "  |   ", nome_c0, nome_c1, oss);

    oss << prefixo << "  +-- Se (" << regra_dir.str() << "):\n";
    exportar_texto_recursivo(no->dir.get(), prefixo + "      ", nome_c0, nome_c1, oss);
}

std::string ArvoreCART::exportar_texto(const std::string& nome_classe_0,
                                       const std::string& nome_classe_1) const {
    std::ostringstream oss;
    exportar_texto_recursivo(raiz_.get(), "", nome_classe_0, nome_classe_1, oss);
    return oss.str();
}

static void exportar_dot_recursivo(const NoArvore* no,
                                   const std::string& nome_c0,
                                   const std::string& nome_c1,
                                   std::ofstream& arq) {
    if (no == nullptr) {
        return;
    }

    arq << std::fixed;
    if (no->eh_folha) {
        std::string rot = (no->classe_predita == 1) ? nome_c1 : nome_c0;
        std::string cor = (no->classe_predita == 1) ? "#d4edda" : "#f8d7da";
        arq << "    n" << no->id_no << " [shape=box, style=\"filled,rounded\", fillcolor=\"" << cor << "\", "
            << "label=\"Folha: " << rot << "\\n"
            << "P(" << nome_c1 << ") = " << std::setprecision(1) << (no->prob_classe_1 * 100.0) << "%\\n"
            << "n = " << no->n_amostras << " [" << no->n_classe_0 << " / " << no->n_classe_1 << "]\\n"
            << "imp = " << std::setprecision(3) << no->impureza << "\"];\n";
        return;
    }

    std::ostringstream cond;
    cond << std::fixed << std::setprecision(2);
    if (!no->eh_categorico) {
        cond << no->nome_atributo << " <= " << no->limiar_numerico;
    } else {
        cond << no->nome_atributo << " == " << no->valor_categorico;
    }

    arq << "    n" << no->id_no << " [shape=box, style=\"filled\", fillcolor=\"#e2e8f0\", "
        << "label=\"" << cond.str() << "\\n"
        << "imp = " << std::setprecision(3) << no->impureza
        << " | n = " << no->n_amostras << " [" << no->n_classe_0 << " / " << no->n_classe_1 << "]\"];\n";

    if (no->esq) {
        exportar_dot_recursivo(no->esq.get(), nome_c0, nome_c1, arq);
        arq << "    n" << no->id_no << " -> n" << no->esq->id_no << " [label=\"Sim\"];\n";
    }
    if (no->dir) {
        exportar_dot_recursivo(no->dir.get(), nome_c0, nome_c1, arq);
        arq << "    n" << no->id_no << " -> n" << no->dir->id_no << " [label=\"Nao\"];\n";
    }
}

void ArvoreCART::salvar_dot(const std::string& caminho_dot,
                            const std::string& titulo,
                            const std::string& nome_classe_0,
                            const std::string& nome_classe_1) const {
    std::ofstream arq(caminho_dot);
    if (!arq.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo DOT: " + caminho_dot);
    }

    arq << "digraph ArvoreCART {\n";
    arq << "    rankdir=TB;\n";
    arq << "    labelloc=\"t\";\n";
    arq << "    label=\"" << titulo << "\";\n";
    arq << "    fontsize=14;\n";
    arq << "    node [fontname=\"Helvetica\", fontsize=10];\n";
    arq << "    edge [fontname=\"Helvetica\", fontsize=9];\n";

    exportar_dot_recursivo(raiz_.get(), nome_classe_0, nome_classe_1, arq);

    arq << "}\n";
}

/**
 * @brief Preenche média, desvio-padrão, precisão, recall e F1-score em ResultadoValidacaoCruzada.
 */
static void finalizar_metricas_cv(ResultadoValidacaoCruzada& res) {
    double soma = 0.0;
    for (double acc : res.acuracia_por_fold) {
        soma += acc;
    }
    int k = static_cast<int>(res.acuracia_por_fold.size());
    res.acuracia_media = (k > 0) ? (soma / static_cast<double>(k)) : 0.0;

    double soma_quad = 0.0;
    for (double acc : res.acuracia_por_fold) {
        double d = acc - res.acuracia_media;
        soma_quad += d * d;
    }
    res.acuracia_desvio_padrao = (k > 1) ? std::sqrt(soma_quad / static_cast<double>(k - 1)) : 0.0;

    res.precisao = (res.vp + res.fp > 0)
                       ? (static_cast<double>(res.vp) / static_cast<double>(res.vp + res.fp))
                       : 0.0;
    res.recall = (res.vp + res.fn > 0)
                     ? (static_cast<double>(res.vp) / static_cast<double>(res.vp + res.fn))
                     : 0.0;
    res.f1_score = (res.precisao + res.recall > 0.0)
                       ? (2.0 * res.precisao * res.recall / (res.precisao + res.recall))
                       : 0.0;
}

ResultadoValidacaoCruzada validar_kfold_classe_majoritaria(const std::vector<AmostraArvore>& amostras,
                                                           const std::string& nome_alvo,
                                                           int k,
                                                           unsigned int semente) {
    int n = static_cast<int>(amostras.size());
    std::vector<int> ordem(n);
    for (int i = 0; i < n; ++i) ordem[i] = i;
    embaralhar_indices_proprio(ordem, semente);

    ResultadoValidacaoCruzada res;
    res.nome_alvo = nome_alvo;
    res.nome_modelo = "Baseline 1: Classe Majoritaria";
    res.n_amostras = n;
    res.k_folds = k;
    res.vp = res.vn = res.fp = res.fn = 0;

    for (int fold = 0; fold < k; ++fold) {
        int inicio_teste = (fold * n) / k;
        int fim_teste = ((fold + 1) * n) / k;

        int treino_n0 = 0, treino_n1 = 0;
        for (int i = 0; i < n; ++i) {
            if (i >= inicio_teste && i < fim_teste) continue;
            if (amostras[ordem[i]].rotulo == 1) treino_n1++;
            else treino_n0++;
        }

        int classe_maj = (treino_n1 >= treino_n0) ? 1 : 0;

        int acertos = 0;
        int total_teste = fim_teste - inicio_teste;
        for (int i = inicio_teste; i < fim_teste; ++i) {
            int real = amostras[ordem[i]].rotulo;
            int pred = classe_maj;
            if (pred == real) acertos++;
            if (real == 1 && pred == 1) res.vp++;
            else if (real == 0 && pred == 0) res.vn++;
            else if (real == 0 && pred == 1) res.fp++;
            else res.fn++;
        }
        res.acuracia_por_fold.push_back(static_cast<double>(acertos) / static_cast<double>(total_teste));
    }

    finalizar_metricas_cv(res);
    return res;
}

ResultadoValidacaoCruzada validar_kfold_arvore(const std::vector<AmostraArvore>& amostras,
                                               const std::string& nome_alvo,
                                               const std::string& nome_modelo,
                                               const ConfigArvore& config,
                                               ConjuntoAtributos conj,
                                               int k,
                                               unsigned int semente) {
    int n = static_cast<int>(amostras.size());
    std::vector<int> ordem(n);
    for (int i = 0; i < n; ++i) ordem[i] = i;
    embaralhar_indices_proprio(ordem, semente);

    ResultadoValidacaoCruzada res;
    res.nome_alvo = nome_alvo;
    res.nome_modelo = nome_modelo;
    res.n_amostras = n;
    res.k_folds = k;
    res.vp = res.vn = res.fp = res.fn = 0;

    for (int fold = 0; fold < k; ++fold) {
        int inicio_teste = (fold * n) / k;
        int fim_teste = ((fold + 1) * n) / k;

        std::vector<AmostraArvore> treino;
        std::vector<AmostraArvore> teste;
        for (int i = 0; i < n; ++i) {
            if (i >= inicio_teste && i < fim_teste) {
                teste.push_back(amostras[ordem[i]]);
            } else {
                treino.push_back(amostras[ordem[i]]);
            }
        }

        ArvoreCART arv;
        arv.treinar(treino, config, conj);

        int acertos = 0;
        for (const AmostraArvore& am : teste) {
            int real = am.rotulo;
            int pred = arv.predizer(am);
            if (pred == real) acertos++;
            if (real == 1 && pred == 1) res.vp++;
            else if (real == 0 && pred == 0) res.vn++;
            else if (real == 0 && pred == 1) res.fp++;
            else res.fn++;
        }
        res.acuracia_por_fold.push_back(static_cast<double>(acertos) / static_cast<double>(teste.size()));
    }

    finalizar_metricas_cv(res);
    return res;
}

static void imprimir_tabela_cv(const std::string& titulo,
                               const std::vector<ResultadoValidacaoCruzada>& resultados) {
    std::cout << "========================================================================================\n";
    std::cout << "VALIDACAO CRUZADA 5-FOLD (seed=42) - " << titulo << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(36) << "Modelo / Configuracao"
              << std::right
              << std::setw(12) << "Acuracia"
              << std::setw(10) << "Desvio"
              << std::setw(18) << "Matriz(VP/VN/FP/FN)"
              << std::setw(10) << "F1-Score"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (const ResultadoValidacaoCruzada& r : resultados) {
        std::ostringstream acc_s, std_s, mat_s, f1_s;
        acc_s << std::fixed << std::setprecision(1) << (r.acuracia_media * 100.0) << "%";
        std_s << std::fixed << std::setprecision(1) << "+-" << (r.acuracia_desvio_padrao * 100.0) << "%";
        mat_s << r.vp << "/" << r.vn << "/" << r.fp << "/" << r.fn;
        f1_s  << std::fixed << std::setprecision(3) << r.f1_score;

        std::cout << std::left  << std::setw(36) << r.nome_modelo
                  << std::right << std::setw(12) << acc_s.str()
                  << std::setw(10) << std_s.str()
                  << std::setw(18) << mat_s.str()
                  << std::setw(10) << f1_s.str()
                  << "\n";
    }
    std::cout << "========================================================================================\n\n";
}

static void imprimir_importancias(const std::string& titulo,
                                  const ArvoreCART& arv,
                                  const std::vector<InfoAtributo>& catalogo) {
    std::vector<double> imp = arv.importancia_atributos();

    // Ordena os atributos da maior para a menor importância para exibição
    std::vector<int> ordem(catalogo.size());
    for (std::size_t i = 0; i < catalogo.size(); ++i) ordem[i] = static_cast<int>(i);
    std::sort(ordem.begin(), ordem.end(), [&](int a, int b) {
        return imp[a] > imp[b];
    });

    double soma_pressao = 0.0;
    double soma_controle = 0.0;

    std::cout << ">>> Importancia dos Atributos (Reducao de Impureza Ponderada) - " << titulo << ":\n";
    for (int id : ordem) {
        const InfoAtributo& info = catalogo[id];
        if (info.eh_controle) soma_controle += imp[id];
        else soma_pressao += imp[id];

        std::cout << "    - " << std::left << std::setw(20) << info.nome
                  << " [" << std::setw(8) << (info.eh_controle ? "CONTROLE" : "PRESSAO") << "]: "
                  << std::right << std::setw(6) << std::fixed << std::setprecision(2)
                  << (imp[id] * 100.0) << "%\n";
    }
    std::cout << "    --------------------------------------------------------\n";
    std::cout << "    * Soma Fatores de PRESSAO  : " << std::fixed << std::setprecision(2) << (soma_pressao * 100.0) << "%\n";
    std::cout << "    * Soma Fatores de CONTROLE : " << std::fixed << std::setprecision(2) << (soma_controle * 100.0) << "%\n\n";
}

void executar_modulo_arvore(const std::vector<JogoRow>& jogos) {
    const std::vector<InfoAtributo> catalogo = obter_catalogo_atributos();
    const std::vector<AmostraArvore> am_res = preparar_amostras_resultado(jogos);
    const std::vector<AmostraArvore> am_cla = preparar_amostras_classificou(jogos);

    ConfigArvore cfg_gini{CriterioImpureza::GINI, 3, 5, 1e-7};
    ConfigArvore cfg_entr{CriterioImpureza::ENTROPIA, 3, 5, 1e-7};

    // 1. Validação Cruzada 5-Fold para Árvore 1: RESULTADO (Venceu vs. Não Venceu, n = 139)
    std::string alvo_res = "RESULTADO (V vs Nao-V, n=139)";
    std::vector<ResultadoValidacaoCruzada> cv_res = {
        validar_kfold_classe_majoritaria(am_res, alvo_res, 5, 42),
        validar_kfold_arvore(am_res, alvo_res, "Baseline 2: CART So Controle (Gini)", cfg_gini, ConjuntoAtributos::SO_CONTROLE, 5, 42),
        validar_kfold_arvore(am_res, alvo_res, "CART So Pressao (Gini, prof=3)",      cfg_gini, ConjuntoAtributos::SO_PRESSAO,  5, 42),
        validar_kfold_arvore(am_res, alvo_res, "CART Completo (Gini, prof=3)",        cfg_gini, ConjuntoAtributos::COMPLETO,    5, 42),
        validar_kfold_arvore(am_res, alvo_res, "CART Completo (Entropia, prof=3)",    cfg_entr, ConjuntoAtributos::COMPLETO,    5, 42)
    };
    imprimir_tabela_cv(alvo_res, cv_res);

    // 2. Validação Cruzada 5-Fold para Árvore 2: CLASSIFICOU (Classificou vs. Eliminado, n = 75)
    std::string alvo_cla = "CLASSIFICOU (1 vs 0, n=75)";
    std::vector<ResultadoValidacaoCruzada> cv_cla = {
        validar_kfold_classe_majoritaria(am_cla, alvo_cla, 5, 42),
        validar_kfold_arvore(am_cla, alvo_cla, "Baseline 2: CART So Controle (Gini)", cfg_gini, ConjuntoAtributos::SO_CONTROLE, 5, 42),
        validar_kfold_arvore(am_cla, alvo_cla, "CART So Pressao (Gini, prof=3)",      cfg_gini, ConjuntoAtributos::SO_PRESSAO,  5, 42),
        validar_kfold_arvore(am_cla, alvo_cla, "CART Completo (Gini, prof=3)",        cfg_gini, ConjuntoAtributos::COMPLETO,    5, 42),
        validar_kfold_arvore(am_cla, alvo_cla, "CART Completo (Entropia, prof=3)",    cfg_entr, ConjuntoAtributos::COMPLETO,    5, 42)
    };
    imprimir_tabela_cv(alvo_cla, cv_cla);

    // 3. Treino das Árvores Finais no Conjunto Completo para Interpretação, Importâncias e Exportação DOT
    ArvoreCART arv_res_final;
    arv_res_final.treinar(am_res, cfg_gini, ConjuntoAtributos::COMPLETO);

    ArvoreCART arv_cla_final;
    arv_cla_final.treinar(am_cla, cfg_gini, ConjuntoAtributos::COMPLETO);

    std::cout << "========================================================================================\n";
    std::cout << "ARVORE 1 TREINADA (RESULTADO: VENCEU vs. NAO VENCEU - 139 JOGOS, GINI, max_depth=3)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::string txt_res = arv_res_final.exportar_texto("NAO_VENCEU", "VENCEU");
    std::cout << txt_res << "\n";
    imprimir_importancias("Arvore 1 (Resultado - 139 jogos)", arv_res_final, catalogo);

    std::cout << "========================================================================================\n";
    std::cout << "ARVORE 2 TREINADA (CLASSIFICOU: CLASSIFICADO vs. ELIMINADO - 75 JOGOS, GINI, max_depth=3)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::string txt_cla = arv_cla_final.exportar_texto("ELIMINADO", "CLASSIFICADO");
    std::cout << txt_cla << "\n";
    imprimir_importancias("Arvore 2 (Classificou - 75 jogos)", arv_cla_final, catalogo);
    std::cout << "========================================================================================\n\n";

    // 4. Salvar arquivos DOT, TXT e CSVs em results/
    arv_res_final.salvar_dot("results/arvore_resultado.dot",
                             "Arvore CART - Resultado do Mandante (139 jogos, Gini, max_depth=3)",
                             "NAO_VENCEU", "VENCEU");
    arv_cla_final.salvar_dot("results/arvore_classificou.dot",
                             "Arvore CART - Classificacao do Mandante (75 jogos, Gini, max_depth=3)",
                             "ELIMINADO", "CLASSIFICADO");

    {
        std::ofstream f_txt_res("results/arvore_resultado.txt");
        f_txt_res << txt_res;
        std::ofstream f_txt_cla("results/arvore_classificou.txt");
        f_txt_cla << txt_cla;
    }

    // Salva CSV de Validação Cruzada
    {
        std::ofstream arq("results/arvore_cv_comparacao.csv");
        arq << "alvo,modelo,n_amostras,k_folds,acuracia_media,acuracia_desvio,"
            << "vp,vn,fp,fn,precisao,recall,f1_score,fold1,fold2,fold3,fold4,fold5\n";
        arq << std::fixed << std::setprecision(6);

        auto gravar_lista = [&](const std::vector<ResultadoValidacaoCruzada>& lista) {
            for (const ResultadoValidacaoCruzada& r : lista) {
                arq << "\"" << r.nome_alvo << "\","
                    << "\"" << r.nome_modelo << "\","
                    << r.n_amostras << ","
                    << r.k_folds << ","
                    << r.acuracia_media << ","
                    << r.acuracia_desvio_padrao << ","
                    << r.vp << "," << r.vn << "," << r.fp << "," << r.fn << ","
                    << r.precisao << "," << r.recall << "," << r.f1_score;
                for (double f : r.acuracia_por_fold) {
                    arq << "," << f;
                }
                arq << "\n";
            }
        };
        gravar_lista(cv_res);
        gravar_lista(cv_cla);
    }

    // Salva CSV de Importância dos Atributos
    {
        std::ofstream arq("results/arvore_importancia_atributos.csv");
        arq << "atributo,grupo,eh_categorico,importancia_resultado,importancia_classificou\n";
        arq << std::fixed << std::setprecision(6);

        std::vector<double> imp_r = arv_res_final.importancia_atributos();
        std::vector<double> imp_c = arv_cla_final.importancia_atributos();

        for (std::size_t i = 0; i < catalogo.size(); ++i) {
            const InfoAtributo& info = catalogo[i];
            arq << info.nome << ","
                << (info.eh_controle ? "CONTROLE" : "PRESSAO") << ","
                << (info.eh_categorico ? 1 : 0) << ","
                << imp_r[i] << ","
                << imp_c[i] << "\n";
        }
    }
}
