<h1 style="border : none">Considerações</h1>
<h3>Aqui constão algumas pontuações e análises que consideramos importantes na confecção do trabalho.</h3>

O problema das rotas consiste em que achemos o melhor caminho partindo de um ponto 1 à outros passando pelos 24 ginásios e finalmente chegando em um ponto U 

Poderiamos analisar o problema ainda no escopo das matrizes e tentar determinar qual o melhor caminho indo de 1 a U passando pelos 24 ginásios , mas com isso teriamos de trabalhar com todas as casas e a solução para isso não seria exatamente
a mais simples ou mais intuitiva possível.

Por outro lado , é muito interessante retrabalhar o problema da seguinte maneira : Temos de maneira fixada os 24 ginásios , o ponto de partida e o ponto de chegada . Nesse sentido , podemos reduzir o problema para um grafo fortemente conexo
em que cada vértice é capaz de avançar para outro qualquer em uma aresta que custo igual ao menor caminho do lugar X até o lugar Y . (UMA IMAGEM MERAMENTE ILUSTRATIVA PARA UM GRAFO COM 5 NÓDULOS)

![IMAGEM](TSP.png)

Nesse sentido , é muito interessante abordar esse problema de maneira parecida com o problema do caixeiro viajante
que tem um cenário inicial idêntico , em contrapartida , tem um objetivo final diferente . ao invés de tratarmos de um ciclo ao final e de pontos de partidas e chegadas diferentes , temo um ponto de partida e um de entrada fixa e temo que olhar para 
o caminho que pssa por todos os nodulos tal que resolva os valores o custo de maneira ótima.

A ideia é usar uma heurística para abordar a "montagem" dos caminhos de maneira a se aproximar do ótimo de maneira análoga ao problema do caixeiro viajante. Então usaremos o A* nesse grafo reduzido e não no grafo da matriz completa


## Modelagem implementada

- **Grafo reduzido.** Rodamos Dijkstra (4 direções , custo = soma dos custos das células em que se entra) a partir de cada um dos 26 pontos de interesse. A matriz `d[i][j]` guarda o menor custo entre qualquer par.
- **Estado.** (último ginásio visitado , conjunto de ginásios ainda não visitados) , o que distingue as situações em que conjuntos diferentes já foram visitados.
- **Ação / custo.** Ir até um ginásio não visitado , com custo `d[atual][destino]`. A origem custa 1 e a chegada em U também.
- **Heurística.** $h(atual , R)$ = custo da árvore geradora mínima (MST) sobre $\{atual\} \cup R \cup \{U\}$ com as distâncias de `d`. Qualquer caminho que parte de `atual` , passa por todos de R e termina em U contém uma árvore geradora desses nós , então $h$ é **admissível**. Ela também é **consistente** (adicionar a aresta atual->v à MST do estado filho dá uma árvore do estado pai , logo $h(u) \le d(u,v) + h(v)$) , então um estado fechado nunca precisa ser reaberto e o A* devolve o ótimo.
- **Engenharia.** O espaço tem até $2^{24} \cdot 24 \approx 4 \cdot 10^8$ estados. O programa guarda `g` em um vetor de 16 bits indexado por (máscara , atual) , usa uma fila por baldes (o `f` é inteiro) e avalia a MST de forma preguiçosa (um filho entra na fila com o `f` do pai , que é um limite inferior pela consistência).
- **Verificação.** A opção `--verificar` roda Held-Karp (DP sobre subconjuntos) e compara com o custo do A*.

Uma primeira versão com a heurística "menor aresta de entrada + MST dos restantes + menor aresta de saída" e `unordered_map` não terminava em tempo útil com 24 ginásios (mais de 5 GB de RAM em poucos minutos).

