# Trabalho Final AEDS2 — Grafos de Estados (DAG) e Árvore de Decisão CART em C++17

**Tema:** *Situações de Pressão Psicológica e Desempenho de Equipes Mandantes em Mata-Matas da UEFA Champions League (2018/19–2022/23)*  
**Disciplina:** Estruturas em Árvores e Teoria dos Grafos (AEDS2) — CEFET-MG, Campus V (Divinópolis-MG)  
**Autores:** Davi Enzo Vieira · Felipe Côrtes do Nascimento  
**Orientador:** Prof. Michel Pires da Silva  
**Base de Dados (Zenodo):** https://zenodo.org/records/23772487

---

## 1. Visão Geral

Este repositório contém a implementação completa, **em C++17 puro (apenas STL, sem bibliotecas externas de grafos ou aprendizado de máquina)**, de duas modelagens complementares aplicadas a **139 partidas eliminatórias** da UEFA Champions League (temporadas 2018/19 a 2022/23):

- **Módulo A — Grafo Acíclico Direcionado (DAG) de Estados e Colapso:**
  - Lista de adjacência ponderada (`COM_TORCIDA` vs. `SEM_TORCIDA`, e subgrafos por perna) sobre 984 transições de 123 jogos com mando efetivo;
  - Ordenação Topológica de **Kahn** com detecção de ciclos ($O(V + E)$);
  - **Programação Dinâmica (DP)** em ordem topológica reversa para o Índice de Resiliência $P(\text{CLASSIFICADO} \mid u)$ ($O(V + E)$);
  - **Caminho Mais Provável até a Virada** com peso $w = -\ln p$, comparando **Dijkstra com Min-Heap** ($O((V + E)\log V)$) e **Relaxamento em Ordem Topológica** ($O(V + E)$);
  - **Alcançabilidade de Virada** via **BFS** e **DFS** (com pilha explícita para $V > 5\,000$);
  - **Métricas de Colapso** (probabilidade de sofrer gols consecutivos e em $\le 15$ min) a partir dos 397 gols de `data/eventos_ko.csv`.
- **Módulo B — Árvore de Decisão CART do Zero:**
  - Critérios de **Gini** e **Entropia de Shannon**, cortes binários numéricos (*Sort + Linear Sweep* em $O(N \log N)$) e categóricos;
  - Pré-poda (`max_depth = 3`, `min_samples_leaf = 5`), validação cruzada **5-fold** (embaralhamento Fisher-Yates + Xorshift32 próprio com `seed = 42`), matriz de confusão, F1-score, comparação contra **Baselines** (Classe Majoritária e CART Só Controle Financeiro), importância dos atributos e exportação em texto e formato `.dot` (Graphviz).
- **Escalabilidade Assintótica:**
  - Geração de DAGs sintéticos ($V \in \{10^2, 10^3, 10^4, 10^5\}$, $E \approx 3V$) e conjuntos sintéticos para a árvore CART ($N \in \{10^2, 10^3, 10^4\}$), gravando tempos médios (5 repetições) em `results/escalabilidade.csv`.

---

## 2. Ambiente e Pré-requisitos (Ubuntu 24.04)

- **Compilação e execução principal (obrigatório):** `g++` (C++17) e `make`.
  ```bash
  sudo apt update && sudo apt install -y build-essential
  ```
- **Geração de gráficos (opcional, apenas para `scripts/plot.py`):** Python 3 com `matplotlib` e `numpy`.
  ```bash
  sudo apt install -y python3-matplotlib python3-numpy
  ```

---

## 3. Como Compilar e Executar no Linux (Ubuntu 24.04)

A partir da raiz do repositório:

```bash
make && ./build/app
```

O executável compila **sem warnings** com `g++ -Wall -Wextra -Werror` e, sem argumentos, executa **todos os módulos** (`grafo`, `arvore` e `escala`), salvando automaticamente todos os arquivos CSV e DOT em `results/`.

### Modos de execução opcionais

```bash
./build/app grafo    # Executa apenas o Módulo A (DAG, Kahn, DP, Dijkstra/DAG, BFS/DFS e Colapso)
./build/app arvore   # Executa apenas o Módulo B (Árvore CART, 5-Fold CV, Baselines, Importâncias e DOT)
./build/app escala   # Executa apenas o Benchmark de Escalabilidade Sintética
```

