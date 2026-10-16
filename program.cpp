// INF1771 - Trabalho 1: busca heuristica no mapa de Kanto.
//   Problema 2 (rota): A* sobre o grafo reduzido (ponto atual, ginasios restantes).
//   Problema 1 (batalhas): Algoritmo Genetico + verificacao exata por programacao dinamica.
//
// Uso: ./program [mapa.txt] [--runs N] [--seed S] [--anim MS] [--js arq.js] [--sem-js] [--verificar]
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>
using namespace std;
typedef long long ll;

// ============================ CONFIGURACAO ============================
// Tudo que o enunciado pede para ser editavel esta nesta secao.

// Custo (em minutos) de entrar em cada tipo de celula. Ginasios, origem e destino custam CUSTO_PADRAO.
const map<char, ll> CUSTO_TERRENO = {{'M', 200}, {'A', 30}, {'F', 15}, {'R', 5}, {'.', 1}};
const ll CUSTO_PADRAO = 1;

// Dificuldade de cada ginasio, na ordem dos ginasios (1o Gym, 2o Gym, ...).
const vector<pair<char, double>> GINASIOS = {
    {'2', 35},  {'3', 40},  {'4', 45},  {'5', 50},  {'6', 55},  {'7', 60},  {'8', 65},  {'9', 70},
    {'B', 75},  {'C', 80},  {'D', 85},  {'E', 90},  {'G', 95},  {'H', 100}, {'I', 110}, {'J', 120},
    {'K', 130}, {'L', 140}, {'N', 150}, {'O', 155}, {'P', 160}, {'Q', 165}, {'S', 170}, {'T', 180}};

// Fator de poder de cada Pokemon e energia inicial.
struct PokemonInfo { string nome; double poder; };
const vector<PokemonInfo> POKEMONS = {
    {"Pikachu", 1.5}, {"Bulbassauro", 1.4}, {"Rattata", 1.3}, {"Caterpie", 1.2}, {"Weedle", 1.1}};
const int ENERGIA_INICIAL = 6;

// Parametros do algoritmo genetico.
const int GA_POPULACAO = 300;
const int GA_GERACOES = 1500;
const int GA_TORNEIO = 3;
const int GA_ELITE = 2;
const double GA_MUTACAO = 0.04;  // probabilidade de mutacao por gene

const ll INF = (ll)1e18;
int LIMITE_GINASIOS = 24;  // so para depuracao (--ginasios N)
const char ORIGEM = '1', DESTINO = 'U';

// ================================ MAPA ================================
struct Mapa {
    int H = 0, W = 0;
    vector<string> grade;
    int k = 0;              // numero de ginasios; pontos: 0..k-1 ginasios, k = origem, k+1 = destino
    vector<char> simbolo;   // simbolo de cada ponto
    vector<int> pos;        // posicao linear (linha * W + coluna) de cada ponto
};

ll custoCelula(char c) {
    auto it = CUSTO_TERRENO.find(c);
    return it == CUSTO_TERRENO.end() ? CUSTO_PADRAO : it->second;
}

bool carregarMapa(const string& arquivo, Mapa& m) {
    ifstream in(arquivo);
    if (!in) { cerr << "Nao foi possivel abrir o mapa: " << arquivo << "\n"; return false; }
    string linha;
    while (in >> linha) m.grade.push_back(linha);
    if (m.grade.empty()) { cerr << "Mapa vazio.\n"; return false; }
    m.H = m.grade.size();
    m.W = m.grade[0].size();
    map<char, int> achados;
    for (int i = 0; i < m.H; i++) {
        if ((int)m.grade[i].size() != m.W) { cerr << "Linha " << i << " com tamanho diferente.\n"; return false; }
        for (int j = 0; j < m.W; j++) {
            char c = m.grade[i][j];
            if (!CUSTO_TERRENO.count(c)) achados[c] = i * m.W + j;
        }
    }
    for (auto& g : GINASIOS) {
        if ((int)m.simbolo.size() >= LIMITE_GINASIOS) break;
        if (!achados.count(g.first)) { cerr << "Ginasio '" << g.first << "' ausente no mapa.\n"; return false; }
        m.simbolo.push_back(g.first);
        m.pos.push_back(achados[g.first]);
    }
    m.k = m.simbolo.size();
    for (char c : {ORIGEM, DESTINO}) {
        if (!achados.count(c)) { cerr << "Ponto '" << c << "' ausente no mapa.\n"; return false; }
        m.simbolo.push_back(c);
        m.pos.push_back(achados[c]);
    }
    return true;
}

