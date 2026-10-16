<h1 style="border : none">Trabalho 1 - INF1771</h1>
<h3 style="">Trabalho feito por : </h3>

<ul>
    <li>Carlos Frederico Stern Sant'ana - 2511920</li>
    <li>Luis Felipe Gadelha - 2210308</li>
    <li>Felipe Peixoto - 2211750 </li>
</ul>

<h2 style="border : none">A organização : </h2>
<ul>
    <li>
        Pokemons.md
        <h4>Descreve nossa abordagem para o problema dos pokemons (batalhas)</h4>
    </li>
    <li>
        Rotas.md
        <h4>Descreve nossa abordagem para o problema da rota dos ginásios</h4>
    </li>
    <li>
        program.cpp
        <h4>Executa todo o cálculo de acordo com nossa modelagem de problema (A*, GA, verificações e saída)</h4>
    </li>
    <li>
        kanto.txt
        <h4>Mapa 150 x 42 (A M F R . = terrenos, 1 = origem, U = destino, demais letras/números = ginásios)</h4>
    </li>
    <li>
        visualizador.html
        <h4>Interface gráfica 2D (lê o resultado.js gerado pelo program)</h4>
    </li>
</ul>

<h2 style="border : none">Como compilar e rodar</h2>

Requer um compilador C++17 (g++ ou clang++). Em um terminal, dentro da pasta do projeto:

```
make                      # compila (gera ./program)
./program                 # roda com kanto.txt e imprime o resultado completo
```

No macOS com Command Line Tools recentes, se a linkagem falhar com `unknown architecture arm64e.x1`, use um SDK mais antigo:

```
export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk
```

Sem `make`: `g++ -O2 -std=c++17 program.cpp -o program`.

O A* com os 24 ginásios leva cerca de 2 minutos e usa cerca de 2 GB de RAM (o espaço de estados tem até 2^24 x 24 estados).

### Opções

| Opção | Efeito |
|---|---|
| `mapa.txt` | usa outro mapa (padrão: `kanto.txt`) |
| `--runs N` | número de execuções do GA no experimento (padrão: 30) |
| `--seed S` | semente aleatória (resultado reprodutível) |
| `--verificar` | confere o custo da rota com Held-Karp (+ ~30 s e ~800 MB) |
| `--anim MS` | anima o percurso no terminal, com MS ms por quadro |
| `--js arq.js` / `--sem-js` | arquivo de saída para o visualizador (padrão: `resultado.js`) |
| `--ginasios N` | usa só os N primeiros ginásios (depuração) |
| `--so-batalhas` | pula o A* e roda só o problema das batalhas (depuração) |

### Visualização gráfica

Depois de rodar `./program`, abra `visualizador.html` no navegador (na mesma pasta de `resultado.js`).

### Configuração

Tudo que o enunciado pede para ser editável está no início de `program.cpp`: custos dos terrenos (`CUSTO_TERRENO`), dificuldade dos ginásios (`GINASIOS`), poder dos Pokémon (`POKEMONS`), energia inicial e parâmetros do GA. O mapa é o arquivo de texto.

<h2 style="border : none">Resultados (mapa kanto.txt)</h2>

| Item | Valor |
|---|---|
| Ordem de visita | 1 → O → J → K → N → L → H → G → E → C → D → B → 8 → 9 → T → S → 5 → 6 → 7 → 4 → I → 3 → 2 → P → Q → U |
| C_rota | 1100 min (A\* e Held-Karp concordam) |
| C_batalhas | 1484.6750 min (ótimo exato do DP) |
| C_total | 2584.6750 min |
| Estados expandidos pelo A\* | 54 155 556 (175 837 856 gerados), ~130 s, ~1,8 GB |
| Energia final | Pikachu 0, Bulbassauro 0, Rattata 0, Caterpie 0, Weedle 1 |

Experimento da busca local (30 execuções, semente 1, melhor / média / desvio-padrão):

| Método | Melhor | Média | Desvio |
|---|---|---|---|
| GA puro | 1484.7656 | 1487.8920 | 3.0738 |
| GA + hill climbing | 1484.7656 | 1487.8920 | 3.0738 |

O GA chega a menos de 0,01% do ótimo no melhor caso (média a 0,2%) , mas nas 30 execuções nenhuma terminou exatamente no ótimo e o refinamento por hill climbing não melhorou nenhuma delas (as soluções do GA já são ótimos locais para essa vizinhança). Por isso o plano final usa a solução do DP , como avisado na saída.

<h2 style="border : none">Discussão da otimalidade</h2>

- **Rota:** o A\* usa uma heurística admissível e consistente , então a rota devolvida é ótima (para o custo definido no enunciado). Conferimos o valor de forma independente com Held-Karp (1100 nos dois).
- **Batalhas:** o GA é uma busca local , **sem garantia** de ótimo. Como a energia não depende da ordem dos ginásios , o problema é pequeno e um DP exato encontra o ótimo global das batalhas (1484.6750). O programa imprime esse valor e , se o melhor GA ficou acima dele , usa o plano do DP como plano final (isso é avisado na saída).
- **Global:** como $C_{total} = C_{rota} + C_{batalhas}$ e as duas partes não interferem (a ordem escolhida pela rota não altera o custo ótimo das batalhas) , a soma dos dois ótimos é o ótimo global : **2584.6750 min**. O ponto sem garantia formal é só o GA em si.