### Outros alvos do `Makefile`

```bash
make clean           # Remove objetos e binários de ./build
make release         # Compila com otimização -O3
make debug           # Compila com símbolos de depuração (-DDEBUG -g)
make run             # Executa ./build/app
```

### Gerar as figuras vetoriais (PDF e PNG)

Após rodar `./build/app`, execute:

```bash
python3 scripts/plot.py
```

As figuras vetoriais (`.pdf`) e rasterizadas (`.png`, 300 DPI) são salvas em `figs/`.

---

## 4. Estrutura do Repositório

```text
.
├── Makefile                    # Compilação C++17 com -Wall -Wextra -Werror
├── README.md                   # Este arquivo
├── data/                       # Base de dados tratada (publicada no Zenodo)
│   ├── jogos_ko.csv            # 139 partidas eliminatórias da UCL (25 colunas)
│   ├── eventos_ko.csv          # 1.962 eventos (gols, cartões e substituições)
│   └── transicoes.csv          # 984 transições observadas do DAG (123 jogos)
├── include/                    # Cabeçalhos C++ (.hpp)
│   ├── csv.hpp                 # Leitor CSV com tratamento de CRLF (\r\n)
│   ├── grafo.hpp               # Estrutura Grafo (lista de adjacência ponderada)
│   ├── toposort.hpp            # Ordenação topológica de Kahn + detecção de ciclo
│   ├── dp.hpp                  # Programação Dinâmica reversa (Índice de Resiliência)
│   ├── caminho.hpp             # Caminho mais provável (-ln p): Dijkstra vs. DAG
│   ├── alcancabilidade.hpp     # BFS e DFS de alcançabilidade de viradas
│   ├── metricas.hpp            # Métricas de colapso psicológico (gols consecutivos)
│   ├── arvore.hpp              # Árvore de Decisão CART, 5-fold CV e exportação DOT
│   ├── sintetico.hpp           # Geradores sintéticos e benchmark de escalabilidade
│   └── main.hpp                # Cabeçalho agregador
├── src/                        # Implementações C++ (.cpp) correspondentes + main.cpp
├── results/                    # Saídas CSV, TXT e DOT geradas pelo programa
├── scripts/
│   └── plot.py                 # Geração dos gráficos vetoriais a partir de results/
└── figs/                       # Figuras vetoriais em PDF/PNG geradas por plot.py
```

---

## 5. Saídas Geradas em `results/`

| Arquivo | Conteúdo |
|---|---|
| `results/dp_resiliencia.csv` | Índice de Resiliência $P(\text{CLASSIFICADO} \mid u)$ e suporte $n$ para todos os estados |
| `results/dp_comparacao_volta.csv` | Comparação lado a lado `COM_TORCIDA` vs. `SEM_TORCIDA` na Perna 2 (Volta) |
| `results/caminhos_virada.csv` | Caminhos mais prováveis ($w = -\ln p$), probabilidades e tempos (Dijkstra vs. DAG) |
| `results/alcancabilidade_virada.csv` | Alcançabilidade (BFS/DFS) nos 24 estados de desvantagem ($P2$, $S < 0$) |
| `results/metricas_colapso.csv` | Taxas de colapso consecutivo (geral e $\le 15$ min), Mandante vs. Visitante por contexto |
| `results/arvore_cv_comparacao.csv` | Métricas de validação cruzada 5-fold (`Resultado` e `Classificou`) frente aos Baselines |
| `results/arvore_importancia_atributos.csv` | Importância relativa dos 9 atributos (Pressão vs. Controle Financeiro) |
| `results/arvore_resultado.dot` / `.txt` | Árvore CART treinada para `Resultado` ($n = 139$) em Graphviz DOT e texto |
| `results/arvore_classificou.dot` / `.txt` | Árvore CART treinada para `Classificou` ($n = 75$) em Graphviz DOT e texto |
| `results/escalabilidade.csv` | Tempos médios e desvios (5 repetições) em DAGs ($10^2$–$10^5$) e CART ($10^2$–$10^4$) |

---

## 6. Nota Metodológica

O conjunto de dados é observacional e de tamanho reduzido (139 partidas no total; 123 no DAG). Todos os achados reportados descrevem **associações empíricas** (*"está associado a"*) entre proxies de pressão situacional e desempenho esportivo, sem inferência causal.