// ============================= DIJKSTRA ==============================
// Menor custo da celula de origem ate todas as outras (movimento em 4 direcoes).
// O custo de um caminho e a soma dos custos das celulas em que se entra.
struct Caminhos { vector<ll> dist; vector<int> prev; };

Caminhos dijkstra(const Mapa& m, int origem) {
    Caminhos r;
    r.dist.assign(m.H * m.W, INF);
    r.prev.assign(m.H * m.W, -1);
    typedef pair<ll, int> P;
    priority_queue<P, vector<P>, greater<P>> pq;
    r.dist[origem] = 0;
    pq.push({0, origem});
    const int dl[4] = {1, -1, 0, 0}, dc[4] = {0, 0, 1, -1};
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > r.dist[u]) continue;
        int li = u / m.W, co = u % m.W;
        for (int t = 0; t < 4; t++) {
            int ni = li + dl[t], nj = co + dc[t];
            if (ni < 0 || nj < 0 || ni >= m.H || nj >= m.W) continue;
            int v = ni * m.W + nj;
            ll nd = d + custoCelula(m.grade[ni][nj]);
            if (nd < r.dist[v]) { r.dist[v] = nd; r.prev[v] = u; pq.push({nd, v}); }
        }
    }
    return r;
}

// ============================ A* DA ROTA =============================
// Estado = (ponto atual, conjunto de ginasios ainda nao visitados).
// Acao = ir ate um ginasio nao visitado (aresta do grafo reduzido, custo = menor caminho no mapa).
// g = custo acumulado; h = MST de {atual, destino} U restantes (limite inferior admissivel de qualquer
// caminho que passe por todos os restantes e termine no destino).
struct ResultadoRota {
    vector<int> ordem;      // ginasios na ordem de visita (indices 0..k-1)
    ll custo = 0;           // custo total da rota (inclui a origem)
    ll expandidos = 0, gerados = 0;
    bool ok = false;
};

