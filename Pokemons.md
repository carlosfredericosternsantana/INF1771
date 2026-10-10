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
    \frac{a}{d}+\frac{c}{b} - \frac{a}{b}-\frac{c}{d}
$$

$$ 
    \frac{a}{b}+\frac{c}{d} - \frac{a}{d}+\frac{c}{b} = \frac{(c-a).(d-b)}{b.d} 
$$
Como c > a e d > b e todos são maiores do que 0 , certamente o resultado é positivo o que infere 
$$ 
    \frac{a}{d}+\frac{c}{b} > \frac{a}{b}-\frac{c}{d}
$$
E sem perda de generalidade a quaisquer valores , e isso segue valendo para quaiquer duplas tais que respeitem as premissas do argumento .




Dessa maneira essa resolução para o subproblema é ótima , mas para a disposição dos 5 pokemons subsequentes isso não se vale .

E é nesse sentido , que vamos utilizar o algoritmo genético para calcular melhor disposição desses pokemons subsequentes