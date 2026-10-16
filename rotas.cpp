#include <bits/stdc++.h> 
using namespace std;
typedef long long ll;
typedef pair<ll,ll> pll;
typedef vector<ll> vll ;
#define forn(i,s,e) for (ll i = (s) ; i < (e) ; i++)

const ll inf = 1e18;



struct cmp  {

    bool operator () (pair<ll,pll> & p1 , pair<ll,pll>& p2 ) {
        return p1.first > p2.first ; 
    } 
};


ll weight (char c) {
    switch (c) {
        case 'M' : 
            return 200;
        case 'A' : 
            return 30;
        case 'F' : 
            return 15;
        case 'R' : 
            return 5;
        default : 
            return 1 ;
    } 

}
void dj (vector<string>& v ,  pll source , map<char,ll>& dist) {



    priority_queue<pair<ll,pll>,vector<pair<ll,pll>>,cmp> pq ; 
    pq.push({0,source});
    vector<vll> matriz(v.size(), vll (v[0].size(),inf));
    matriz[source.first][source.second]=0;
    vll deltax , deltay ;
    deltax = {1,-1,0,0};
    deltay = {0,0,1,-1};
    while (!pq.empty()) {
        auto next  = pq.top();
        // next.first é o peso do no
        // next.second.first é o x na matriz
        // next.second.second é o y na matriz
        pq.pop();
        if (next.first > matriz[next.second.first][next.second.second])
            continue;

        forn(i,0,4) 
            if (0<=next.second.first+deltax[i] && 0<=next.second.second+deltay[i] && next.second.first+deltax[i]<v.size() && next.second.second+deltay[i]<v[0].size()) {
                // esse é um vizinho que existe 
                if (matriz[next.second.first][next.second.second] + weight(v[next.second.first+deltax[i]][next.second.second+deltay[i]]) < matriz[next.second.first+deltax[i]][next.second.second+deltay[i]]) {
                    // verifica se a distancia atual do vizinho é melhor do que a sendo calculada no presente momento
                    matriz[next.second.first+deltax[i]][next.second.second+deltay[i]] = matriz[next.second.first][next.second.second] + weight(v[next.second.first+deltax[i]][next.second.second+deltay[i]]);
                    pq.push({matriz[next.second.first+deltax[i]][next.second.second+deltay[i]],{next.second.first+deltax[i],next.second.second+deltay[i]}});
                    
                    if  (v[next.second.first+deltax[i]][next.second.second+deltay[i]]!='A' && v[next.second.first+deltax[i]][next.second.second+deltay[i]]!='M' && v[next.second.first+deltax[i]][next.second.second+deltay[i]]!='F' 
                        && v[next.second.first+deltax[i]][next.second.second+deltay[i]]!='A' && v[next.second.first+deltax[i]][next.second.second+deltay[i]]!='.' && v[next.second.first+deltax[i]][next.second.second+deltay[i]]!='R') {
                        dist[v[next.second.first+deltax[i]][next.second.second+deltay[i]]] = matriz[next.second.first+deltax[i]][next.second.second+deltay[i]];
                    }
                }

            }


    }
}


// A* no grafo reduzido: estado = (ponto atual, ginasios restantes).
struct EstadoRotas {
    ll f, g;
    uint32_t restantes;
    int atual;
    bool operator<(const EstadoRotas& outro) const {
        if (f != outro.f) return f > outro.f;
        return g < outro.g;
    }
};