// Implementacao compacta (o espaco tem ate 2^24 * 24 estados):
//  - g guardado em vetor indexado por (mascara, atual), 16 bits (0xFFFF = nao gerado);
//  - fila de prioridade por baldes (f e inteiro), LIFO dentro do balde (prefere nos mais profundos);
//  - a heuristica e consistente, entao um estado fechado nunca precisa ser reaberto;
//  - avaliacao preguicosa: um filho entra na fila com f do pai (limite inferior valido, pela consistencia)
//    e so calcula a MST quando chega a ser desempilhado.
ResultadoRota aStarRota(const Mapa& m, const vector<vector<ll>>& d) {
    ResultadoRota res;
    int k = m.k, dest = k + 1;
    auto heuristica = [&](int atual, uint32_t restantes) -> ll {
        int nos[32], n = 0;
        nos[n++] = atual;
        nos[n++] = dest;
        for (int i = 0; i < k; i++) if (restantes >> i & 1) nos[n++] = i;
        ll menor[32];
        bool dentro[32] = {};
        for (int i = 0; i < n; i++) menor[i] = INF;
        menor[0] = 0;
        ll custo = 0;
        for (int it = 0; it < n; it++) {
            int e = -1;
            for (int i = 0; i < n; i++) if (!dentro[i] && (e < 0 || menor[i] < menor[e])) e = i;
            dentro[e] = true;
            custo += menor[e];
            for (int i = 0; i < n; i++) if (!dentro[i]) menor[i] = min(menor[i], d[nos[e]][nos[i]]);
        }
        return custo;
    };
    const uint16_t NAO_GERADO = 0xFFFF;
    const uint32_t AVALIADO = 1u << 31;  // bit do no da fila: heuristica ja calculada
    uint32_t todos = (1u << k) - 1;
    // estado (v, r): v = ultimo ginasio visitado (0..k-1), r = ginasios restantes
    auto indice = [&](int v, uint32_t r) { return (size_t)r * k + v; };
    vector<uint16_t> G((size_t)(1u << k) * k, NAO_GERADO);
    vector<bool> fechado((size_t)(1u << k) * k, false);
    vector<vector<uint32_t>> baldes(1);
    auto empilhar = [&](ll f, uint32_t no) {
        if (f >= (ll)baldes.size()) baldes.resize(f + 1);
        baldes[f].push_back(no);
    };
    auto guardarG = [&](int v, uint32_t r, ll g, ll fPai) {
        if (g >= NAO_GERADO) { cerr << "Custo acima de 65534: aumente o tipo de g.\n"; exit(1); }
        G[indice(v, r)] = (uint16_t)g;
        res.gerados++;
        empilhar(fPai, (r << 5) | v);
    };
    // expande a origem (estado inicial, fora do vetor G)
    ll fOrigem = 1 + heuristica(k, todos);
    for (int v = 0; v < k; v++) guardarG(v, todos & ~(1u << v), 1 + d[k][v], fOrigem);

    size_t cursor = 0;
    while (true) {
        while (cursor < baldes.size() && baldes[cursor].empty()) cursor++;
        if (cursor == baldes.size()) return res;  // fila vazia: sem solucao
        uint32_t no = baldes[cursor].back();
        baldes[cursor].pop_back();
        bool avaliado = no & AVALIADO;
        no &= ~AVALIADO;
        int v = no & 31u;
        uint32_t r = no >> 5;
        size_t id = indice(v, r);
        if (fechado[id]) continue;
        ll g = G[id];
        if (!avaliado) {
            ll f = g + heuristica(v, r);
            if (f > (ll)cursor) { empilhar(f, no | AVALIADO); continue; }
        }
        fechado[id] = true;
        res.expandidos++;
        if (!r) {
            // reconstroi a ordem: o pai de (v, r) e o (u, r | v) que minimiza G + d[u][v]
            vector<int> ordem = {v};
            int cv = v;
            uint32_t cr = r;
            while (true) {
                uint32_t rp = cr | (1u << cv);
                ll gAtual = G[indice(cv, cr)];
                if (rp == todos && 1 + d[k][cv] == gAtual) break;  // veio da origem
                int melhorU = -1;
                ll melhorC = INF;
                for (int u = 0; u < k; u++) {
                    if (rp >> u & 1) continue;  // u ja visitado: nao esta em rp
                    ll c = (ll)G[indice(u, rp)] + d[u][cv];
                    if (G[indice(u, rp)] != NAO_GERADO && c < melhorC) { melhorC = c; melhorU = u; }
                }
                if (melhorU < 0) { cerr << "Falha ao reconstruir a rota.\n"; exit(1); }
                ordem.push_back(melhorU);
                cr = rp;
                cv = melhorU;
            }
            reverse(ordem.begin(), ordem.end());
            res.ordem = ordem;
            res.custo = g + d[v][dest];
            res.ok = true;
            return res;
        }
        for (int w = 0; w < k; w++) {
            if (!(r >> w & 1)) continue;
            uint32_t r2 = r & ~(1u << w);
            ll g2 = g + d[v][w];
            size_t id2 = indice(w, r2);
            if (G[id2] != NAO_GERADO && g2 >= G[id2]) continue;
            guardarG(w, r2, g2, cursor);  // f do pai: limite inferior do filho (consistencia)
        }
    }
}


// Verificador independente do A*: Held-Karp (DP sobre subconjuntos), O(2^k * k^2). Usa ~800 MB para k = 24.
ll heldKarp(const Mapa& m, const vector<vector<ll>>& d) {
    int k = m.k;
    size_t N = (size_t)1 << k;
    vector<uint16_t> dp(N * k, 0xFFFF);
    for (int v = 0; v < k; v++) dp[((size_t)1 << v) * k + v] = 1 + d[k][v];
    for (size_t mask = 1; mask < N; mask++)
        for (int u = 0; u < k; u++) {
            ll cur = dp[mask * k + u];
            if (cur == 0xFFFF) continue;
            for (int v = 0; v < k; v++) {
                if (mask >> v & 1) continue;
                ll c = cur + d[u][v];
                uint16_t& alvo = dp[(mask | (size_t)1 << v) * k + v];
                if (c < alvo) alvo = (uint16_t)c;
            }
        }
    ll melhor = INF;
    for (int v = 0; v < k; v++) melhor = min(melhor, (ll)dp[(N - 1) * k + v] + d[v][k + 1]);
    return melhor;
}

// ============================ BATALHAS ===============================
// Solucao: uma mascara de 5 bits por ginasio (quais Pokemon lutam). Como a energia so diminui, o custo
// das batalhas e as restricoes nao dependem da ordem dos ginasios, so de qual ginasio recebe qual mascara.
const int NP = 5;
vector<double> PODER_MASCARA(1 << NP, 0.0);

