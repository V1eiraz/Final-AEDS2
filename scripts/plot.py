#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
@file plot.py
@brief Gerador de gráficos vetoriais (PDF e PNG) para o Artigo IEEE de AEDS2.

Lê exclusivamente os arquivos CSV e DOT gerados pelo executável C++ em results/
e produz as figuras científicas em figs/:
  1. figs/escalabilidade_loglog.pdf     - Tempo x Tamanho (escala log-log: Grafo e Árvore CART)
  2. figs/resiliencia_comparacao.pdf    - Índice de Resiliência por estado (COM vs. SEM torcida)
  3. figs/importancia_atributos.pdf     - Importância dos Atributos (Pressão vs. Controle Financeiro)
  4. figs/acuracia_kfold_baselines.pdf  - Validação Cruzada 5-Fold vs. Baselines
"""

import csv
import os
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# Configuração visual padrão compatível com artigos IEEE Conference
plt.rcParams.update({
    "font.family": "sans-serif",
    "font.size": 10,
    "axes.titlesize": 11,
    "axes.labelsize": 10,
    "xtick.labelsize": 9,
    "ytick.labelsize": 9,
    "legend.fontsize": 8.5,
    "figure.titlesize": 12,
})


def plotar_escalabilidade(caminho_csv, pasta_figs):
    """Gera o gráfico log-log de Tempo de Execução vs. Tamanho da Entrada."""
    dados_grafo = defaultdict(list)
    dados_arvore = []

    with open(caminho_csv, "r", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            tam = int(row["tamanho"])
            t_med = float(row["tempo_medio_ms"])
            t_std = float(row["tempo_desvio_ms"])
            if row["categoria"] == "GRAFO_DAG":
                dados_grafo[row["algoritmo"]].append((tam, t_med, t_std))
            else:
                dados_arvore.append((tam, t_med, t_std))

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.5, 4.6))

    estilos = {
        "Dijkstra_MinHeap": ("#d62728", "o-", "Dijkstra Min-Heap O((V+E) log V)"),
        "Kahn_Toposort":    ("#1f77b4", "s-", "Kahn Toposort O(V+E)"),
        "DP_Resiliencia":   ("#2ca02c", "^-", "DP Resiliência O(V+E)"),
        "DAG_CaminhoMin":   ("#9467bd", "d--", "Caminho Mín. DAG O(V+E)"),
        "BFS_Alcance":      ("#ff7f0e", "v:", "BFS Alcançabilidade O(V+E)"),
        "DFS_Alcance":      ("#8c564b", "x-.", "DFS Alcançabilidade O(V+E)"),
    }

    for alg, pontos in dados_grafo.items():
        pontos.sort(key=lambda x: x[0])
        xs = [p[0] for p in pontos]
        ys = [max(p[1], 1e-4) for p in pontos]
        cor, fmt, rotulo = estilos.get(alg, ("#333333", "o-", alg))
        ax1.plot(xs, ys, fmt, color=cor, label=rotulo, linewidth=1.8, markersize=5.5)

    # Linha guia teórica linear O(V+E)
    if "DP_Resiliencia" in dados_grafo:
        p0 = dados_grafo["DP_Resiliencia"][0]
        xs_ref = np.array([100, 1000, 10000, 100000], dtype=float)
        ys_ref = p0[1] * (xs_ref / p0[0])
        ax1.plot(xs_ref, ys_ref, "k--", alpha=0.45, linewidth=1.2, label="Referência Linear O(V)")

    ax1.set_xscale("log")
    ax1.set_yscale("log")
    ax1.set_xlabel("Número de Vértices V (com E ≈ 3V) [escala log]")
    ax1.set_ylabel("Tempo Médio de Execução (ms) [escala log]")
    ax1.set_title("(a) Escalabilidade dos Algoritmos em Grafo (DAG)")
    ax1.grid(True, which="both", linestyle="--", alpha=0.35)
    ax1.legend(loc="upper left", framealpha=0.9)

    # Painel (b): Árvore de Decisão CART
    dados_arvore.sort(key=lambda x: x[0])
    xs_a = [p[0] for p in dados_arvore]
    ys_a = [max(p[1], 1e-4) for p in dados_arvore]
    ax2.plot(xs_a, ys_a, "o-", color="#1b9e77", linewidth=2.0, markersize=6.5,
             label="Treino CART (Gini, prof=3, M=9)")

    if dados_arvore:
        n0, t0, _ = dados_arvore[0]
        xs_t = np.array(xs_a, dtype=float)
        ys_t = t0 * (xs_t * np.log2(xs_t)) / (n0 * np.log2(n0))
        ax2.plot(xs_t, ys_t, "k--", alpha=0.5, linewidth=1.3, label="Referência Teórica O(N log N)")

    ax2.set_xscale("log")
    ax2.set_yscale("log")
    ax2.set_xlabel("Número de Amostras Sintéticas N [escala log]")
    ax2.set_ylabel("Tempo Médio de Treinamento (ms) [escala log]")
    ax2.set_title("(b) Escalabilidade do Treinamento da Árvore CART")
    ax2.grid(True, which="both", linestyle="--", alpha=0.35)
    ax2.legend(loc="upper left", framealpha=0.9)

    plt.tight_layout()
    fig.savefig(os.path.join(pasta_figs, "escalabilidade_loglog.pdf"), bbox_inches="tight")
    fig.savefig(os.path.join(pasta_figs, "escalabilidade_loglog.png"), dpi=300, bbox_inches="tight")
    plt.close(fig)


def plotar_resiliencia(caminho_csv, pasta_figs):
    """Gera o gráfico comparativo do Índice de Resiliência por janela de tempo e saldo."""
    linhas = []
    with open(caminho_csv, "r", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            linhas.append(row)

    # Seleciona estados críticos da volta (S = -2, S = -1, S = 0, S = +1) ao longo de T0..T7
    saldos_foco = [-2, -1, 0, 1]
    titulos_saldo = {
        -2: "Saldo S = -2 (Perdendo por 2 gols)",
        -1: "Saldo S = -1 (Perdendo por 1 gol)",
         0: "Saldo S = 0 (Confronto Empatado)",
         1: "Saldo S = +1 (Vencendo por 1 gol)",
    }

    fig, axes = plt.subplots(2, 2, figsize=(11.5, 7.5), sharey=True)
    axes_flat = axes.flatten()

    janelas_labels = ["T0\n(0')", "T1\n(1-14')", "T2\n(15-29')", "T3\n(30-44')",
                      "T4\n(45-59')", "T5\n(60-74')", "T6\n(75-89')", "T7\n(90'+)"]

    for idx, s in enumerate(saldos_foco):
        ax = axes_flat[idx]
        ts_com, ps_com, ns_com = [], [], []
        ts_sem, ps_sem, ns_sem = [], [], []

        for r in linhas:
            if int(r["saldo"]) != s:
                continue
            t = int(r["janela"])
            n_c = int(r["n_com_torcida"])
            p_c = float(r["prob_com_torcida"])
            n_s = int(r["n_sem_torcida"])
            p_s = float(r["prob_sem_torcida"])

            if n_c > 0 and p_c >= 0.0:
                ts_com.append(t)
                ps_com.append(p_c * 100.0)
                ns_com.append(n_c)
            if n_s > 0 and p_s >= 0.0:
                ts_sem.append(t)
                ps_sem.append(p_s * 100.0)
                ns_sem.append(n_s)

        ax.plot(ts_com, ps_com, "o-", color="#1f77b4", linewidth=2.0, markersize=6,
                label="COM_TORCIDA")
        ax.plot(ts_sem, ps_sem, "s--", color="#d62728", linewidth=2.0, markersize=6,
                label="SEM_TORCIDA")

        # Anota o tamanho amostral (n) junto a cada ponto
        for t, p, n in zip(ts_com, ps_com, ns_com):
            ax.annotate(f"n={n}", (t, p), textcoords="offset points", xytext=(0, 6),
                        ha="center", fontsize=7.5, color="#1f77b4")
        for t, p, n in zip(ts_sem, ps_sem, ns_sem):
            ax.annotate(f"n={n}", (t, p), textcoords="offset points", xytext=(0, -11),
                        ha="center", fontsize=7.5, color="#d62728")

        ax.set_title(titulos_saldo[s])
        ax.set_xticks(range(8))
        ax.set_xticklabels(janelas_labels, fontsize=8)
        ax.set_ylim(-10, 112)
        ax.grid(True, linestyle="--", alpha=0.35)
        if idx in (0, 2):
            ax.set_ylabel("Índice de Resiliência P(Classificar) [%]")
        ax.legend(loc="best")

    plt.suptitle("Índice de Resiliência na Perna 2 (Volta) por Janela Temporal e Saldo Agregado", y=1.01)
    plt.tight_layout()
    fig.savefig(os.path.join(pasta_figs, "resiliencia_comparacao.pdf"), bbox_inches="tight")
    fig.savefig(os.path.join(pasta_figs, "resiliencia_comparacao.png"), dpi=300, bbox_inches="tight")
    plt.close(fig)


def plotar_importancia_e_cv(caminho_imp_csv, caminho_cv_csv, pasta_figs):
    """Gera os gráficos de Importância dos Atributos e de Validação Cruzada 5-Fold."""
    atributos = []
    grupos = []
    imp_res = []
    imp_cla = []

    with open(caminho_imp_csv, "r", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            ir = float(row["importancia_resultado"]) * 100.0
            ic = float(row["importancia_classificou"]) * 100.0
            if ir > 0.0 or ic > 0.0:
                atributos.append(row["atributo"])
                grupos.append(row["grupo"])
                imp_res.append(ir)
                imp_cla.append(ic)

    x = np.arange(len(atributos))
    largura = 0.36

    fig, ax = plt.subplots(figsize=(8.8, 4.5))
    b1 = ax.bar(x - largura / 2, imp_res, largura, label="Árvore 1: Resultado (V vs. Não-V, n=139)",
                color="#2b5c8f", edgecolor="black", linewidth=0.6)
    b2 = ax.bar(x + largura / 2, imp_cla, largura, label="Árvore 2: Classificou (1 vs. 0, n=75)",
                color="#d95f02", edgecolor="black", linewidth=0.6)

    labels_x = [f"{a}\n[{g}]" for a, g in zip(atributos, grupos)]
    ax.set_xticks(x)
    ax.set_xticklabels(labels_x)
    ax.set_ylabel("Importância Relativa na Redução de Gini (%)")
    ax.set_title("Importância dos Atributos na Árvore CART: Fatores de Pressão vs. Controle Financeiro")
    ax.set_ylim(0, 68)
    ax.grid(axis="y", linestyle="--", alpha=0.35)
    ax.legend()

    for barras in (b1, b2):
        for b in barras:
            h = b.get_height()
            if h > 0.1:
                ax.annotate(f"{h:.1f}%",
                            xy=(b.get_x() + b.get_width() / 2, h),
                            xytext=(0, 3), textcoords="offset points",
                            ha="center", va="bottom", fontsize=8)

    plt.tight_layout()
    fig.savefig(os.path.join(pasta_figs, "importancia_atributos.pdf"), bbox_inches="tight")
    fig.savefig(os.path.join(pasta_figs, "importancia_atributos.png"), dpi=300, bbox_inches="tight")
    plt.close(fig)

    # Gráfico de Acurácia em Validação Cruzada 5-Fold vs. Baselines
    modelos_curtos = [
        "Baseline 1:\nClasse Majoritária",
        "Baseline 2:\nCART Só Controle",
        "CART Só Pressão\n(Gini, prof=3)",
        "CART Completo\n(Gini, prof=3)",
    ]
    acc_res, std_res = [], []
    acc_cla, std_cla = [], []

    with open(caminho_cv_csv, "r", encoding="utf-8") as f:
        linhas_cv = list(csv.DictReader(f))

    for linha in linhas_cv[:4]:
        acc_res.append(float(linha["acuracia_media"]) * 100.0)
        std_res.append(float(linha["acuracia_desvio"]) * 100.0)
    for linha in linhas_cv[5:9]:
        acc_cla.append(float(linha["acuracia_media"]) * 100.0)
        std_cla.append(float(linha["acuracia_desvio"]) * 100.0)

    x_cv = np.arange(len(modelos_curtos))
    fig2, ax_cv = plt.subplots(figsize=(8.8, 4.4))
    c1 = ax_cv.bar(x_cv - largura / 2, acc_res, largura, yerr=std_res, capsize=4,
                   label="Resultado no Jogo (n=139)", color="#3182bd", edgecolor="black", linewidth=0.6)
    c2 = ax_cv.bar(x_cv + largura / 2, acc_cla, largura, yerr=std_cla, capsize=4,
                   label="Classificação no Confronto (n=75)", color="#e6550d", edgecolor="black", linewidth=0.6)

    ax_cv.set_xticks(x_cv)
    ax_cv.set_xticklabels(modelos_curtos)
    ax_cv.set_ylabel("Acurácia Média em 5-Fold CV (%)")
    ax_cv.set_title("Desempenho Preditivo da Árvore CART (5-Fold CV) vs. Baselines Obrigatórios")
    ax_cv.set_ylim(0, 105)
    ax_cv.grid(axis="y", linestyle="--", alpha=0.35)
    ax_cv.legend(loc="upper left")

    for barras in (c1, c2):
        for b in barras:
            h = b.get_height()
            ax_cv.annotate(f"{h:.1f}%",
                           xy=(b.get_x() + b.get_width() / 2, h + 1.5),
                           xytext=(0, 3), textcoords="offset points",
                           ha="center", va="bottom", fontsize=8, fontweight="bold")

    plt.tight_layout()
    fig2.savefig(os.path.join(pasta_figs, "acuracia_kfold_baselines.pdf"), bbox_inches="tight")
    fig2.savefig(os.path.join(pasta_figs, "acuracia_kfold_baselines.png"), dpi=300, bbox_inches="tight")
    plt.close(fig2)


def plotar_arvore_dot(caminho_dot, nome_saida, pasta_figs):
    """Renderiza o arquivo .dot da Árvore CART em PDF/PNG vetorial usando matplotlib."""
    import re
    nos = {}
    filhos = defaultdict(list)
    titulo = "Árvore de Decisão CART"

    with open(caminho_dot, "r", encoding="utf-8") as f:
        for linha in f:
            linha = linha.strip()
            m_tit = re.match(r'label="([^"]+)";', linha)
            if m_tit:
                titulo = m_tit.group(1)
                continue
            m_no = re.match(r'n(\d+)\s+\[.*fillcolor="([^"]+)".*label="([^"]+)"\];', linha)
            if m_no:
                nid = int(m_no.group(1))
                cor = m_no.group(2)
                lbl = m_no.group(3).replace("\\n", "\n")
                nos[nid] = (lbl, cor)
                continue
            m_ar = re.match(r'n(\d+)\s+->\s+n(\d+)\s+\[label="([^"]+)"\];', linha)
            if m_ar:
                u, v, elbl = int(m_ar.group(1)), int(m_ar.group(2)), m_ar.group(3)
                filhos[u].append((v, elbl))

    if not nos:
        return

    # Calcula posições (x, y) por travessia em ordem das folhas
    pos = {}
    folha_x = [0]

    def calc_pos(u, prof):
        if u not in filhos or not filhos[u]:
            x = float(folha_x[0])
            folha_x[0] += 1
            pos[u] = (x, -prof)
            return x
        xs = [calc_pos(v, prof + 1) for v, _ in filhos[u]]
        x = sum(xs) / len(xs)
        pos[u] = (x, -prof)
        return x

    calc_pos(0, 0)

    fig, ax = plt.subplots(figsize=(12.5, 6.2))
    for u, lista_v in filhos.items():
        x1, y1 = pos[u]
        for v, elbl in lista_v:
            x2, y2 = pos[v]
            ax.annotate("", xy=(x2, y2 + 0.18), xytext=(x1, y1 - 0.18),
                        arrowprops=dict(arrowstyle="->", color="#444444", lw=1.4))
            mx, my = 0.5 * (x1 + x2), 0.5 * (y1 + y2)
            ax.text(mx, my, elbl, fontsize=8.5, fontweight="bold",
                    ha="center", va="center",
                    bbox=dict(boxstyle="round,pad=0.15", fc="white", ec="none", alpha=0.85))

    for nid, (lbl, cor) in nos.items():
        x, y = pos[nid]
        ax.text(x, y, lbl, ha="center", va="center", fontsize=8.2,
                bbox=dict(boxstyle="round,pad=0.45", fc=cor, ec="#333333", lw=1.1))

    xs_all = [p[0] for p in pos.values()]
    ys_all = [p[1] for p in pos.values()]
    ax.set_xlim(min(xs_all) - 0.7, max(xs_all) + 0.7)
    ax.set_ylim(min(ys_all) - 0.55, max(ys_all) + 0.45)
    ax.axis("off")
    ax.set_title(titulo, fontsize=11.5, fontweight="bold", pad=12)

    plt.tight_layout()
    fig.savefig(os.path.join(pasta_figs, f"{nome_saida}.pdf"), bbox_inches="tight")
    fig.savefig(os.path.join(pasta_figs, f"{nome_saida}.png"), dpi=300, bbox_inches="tight")
    plt.close(fig)


def main():
    pasta_results = "results"
    pasta_figs = "figs"
    os.makedirs(pasta_figs, exist_ok=True)

    plotar_escalabilidade(os.path.join(pasta_results, "escalabilidade.csv"), pasta_figs)
    plotar_resiliencia(os.path.join(pasta_results, "dp_comparacao_volta.csv"), pasta_figs)
    plotar_importancia_e_cv(
        os.path.join(pasta_results, "arvore_importancia_atributos.csv"),
        os.path.join(pasta_results, "arvore_cv_comparacao.csv"),
        pasta_figs,
    )
    plotar_arvore_dot(os.path.join(pasta_results, "arvore_resultado.dot"), "arvore_resultado", pasta_figs)
    plotar_arvore_dot(os.path.join(pasta_results, "arvore_classificou.dot"), "arvore_classificou", pasta_figs)
    print(">>> Figuras vetoriais (PDF) e PNG geradas com sucesso em figs/:")
    for nome in sorted(os.listdir(pasta_figs)):
        print("    - figs/" + nome)


if __name__ == "__main__":
    main()