void astar_rotas(const map<char, map<char,ll>>& dist) {
    if (!dist.count('1') || !dist.count('U')) {
        cerr << "O mapa precisa conter origem 1 e destino U.\n";
        return;
    }
    vector<char> pontos;
    for (const auto& p : dist)
        if (p.first != '1' && p.first != 'U') pontos.push_back(p.first);
    int k = static_cast<int>(pontos.size());
    if (k > 24) {
        cerr << "O mapa possui mais de 24 ginasios.\n";
        return;
    }
    pontos.push_back('1');
    pontos.push_back('U');
    vector<vll> d(k + 2, vll(k + 2, inf));
    for (int i = 0; i < k + 2; ++i) {
        d[i][i] = 0;
        for (int j = 0; j < k + 2; ++j) {
            if (i == j) continue;
            auto it = dist.at(pontos[i]).find(pontos[j]);
            if (it == dist.at(pontos[i]).end()) {
                cerr << "Distancia ausente entre pontos do mapa.\n";
                return;
            }
            d[i][j] = it->second;
        }
    }

    // A MST depende somente do conjunto restante; guardar apenas seu custo.
    unordered_map<uint32_t,ll> cache_mst;
    auto mst = [&](uint32_t mascara) -> ll {
        auto it = cache_mst.find(mascara);
        if (it != cache_mst.end()) return it->second;
        if (!mascara) return 0;
        vll menor(k, inf);
        int primeiro = __builtin_ctz(mascara);
        menor[primeiro] = 0;
        uint32_t falta = mascara;
        ll custo = 0;
        while (falta) {
            int escolhido = -1;
            for (int i = 0; i < k; ++i)
                if ((falta & (1u << i)) &&
                    (escolhido == -1 || menor[i] < menor[escolhido])) escolhido = i;
            custo += menor[escolhido];
            falta &= ~(1u << escolhido);
            for (int i = 0; i < k; ++i)
                if (falta & (1u << i)) menor[i] = min(menor[i], d[escolhido][i]);
        }
        cache_mst[mascara] = custo;
        return custo;
    };
    auto heuristica = [&](int atual, uint32_t restantes) -> ll {
        if (!restantes) return d[atual][k + 1];
        ll entrada = inf, saida = inf;
        for (int i = 0; i < k; ++i) if (restantes & (1u << i)) {
            entrada = min(entrada, d[atual][i]);
            saida = min(saida, d[i][k + 1]);
        }
        return entrada + mst(restantes) + saida;
    };
    auto chave = [](int atual, uint32_t restantes) -> uint64_t {
        return (static_cast<uint64_t>(restantes) << 5) | static_cast<uint64_t>(atual);
    };
    uint32_t todos = (1u << k) - 1;
    uint64_t inicial = chave(k, todos);
    unordered_map<uint64_t,ll> melhor;
    unordered_map<uint64_t,uint64_t> pai;
    priority_queue<EstadoRotas> fila;
    // Conta a origem uma unica vez; as arestas contam a entrada no destino.
    melhor[inicial] = 1;
    fila.push({1 + heuristica(k, todos), 1, todos, k});
    ll expandidos = 0;
    while (!fila.empty()) {
        EstadoRotas estado = fila.top();
        fila.pop();
        uint64_t id = chave(estado.atual, estado.restantes);
        if (estado.g != melhor.at(id)) continue;
        ++expandidos;
        if (!estado.restantes) {
            vector<char> ordem;
            for (uint64_t cursor = id;; cursor = pai.at(cursor)) {
                ordem.push_back(pontos[cursor & 31u]);
                if (cursor == inicial) break;
            }
            reverse(ordem.begin(), ordem.end());
            cout << "Ordem planejada: ";
            for (char ponto : ordem) cout << ponto << " -> ";
            cout << "U\nCusto da rota (incluindo origem): "
                 << estado.g + d[estado.atual][k + 1]
                 << "\nEstados expandidos: " << expandidos << '\n';
            return;
        }
        for (int vizinho = 0; vizinho < k; ++vizinho) {
            if (!(estado.restantes & (1u << vizinho))) continue;
            uint32_t restantes = estado.restantes & ~(1u << vizinho);
            ll novo_g = estado.g + d[estado.atual][vizinho];
            uint64_t novo_id = chave(vizinho, restantes);
            auto it = melhor.find(novo_id);
            if (it != melhor.end() && novo_g >= it->second) continue;
            melhor[novo_id] = novo_g;
            pai[novo_id] = id;
            fila.push({novo_g + heuristica(vizinho, restantes), novo_g, restantes, vizinho});
        }
    }
    cerr << "Nenhuma rota encontrada.\n";
}

int main() {
    vector<string> mapa;
    string s;

    while (cin >> s) {
        mapa.push_back(s);
    }

    ll n = mapa.size();

    map<char, pair<ll,ll>> m;
    map<char, map<char,ll>> dist;

    forn(i,0,n) {
        forn(j,0,mapa[i].size()) {
            if  (mapa[i][j]!='A' && mapa[i][j]!='M' && mapa[i][j]!='F' && mapa[i][j]!='A' && mapa[i][j]!='.' && mapa[i][j]!='R') {
                m[mapa[i][j]]={i,j};
            }
        }
    }
    for (auto p : m) {
        dj(mapa,m[p.first],dist[p.first]);
    }
    // com todas as menores distâncias de todos os nodulos para todos os nódulos







    astar_rotas(dist);
    return 0;
}