void prepararPoder() {
    for (int s = 1; s < (1 << NP); s++)
        for (int j = 0; j < NP; j++) if (s >> j & 1) PODER_MASCARA[s] += POKEMONS[j].poder;
}

double custoBatalhas(const vector<double>& dif, const vector<uint8_t>& mascara) {
    double c = 0;
    for (size_t i = 0; i < dif.size(); i++) c += dif[i] / PODER_MASCARA[mascara[i]];
    return c;
}

// Usos de cada Pokemon; valido se nenhum passa da energia e pelo menos um sobra com energia >= 1.
vector<int> usos(const vector<uint8_t>& mascara) {
    vector<int> u(NP, 0);
    for (uint8_t s : mascara) for (int j = 0; j < NP; j++) if (s >> j & 1) u[j]++;
    return u;
}
bool valido(const vector<uint8_t>& mascara) {
    for (uint8_t s : mascara) if (!s) return false;
    auto u = usos(mascara);
    bool sobra = false;
    for (int j = 0; j < NP; j++) {
        if (u[j] > ENERGIA_INICIAL) return false;
        if (u[j] < ENERGIA_INICIAL) sobra = true;
    }
    return sobra;
}

// Reparo: remove usos em excesso / garante um sobrevivente, sempre mantendo >= 1 Pokemon por batalha.
void reparar(vector<uint8_t>& mascara, mt19937& rng) {
    int n = mascara.size();
    for (int guarda = 0; guarda < 10000; guarda++) {
        auto u = usos(mascara);
        int sobre = -1;
        for (int j = 0; j < NP; j++) if (u[j] > ENERGIA_INICIAL) { sobre = j; break; }
        if (sobre < 0) {
            bool sobra = false;
            for (int j = 0; j < NP; j++) if (u[j] < ENERGIA_INICIAL) sobra = true;
            if (sobra) return;
            sobre = rng() % NP;  // todos no limite: tira um uso de algum Pokemon
        }
        vector<int> multi, unico;
        for (int i = 0; i < n; i++) if (mascara[i] >> sobre & 1) (__builtin_popcount(mascara[i]) > 1 ? multi : unico).push_back(i);
        if (!multi.empty()) {
            mascara[multi[rng() % multi.size()]] &= ~(1 << sobre);
            continue;
        }
        vector<int> livres;
        for (int j = 0; j < NP; j++) if (u[j] < ENERGIA_INICIAL) livres.push_back(j);
        if (!unico.empty() && !livres.empty()) {
            mascara[unico[rng() % unico.size()]] = 1 << livres[rng() % livres.size()];
        } else {
            vector<int> outros;  // qualquer ginasio com mais de um Pokemon
            for (int i = 0; i < n; i++) if (__builtin_popcount(mascara[i]) > 1) outros.push_back(i);
            if (outros.empty()) return;
            int i = outros[rng() % outros.size()];
            vector<int> bits;
            for (int j = 0; j < NP; j++) if (mascara[i] >> j & 1) bits.push_back(j);
            mascara[i] &= ~(1 << bits[rng() % bits.size()]);
        }
    }
}

struct PlanoBatalhas { vector<uint8_t> mascara; double custo = 1e18; };

