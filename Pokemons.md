<h1 style="border : none">Considerações</h1>
<h3>Aqui constão algumas pontuações e análises que consideramos importantes na confecção do trabalho.</h3>


Ao total temos 5 pokemons , cada um com 6 de energia , o que significa que ao máximo podemos batalhar 30 vezes mas considerando que temos a restrição de que temo de ter um pokemon vivo ao menos ao fim de todas as batalhas e que lutaremos somente em ginásios e eles são 24 ao total : idealmente juntaresmo alguns pokemons e faremos efetivamente 29 batalhas .

Interessante que pelo princípio da casa dos pombos ao máximo poderemos repertir 5 pokemons ao longo das 24 batalhas para garantir que chegaremos a última casa ainda com um pokemon e que teremos pokemons para batalhar até a última casa . 


Outro ponto interessante focando ainda nos pokemons , nós temos o seguinte subproblema : 
Temos uma sequência : 

$$ A_n = a_1 , a_2 , a_3 ... a_n $$

Temos ainda outra sequência : 

$$ B_n = b_1 , b_2 , b_3 ... a_n $$

E queremos  : 

$$
    Min(\sum_{i=1}^{n}\frac{a_i}{b_j})
$$

A maneira eficiente de parear os pokemons com os ginásios (Nesse subproblema) segue a lógica intuitiva . 
Isso se faz verdadeiro , ao considerar um somatório ótimo até um passo k (0<=k<=24) suponha 0 < a < c e 0 < b < d 
e queremos analisar a relação 
$$
    \frac{a}{b}+\frac{c}{d} ? \frac{a}{d}+\frac{c}{b}
$$
para analisar essa relação podemos calcular : 
$$ 
    \left(\frac{a}{d}+\frac{c}{b}\right) - \left(\frac{a}{b}+\frac{c}{d}\right) = \frac{(c-a).(d-b)}{b.d} 
$$
Como c > a e d > b e todos são maiores do que 0 , certamente o resultado é positivo o que infere 
$$ 
    \frac{a}{b}+\frac{c}{d} < \frac{a}{d}+\frac{c}{b}
$$
ou seja , emparelhar o menor numerador (dificuldade) com o menor denominador (poder) e o maior com o maior é sempre melhor do que cruzar . E sem perda de generalidade a quaisquer valores , e isso segue valendo para quaiquer duplas tais que respeitem as premissas do argumento .

**Atenção:** esse argumento vale quando cada batalha usa **um único** pokemon (um denominador por batalha). No problema real o denominador de uma batalha é a *soma* dos poderes dos pokemons que participam , então ele só serve como intuição (ginásios mais difíceis ficam com os pokemons mais fortes / mais pokemons).

Dessa maneira essa resolução para o subproblema é ótima , mas para a disposição dos 5 pokemons subsequentes isso não se vale .

E é nesse sentido , que vamos utilizar o algoritmo genético para calcular melhor disposição desses pokemons subsequentes


## Modelagem implementada

**Energia não depende da ordem.** A energia só diminui e o único requisito é que nenhum pokemon lute mais de 6 vezes e que pelo menos um lute no máximo 5 (para chegar ao destino com energia >= 1). Logo o custo das batalhas **não depende da ordem** em que os ginásios são visitados , só de qual conjunto de pokemons é associado a cada ginásio. Isso separa os dois problemas :

$$ C_{total} = C_{rota} + C_{batalhas} $$

e o ótimo global é a soma do ótimo de cada parte (rota ótima + batalhas ótimas).

**Representação.** Um indivíduo é um vetor com 24 máscaras de 5 bits (uma por ginásio): o bit j diz se o pokemon j luta naquele ginásio. Toda máscara é não vazia.

**Restrições (reparo).** Depois do cruzamento e da mutação o indivíduo é reparado : se algum pokemon passa de 6 usos , ele é retirado de um ginásio que tenha outros pokemons (ou trocado por um com energia sobrando) ; se todos ficam no limite , um uso é removido. Assim a população só contém soluções válidas.

**Fitness.** $\sum_i \frac{dificuldade_i}{\sum poder(pokemons\ de\ i)}$ (quanto menor , melhor).

**Operadores.** População de 300 , 1500 gerações , torneio de 3 , elitismo de 2 , cruzamento uniforme e mutação por gene (4%) alternando entre trocar um bit e transferir um pokemon de um ginásio para outro (mantém a contagem de usos). O resultado de cada execução ainda passa por um *hill climbing* de refinamento (trocar a máscara de um ginásio ou transferir um pokemon).

**Verificação exata.** Como o espaço é pequeno , existe um DP exato sobre (ginásio , usos de cada pokemon em base 7) com $24 \cdot 7^5 \cdot 31$ operações. Ele certifica o ótimo e permite medir quão perto o GA chega.