// Otimo exato por DP sobre (ginasio, usos de cada Pokemon em base 7): 24 * 7^5 * 31 operacoes.
// Serve para validar o GA e responder a pergunta de otimalidade.
PlanoBatalhas otimoExato(const vector<double>& dif) {
    int n = dif.size(), E = ENERGIA_INICIAL + 1;
    int total = 1;
    vector<int> pot(NP);
    for (int j = 0; j < NP; j++) { pot[j] = total; total *= E; }
    vector<int> soma(1 << NP, 0);
    for (int s = 1; s < (1 << NP); s++) for (int j = 0; j < NP; j++) if (s >> j & 1) soma[s] += pot[j];
    vector<double> cur(total, 1e18), nxt;
    vector<vector<uint8_t>> escolha(n, vector<uint8_t>(total, 0));
    cur[0] = 0;
    for (int i = 0; i < n; i++) {
        nxt.assign(total, 1e18);
        for (int st = 0; st < total; st++) {
            if (cur[st] >= 1e17) continue;
            int dig[NP], t = st;
            for (int j = 0; j < NP; j++) { dig[j] = t % E; t /= E; }
            for (int s = 1; s < (1 << NP); s++) {
                bool ok = true;
                for (int j = 0; j < NP && ok; j++) if ((s >> j & 1) && dig[j] >= ENERGIA_INICIAL) ok = false;
                if (!ok) continue;
                int ns = st + soma[s];
                double c = cur[st] + dif[i] / PODER_MASCARA[s];
                if (c < nxt[ns]) { nxt[ns] = c; escolha[i][ns] = s; }
            }
        }
        cur.swap(nxt);
    }
    PlanoBatalhas p;
    int melhorSt = -1;
    for (int st = 0; st < total; st++) {
        if (cur[st] >= 1e17) continue;
        bool sobra = false;
        int t = st;
        for (int j = 0; j < NP; j++) { if (t % E < ENERGIA_INICIAL) sobra = true; t /= E; }
        if (sobra && (melhorSt < 0 || cur[st] < cur[melhorSt])) melhorSt = st;
    }
    if (melhorSt < 0) return p;
    p.custo = cur[melhorSt];
    p.mascara.assign(n, 0);
    int st = melhorSt;
    for (int i = n - 1; i >= 0; i--) { p.mascara[i] = escolha[i][st]; st -= soma[p.mascara[i]]; }
    return p;
}

// Algoritmo genetico: individuo = vetor de mascaras (uma por ginasio); fitness = custo das batalhas
// (menor e melhor). Cruzamento uniforme, mutacao por troca de bit, torneio, elitismo e reparo.
PlanoBatalhas algoritmoGenetico(const vector<double>& dif, mt19937& rng) {
    int n = dif.size();
    struct Ind { vector<uint8_t> m; double c; };
    auto avaliar = [&](Ind& x) { x.c = custoBatalhas(dif, x.m); };
    vector<Ind> pop(GA_POPULACAO);
    for (auto& x : pop) {
        x.m.resize(n);
        for (auto& g : x.m) g = 1 + rng() % ((1 << NP) - 1);
        reparar(x.m, rng);
        avaliar(x);
    }
    auto melhorDe = [&](const vector<Ind>& p) {
        return *min_element(p.begin(), p.end(), [](const Ind& a, const Ind& b) { return a.c < b.c; });
    };
    uniform_real_distribution<double> U(0, 1);
    auto torneio = [&]() -> const Ind& {
        const Ind* b = &pop[rng() % pop.size()];
        for (int t = 1; t < GA_TORNEIO; t++) {
            const Ind& o = pop[rng() % pop.size()];
            if (o.c < b->c) b = &o;
        }
        return *b;
    };
    for (int g = 0; g < GA_GERACOES; g++) {
        sort(pop.begin(), pop.end(), [](const Ind& a, const Ind& b) { return a.c < b.c; });
        vector<Ind> nova(pop.begin(), pop.begin() + GA_ELITE);
        while ((int)nova.size() < GA_POPULACAO) {
            const Ind& a = torneio();
            const Ind& b = torneio();
            Ind f;
            f.m.resize(n);
            for (int i = 0; i < n; i++) f.m[i] = (rng() & 1) ? a.m[i] : b.m[i];  // cruzamento uniforme
            for (int i = 0; i < n; i++) if (U(rng) < GA_MUTACAO) {
                if (rng() & 1) {  // troca de bit: adiciona ou remove um Pokemon do ginasio
                    uint8_t novo = f.m[i] ^ (1 << (rng() % NP));
                    if (novo) f.m[i] = novo;
                } else {  // transferencia: move um Pokemon do ginasio i para outro ginasio (mantem os usos)
                    int j = rng() % n, p = rng() % NP;
                    if ((f.m[i] >> p & 1) && !(f.m[j] >> p & 1) && __builtin_popcount(f.m[i]) > 1) {
                        f.m[i] &= ~(1 << p);
                        f.m[j] |= 1 << p;
                    }
                }
            }
            reparar(f.m, rng);
            avaliar(f);
            nova.push_back(std::move(f));
        }
        pop.swap(nova);
    }
    Ind b = melhorDe(pop);
    return {b.m, b.c};
}


// Hill climbing (subida de encosta mais inclinada) sobre a vizinhanca: trocar a mascara de um ginasio ou
// transferir um Pokemon de um ginasio para outro. Usado para refinar a solucao do GA.
PlanoBatalhas hillClimbing(const vector<double>& dif, PlanoBatalhas p) {
    int n = dif.size();
    while (true) {
        PlanoBatalhas melhor = p;
        for (int i = 0; i < n; i++) {
            for (int s = 1; s < (1 << NP); s++) {
                if (s == p.mascara[i]) continue;
                PlanoBatalhas q = p;
                q.mascara[i] = s;
                q.custo = custoBatalhas(dif, q.mascara);
                if (q.custo < melhor.custo - 1e-12 && valido(q.mascara)) melhor = q;
            }
            for (int j = 0; j < n; j++) for (int e = 0; e < NP; e++) {
                if (!(p.mascara[i] >> e & 1) || (p.mascara[j] >> e & 1) || __builtin_popcount(p.mascara[i]) < 2) continue;
                PlanoBatalhas q = p;
                q.mascara[i] &= ~(1 << e);
                q.mascara[j] |= 1 << e;
                q.custo = custoBatalhas(dif, q.mascara);
                if (q.custo < melhor.custo - 1e-12 && valido(q.mascara)) melhor = q;
            }
        }
        if (melhor.custo >= p.custo - 1e-12) return p;
        p = melhor;
    }
}

// ============================ APRESENTACAO ===========================
vector<int> caminhoCelulas(const Mapa& m, const vector<Caminhos>& cam, int de, int para) {
    vector<int> c;
    for (int u = m.pos[para]; u != m.pos[de]; u = cam[de].prev[u]) c.push_back(u);
    reverse(c.begin(), c.end());
    return c;  // exclui a celula de partida
}

string nomeGinasio(int idx) { return "Gym " + to_string(idx + 1) + " (" + string(1, GINASIOS[idx].first) + ")"; }

struct Evento { int passo; int ponto; };  // passo = indice no percurso em que o ponto foi alcancado

void desenhar(const Mapa& m, const vector<int>& percurso, int ate, const vector<char>& visitado, int alvo, bool cor) {
    vector<char> trilha(m.H * m.W, 0);
    for (int i = 0; i <= ate && i < (int)percurso.size(); i++) trilha[percurso[i]] = 1;
    int agente = percurso[min(ate, (int)percurso.size() - 1)];
    string saida;
    for (int i = 0; i < m.H; i++) {
        for (int j = 0; j < m.W; j++) {
            int p = i * m.W + j;
            char c = m.grade[i][j];
            string cel(1, c), cr;
            if (p == agente) { cel = "@"; cr = "\033[1;97;41m"; }
            else if (p == m.pos[alvo]) { cr = "\033[1;30;43m"; }
            else if (trilha[p]) { cel = "*"; cr = "\033[91m"; }
            else if (!CUSTO_TERRENO.count(c)) {
                bool v = false;
                for (int q = 0; q < (int)m.pos.size(); q++) if (m.pos[q] == p && visitado[q]) v = true;
                cr = v ? "\033[1;32m" : "\033[1;33m";
            }
            else if (c == 'M') cr = "\033[33m";
            else if (c == 'A') cr = "\033[34m";
            else if (c == 'F') cr = "\033[32m";
            else if (c == 'R') cr = "\033[90m";
            if (cor && !cr.empty()) saida += cr + cel + "\033[0m"; else saida += cel;
        }
        saida += "\n";
    }
    cout << saida;
}

int main(int argc, char** argv) {
    string arquivo = "kanto.txt", js = "resultado.js";
    bool verificar = false;
    bool soBatalhas = false;  // depuracao: pula o A* (o custo das batalhas nao depende da ordem)
    int runs = 30, animMs = 0;
    unsigned seed = random_device{}();
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        if (a == "--runs" && i + 1 < argc) runs = atoi(argv[++i]);
        else if (a == "--seed" && i + 1 < argc) seed = atoi(argv[++i]);
        else if (a == "--anim" && i + 1 < argc) animMs = atoi(argv[++i]);
        else if (a == "--js" && i + 1 < argc) js = argv[++i];
        else if (a == "--ginasios" && i + 1 < argc) LIMITE_GINASIOS = atoi(argv[++i]);
        else if (a == "--verificar") verificar = true;
        else if (a == "--so-batalhas") soBatalhas = true;
        else if (a == "--sem-js") js = "";
        else if (a[0] != '-') arquivo = a;
        else { cerr << "Opcao desconhecida: " << a << "\n"; return 1; }
    }
    prepararPoder();
    Mapa m;
    if (!carregarMapa(arquivo, m)) return 1;
    int k = m.k, np = k + 2;

    // ---- menores caminhos entre todos os pontos de interesse ----
    vector<Caminhos> cam;
    for (int i = 0; i < np; i++) cam.push_back(dijkstra(m, m.pos[i]));
    vector<vector<ll>> d(np, vector<ll>(np, 0));
    for (int i = 0; i < np; i++) for (int j = 0; j < np; j++) d[i][j] = cam[i].dist[m.pos[j]];

    // ---- Problema 2: rota ----
    auto t0 = chrono::steady_clock::now();
    ResultadoRota rota;
    if (soBatalhas) { for (int i = 0; i < k; i++) rota.ordem.push_back(i); rota.ok = true; }
    else rota = aStarRota(m, d);
    double tRota = chrono::duration<double>(chrono::steady_clock::now() - t0).count();
    if (!rota.ok) { cerr << "Nenhuma rota encontrada.\n"; return 1; }

    // ---- Problema 1: batalhas ----
    vector<double> dif;  // dificuldade na ordem de visita
    for (int g : rota.ordem) dif.push_back(GINASIOS[g].second);
    PlanoBatalhas exato = otimoExato(dif);
    mt19937 rng(seed);
    vector<double> custosGA, custosHC;
    PlanoBatalhas melhor;
    for (int r = 0; r < runs; r++) {
        PlanoBatalhas p = algoritmoGenetico(dif, rng);
        if (!valido(p.mascara)) { cerr << "GA gerou solucao invalida!\n"; return 1; }
        custosGA.push_back(p.custo);
        p = hillClimbing(dif, p);
        if (!valido(p.mascara)) { cerr << "HC gerou solucao invalida!\n"; return 1; }
        custosHC.push_back(p.custo);
        if (p.custo < melhor.custo) melhor = p;
    }
    auto estat = [](const vector<double>& v, double& media, double& desvio) {
        media = desvio = 0;
        for (double c : v) media += c;
        media /= v.size();
        for (double c : v) desvio += (c - media) * (c - media);
        desvio = sqrt(desvio / v.size());
    };
    double mediaGA, desvioGA, mediaHC, desvioHC;
    estat(custosGA, mediaGA, desvioGA);
    estat(custosHC, mediaHC, desvioHC);
    auto atingiu = [&](const vector<double>& v) {
        int n = 0;
        for (double c : v) if (c <= exato.custo + 1e-9) n++;
        return n;
    };

    // O DP certifica o otimo: se o melhor GA ficou acima dele, o plano final usa a solucao do DP.
    double melhorGA = *min_element(custosGA.begin(), custosGA.end());
    double melhorHC = melhor.custo;
    bool usouDP = exato.custo < melhor.custo - 1e-9;
    if (usouDP) melhor = exato;

    // ---- resultado ----
    cout << "================ ROTA (A*) ================\n";
    cout << "Ordem de visita: 1";
    for (int g : rota.ordem) cout << " -> " << GINASIOS[g].first;
    cout << " -> U\n";
    cout << "Custo da rota (inclui origem e destino): " << rota.custo << " min\n";
    cout << "Estados expandidos: " << rota.expandidos << " | gerados: " << rota.gerados
         << " | tempo: " << tRota << " s\n";
    if (verificar) {
        ll hk = heldKarp(m, d);
        cout << "Verificacao Held-Karp: " << hk << (hk == rota.custo ? " (confere: rota otima)" : " (DIVERGE!)") << "\n";
    }
    cout << "\n";

    cout << "============== BATALHAS (GA) ==============\n";
    vector<int> energia(NP, ENERGIA_INICIAL);
    double cBat = 0;
    vector<double> tempoBat;
    for (size_t i = 0; i < rota.ordem.size(); i++) {
        uint8_t s = melhor.mascara[i];
        double t = dif[i] / PODER_MASCARA[s];
        tempoBat.push_back(t);
        cBat += t;
        cout << nomeGinasio(rota.ordem[i]) << " dif=" << dif[i] << "  Pokemon:";
        for (int j = 0; j < NP; j++) if (s >> j & 1) { cout << " " << POKEMONS[j].nome; energia[j]--; }
        printf("  tempo=%.3f\n", t);
    }
    cout << "\nEnergia final:";
    for (int j = 0; j < NP; j++) cout << " " << POKEMONS[j].nome << "=" << energia[j];
    printf("\nCusto das batalhas: %.4f min\n", cBat);
    printf("Experimento (%d execucoes, melhor / media / desvio-padrao / vezes no otimo):\n", runs);
    printf("  GA puro  : %.4f / %.4f / %.4f / %d\n", melhorGA, mediaGA, desvioGA, atingiu(custosGA));
    printf("  GA + HC  : %.4f / %.4f / %.4f / %d\n", melhorHC, mediaHC, desvioHC, atingiu(custosHC));
    printf("Otimo exato (DP): %.4f min\n", exato.custo);
    if (usouDP) printf("(O GA nao atingiu o otimo; o plano acima e o plano otimo do DP.)\n");
    cout << "\n";

    printf("================== TOTAL ==================\n");
    printf("C_rota = %lld | C_batalhas = %.4f | C_total = %.4f min\n", rota.custo, cBat, rota.custo + cBat);

    // ---- percurso celula a celula ----
    vector<int> percurso = {m.pos[k]};
    vector<Evento> eventos = {{0, k}};
    vector<int> pontos = {k};
    for (int g : rota.ordem) pontos.push_back(g);
    pontos.push_back(k + 1);
    for (size_t i = 0; i + 1 < pontos.size(); i++) {
        auto c = caminhoCelulas(m, cam, pontos[i], pontos[i + 1]);
        percurso.insert(percurso.end(), c.begin(), c.end());
        eventos.push_back({(int)percurso.size() - 1, pontos[i + 1]});
    }

    if (animMs > 0) {
        vector<char> visitado(np, 0);
        size_t prox = 1;
        ll custoMov = 1;
        double custoBat = 0;
        visitado[k] = 1;
        int frame = max<int>(1, percurso.size() / 300);
        for (int p = 0; p < (int)percurso.size(); p++) {
            if (p > 0) custoMov += custoCelula(m.grade[percurso[p] / m.W][percurso[p] % m.W]);
            bool chegou = prox < eventos.size() && eventos[prox].passo == p;
            if (chegou) {
                int pt = eventos[prox].ponto;
                visitado[pt] = 1;
                if (pt < k) custoBat += tempoBat[prox - 1];
                prox++;
            }
            if (p % frame == 0 || chegou || p + 1 == (int)percurso.size()) {
                cout << "\033[H\033[2J";
                desenhar(m, percurso, p, visitado, prox < eventos.size() ? eventos[prox].ponto : k + 1, true);
                printf("Passo %d/%zu | custo de rota=%lld | custo de batalhas=%.2f | total=%.2f\n", p,
                       percurso.size() - 1, custoMov, custoBat, custoMov + custoBat);
                cout.flush();
                this_thread::sleep_for(chrono::milliseconds(animMs));
            }
        }
    } else {
        vector<char> visitado(np, 1);
        cout << "\nMapa com o percurso final (* = caminho, letras verdes = ginasios visitados):\n";
        desenhar(m, percurso, percurso.size() - 1, visitado, k + 1, isatty(1));
    }

    if (!js.empty()) {
        ofstream f(js);
        f << "const RESULTADO = {\n  largura: " << m.W << ", altura: " << m.H << ",\n  mapa: [";
        for (int i = 0; i < m.H; i++) f << (i ? "," : "") << "\"" << m.grade[i] << "\"";
        f << "],\n  caminho: [";
        for (size_t i = 0; i < percurso.size(); i++) f << (i ? "," : "") << percurso[i];
        f << "],\n  paradas: [";
        for (size_t i = 0; i < eventos.size(); i++) {
            int pt = eventos[i].ponto;
            f << (i ? "," : "") << "{passo:" << eventos[i].passo << ",simbolo:\"" << m.simbolo[pt] << "\"";
            if (pt < k) {
                uint8_t s = melhor.mascara[i - 1];
                f << ",dificuldade:" << dif[i - 1] << ",tempo:" << tempoBat[i - 1] << ",pokemon:[";
                bool prim = true;
                for (int j = 0; j < NP; j++) if (s >> j & 1) { f << (prim ? "" : ",") << "\"" << POKEMONS[j].nome << "\""; prim = false; }
                f << "]";
            }
            f << "}";
        }
        f << "],\n  custoRota: " << rota.custo << ", custoBatalhas: " << cBat
          << ", expandidos: " << rota.expandidos << "\n};\n";
        cout << "\n(Dados da visualizacao gravados em " << js << ": abra visualizador.html)\n";
    }
    return 0;
}
