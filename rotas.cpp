#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
using namespace std;
using Int = int32_t;
using Mask = uint32_t;
using Clock = chrono::steady_clock;
constexpr Int INF = numeric_limits<Int>::max()/4;
constexpr Mask EMPTY = numeric_limits<Mask>::max();

Int weight(char c) {
    switch(c) { case 'M': return 200; case 'A': return 30;
        case 'F': return 15; case 'R': return 5; default: return 1; }
}
bool terreno(char c) { return c=='.'||c=='M'||c=='A'||c=='F'||c=='R'; }
Int bit(Mask x) { return __builtin_ctz(x); } // x precisa ser diferente de zero.

struct Mapa {
    vector<string> linhas;
    vector<char> nomes;
    vector<Int> pos;
    vector<vector<Int>> d, pais;
    Int altura=0, largura=0, k=0;
    explicit Mapa(istream& in) {
        string s; while(in>>s) linhas.push_back(s);
        if(linhas.empty()) throw runtime_error("Mapa vazio.");
        altura=static_cast<Int>(linhas.size()); largura=static_cast<Int>(linhas[0].size());
        if(int64_t(altura)*largura*200*26 >= INF)
            throw runtime_error("Mapa excede o limite seguro dos custos int32_t.");
        array<Int,256> pontos; pontos.fill(-1);
        for(Int r=0;r<altura;++r) {
            if(static_cast<Int>(linhas[r].size())!=largura) throw runtime_error("Mapa nao retangular.");
            for(Int c=0;c<largura;++c) if(!terreno(linhas[r][c])) {
                auto ch=static_cast<unsigned char>(linhas[r][c]);
                if(pontos[ch]!=-1) throw runtime_error("Ponto repetido no mapa.");
                pontos[ch]=r*largura+c;
            }
        }
        if(pontos['1']<0||pontos['U']<0) throw runtime_error("Faltam origem 1 ou destino U.");
        for(Int c=0;c<256;++c) if(pontos[c]>=0&&c!='1'&&c!='U') {
            nomes.push_back(static_cast<char>(c)); pos.push_back(pontos[c]);
        }
        k=static_cast<Int>(nomes.size());
        if(k>24) throw runtime_error("Sao permitidos no maximo 24 ginasios.");
        nomes.push_back('1'); pos.push_back(pontos['1']);
        nomes.push_back('U'); pos.push_back(pontos['U']);
        d.assign(k+2,vector<Int>(k+2,INF)); pais.resize(k+2);
    }
    // Dijkstra preserva o custo de entrada das celulas do codigo original.
    // U nunca pode ser ponto de passagem: a jornada termina ao chegar la.
    void dj(Int origem) {
        vector<Int> dist(altura*largura,INF), pai(altura*largura,-1);
        using Item=pair<Int,Int>;
        priority_queue<Item,vector<Item>,greater<Item>> fila;
        dist[pos[origem]]=0; fila.emplace(0,pos[origem]);
        while(!fila.empty()) {
            auto [g,p]=fila.top(); fila.pop(); if(g!=dist[p])continue;
            if(p==pos[k+1]&&origem!=k+1)continue;
            Int r=p/largura,c=p%largura;
            array<Int,4> viz={r>0?p-largura:-1,r+1<altura?p+largura:-1,
                              c>0?p-1:-1,c+1<largura?p+1:-1};
            for(Int q:viz)if(q>=0) {
                Int ng=g+weight(linhas[q/largura][q%largura]);
                if(ng<dist[q]) {dist[q]=ng;pai[q]=p;fila.emplace(ng,q);}
            }
        }
        for(Int j=0;j<k+2;++j)d[origem][j]=dist[pos[j]];
        pais[origem]=move(pai);
    }
    void preparar() {
        for(Int i=0;i<k+2;++i)dj(i);
        for(auto& row:d)for(Int x:row)if(x==INF)throw runtime_error("Pontos desconectados.");
    }
    vector<Int> caminho(const vector<Int>& ordem) const {
        vector<Int> resultado={pos[k]};
        for(size_t i=1;i<ordem.size();++i) {
            Int a=ordem[i-1],b=ordem[i]; vector<Int> trecho;
            for(Int p=pos[b];p!=pos[a];p=pais[a][p]) {
                if(p<0)throw runtime_error("Falha ao reconstruir caminho.");
                trecho.push_back(p);
            }
            reverse(trecho.begin(),trecho.end());
            resultado.insert(resultado.end(),trecho.begin(),trecho.end());
        }
        return resultado;
    }
};

struct Heuristica {
    const vector<vector<Int>>& d;
    Int k;
    vector<Int> custo_mst;
    uint64_t calculadas=0,consultas=0;
    explicit Heuristica(const Mapa& m):d(m.d),k(m.k),custo_mst(size_t(1)<<k,-1) {
        custo_mst[0]=0;
    }
    // Acesso direto O(1). A primeira consulta calcula Prim O(s^2);
    // as seguintes retornam o valor pronto, sem hashing nem recomputacao.
    Int mst(Mask mask) {
        ++consultas;
        Int& salvo=custo_mst[mask]; if(salvo>=0)return salvo;
        ++calculadas;
        array<Int,24> melhor; melhor.fill(INF);
        melhor[bit(mask)]=0; Mask falta=mask; Int custo=0;
        while(falta) {
            Int escolhido=-1;
            for(Mask bits=falta;bits;bits&=bits-1) {
                Int i=bit(bits); if(escolhido<0||melhor[i]<melhor[escolhido])escolhido=i;
            }
            custo+=melhor[escolhido]; falta&=~(Mask(1)<<escolhido);
            for(Mask bits=falta;bits;bits&=bits-1) {
                Int i=bit(bits);
                melhor[i]=min(melhor[i],min(d[escolhido][i],d[i][escolhido]));
            }
        }
        return salvo=custo;
    }
    Int operator()(Int atual,Mask restantes) {
        if(!restantes)return d[atual][k+1];
        Int entrada=INF,saida=INF;
        for(Mask bits=restantes;bits;bits&=bits-1) {
            Int i=bit(bits);entrada=min(entrada,d[atual][i]);saida=min(saida,d[i][k+1]);
        }
        return entrada+mst(restantes)+saida;
    }
};

// Tabela hash compacta: a chave e apenas (mascara << 5) | ponto atual.
// Os custos usam Int; a mascara e uma chave sem sinal, nao um custo.
struct Melhores {
    vector<Mask> keys;
    vector<Int> valores;
    vector<uint8_t> pais;
    size_t usados=0;
    explicit Melhores(size_t cap=1<<16):keys(cap,EMPTY),valores(cap),pais(cap){}
    size_t slot(Mask key)const {
        size_t i=(uint64_t(key)*11400714819323198485ull)>>(64-__builtin_ctzll(keys.size()));
        while(keys[i]!=EMPTY&&keys[i]!=key)i=(i+1)&(keys.size()-1);
        return i;
    }
    void crescer() {
        Melhores outra(keys.size()*2);
        for(size_t i=0;i<keys.size();++i)if(keys[i]!=EMPTY) {
            size_t j=outra.slot(keys[i]);outra.keys[j]=keys[i];outra.valores[j]=valores[i];outra.pais[j]=pais[i];
        }
        outra.usados=usados; *this=move(outra);
    }
    Int get(Mask key)const {size_t i=slot(key);return keys[i]==EMPTY?INF:valores[i];}
    bool atualizar(Mask key,Int custo,Int pai) {
        size_t i=slot(key);
        if(keys[i]!=EMPTY) {
            if(valores[i]<=custo)return false;
            valores[i]=custo;pais[i]=static_cast<uint8_t>(pai);return true;
        }
        if((usados+1)*10>keys.size()*7){crescer();i=slot(key);}
        keys[i]=key;valores[i]=custo;pais[i]=static_cast<uint8_t>(pai);++usados;return true;
    }
    Int pai(Mask key)const{return pais[slot(key)];}
};
Mask chave(Int atual,Mask restantes){return (restantes<<5)|static_cast<Mask>(atual);}
struct Estado {
    Int f,g; Mask id;
    bool operator<(const Estado& b)const {
        if(f!=b.f)return f>b.f;
        if(g!=b.g)return g<b.g;
        return id>b.id;
    }
};
Int custo_ordem(const vector<Int>& o,const Mapa& m) {
    Int total=1;for(size_t i=1;i<o.size();++i)total+=m.d[o[i-1]][o[i]];return total;
}
// Uma rota viavel fornece apenas um limite superior; a prova continua sendo A*.
vector<Int> limite_superior(const Mapa& m) {
    mt19937 rng(1771); vector<Int> melhor;
    Int custo=INF;
    for(Int tentativa=0;tentativa<200;++tentativa) {
        vector<Int> ordem(m.k);iota(ordem.begin(),ordem.end(),0);
        if(tentativa)shuffle(ordem.begin(),ordem.end(),rng);
        else {
            Mask falta=(Mask(1)<<m.k)-1;Int atual=m.k;
            for(Int j=0;j<m.k;++j) {
                Int v=-1;for(Int i=0;i<m.k;++i)if((falta>>i&1)&&(v<0||m.d[atual][i]<m.d[atual][v]))v=i;
                ordem[j]=v;falta&=~(Mask(1)<<v);atual=v;
            }
        }
        ordem.insert(ordem.begin(),m.k);ordem.push_back(m.k+1);
        bool mudou=true;
        while(mudou) {
            mudou=false;
            for(Int a=1;a<m.k;++a)for(Int b=a+1;b<=m.k;++b) {
                // Distancias simetricas: todos os pontos importantes custam 1.
                Int antes=m.d[ordem[a-1]][ordem[a]]+m.d[ordem[b]][ordem[b+1]];
                Int depois=m.d[ordem[a-1]][ordem[b]]+m.d[ordem[a]][ordem[b+1]];
                if(depois<antes){reverse(ordem.begin()+a,ordem.begin()+b+1);mudou=true;}
            }
        }
        Int c=custo_ordem(ordem,m);if(c<custo){custo=c;melhor=ordem;}
    }
    return melhor;
}
struct Resultado {
    vector<Int> ordem;
    Int custo=0;
    bool otimo=false;
    uint64_t expandidos=0,gerados=0;
    size_t estados=0,pico_fila=0;
};
Resultado astar_rotas(const Mapa& m,Heuristica& h,double limite) {
    Resultado res;res.ordem=limite_superior(m);res.custo=custo_ordem(res.ordem,m);
    cerr<<"Limite superior inicial: "<<res.custo<<"\n";
    Mask todos=(Mask(1)<<m.k)-1,inicial=chave(m.k,todos);
    Melhores best;priority_queue<Estado> fila;
    best.atualizar(inicial,1,m.k);fila.push({1+h(m.k,todos),1,inicial});
    auto inicio=Clock::now(),ultimo=inicio;
    while(!fila.empty()) {
        Estado e=fila.top();fila.pop();
        if(e.g!=best.get(e.id))continue;
        if(e.f>=res.custo){res.otimo=true;break;}
        Int atual=static_cast<Int>(e.id&31u);Mask restantes=e.id>>5;
        ++res.expandidos;
        if(!restantes) {
            res.custo=e.g+m.d[atual][m.k+1];
            vector<Int> ordem={m.k+1};Mask cursor=e.id;
            while(cursor!=inicial) {
                Int v=static_cast<Int>(cursor&31u);ordem.push_back(v);
                Int p=best.pai(cursor);cursor=chave(p,(cursor>>5)|(Mask(1)<<v));
            }
            ordem.push_back(m.k);reverse(ordem.begin(),ordem.end());res.ordem=move(ordem);
            res.otimo=true;break;
        }
        for(Mask bits=restantes;bits;bits&=bits-1) {
            Int v=bit(bits);Mask next=restantes&~(Mask(1)<<v),id=chave(v,next);
            Int g=e.g+m.d[atual][v];
            if(g>=res.custo||g>=best.get(id))continue;
            Int f=g+h(v,next);if(f>=res.custo)continue;
            if(best.atualizar(id,g,atual)){fila.push({f,g,id});++res.gerados;}
        }
        res.pico_fila=max(res.pico_fila,fila.size());
        if((res.expandidos&16383u)==0) {
            auto agora=Clock::now();double segundos=chrono::duration<double>(agora-inicio).count();
            if(chrono::duration<double>(agora-ultimo).count()>=5) {
                cerr<<fixed<<setprecision(1)<<segundos<<" s; expandidos="<<res.expandidos
                    <<"; estados="<<best.usados<<"; MSTs="<<h.calculadas
                    <<"; menor f="<<(fila.empty()?res.custo:fila.top().f)<<"; limite="<<res.custo<<'\n';ultimo=agora;
            }
            if(limite>0&&segundos>=limite){cerr<<"Limite de tempo: resultado ainda sem prova de otimalidade.\n";break;}
        }
    }
    if(fila.empty())res.otimo=true;
    res.estados=best.usados;
    return res;
}
int main(int argc,char** argv) {
    try {
        string arquivo,prefixo="resultado_rotas";double limite=0;
        bool so_mst=false;
        for(Int i=1;i<argc;++i) {
            string arg=argv[i];
            if(arg=="--tempo"&&i+1<argc)limite=stod(argv[++i]);
            else if(arg=="--saida"&&i+1<argc)prefixo=argv[++i];
            else if(arg=="--precalcular-mst")so_mst=true;
            else if(arg=="--help") {
                cout<<"Uso: rotas [mapa.txt] [--tempo segundos] [--saida prefixo] [--precalcular-mst]\n";return 0;
            } else if(!arg.empty()&&arg[0]=='-')throw runtime_error("Opcao desconhecida: "+arg);
            else arquivo=arg;
        }
        ifstream in;if(!arquivo.empty()){in.open(arquivo);if(!in)throw runtime_error("Nao foi possivel abrir o mapa.");}
        auto inicio=Clock::now();Mapa m(arquivo.empty()?cin:in);m.preparar();
        double dj_s=chrono::duration<double>(Clock::now()-inicio).count();
        Heuristica h(m);
        cerr<<"Mapa: "<<m.largura<<" x "<<m.altura<<"; ginasios: "<<m.k
            <<"; Dijkstra: "<<dj_s<<" s; cache MST: "<<(static_cast<double>(h.custo_mst.size())*sizeof(Int)/1048576.0)<<" MiB\n";
        if(so_mst) {
            auto t=Clock::now();for(Mask mask=1;mask<h.custo_mst.size();++mask)h.mst(mask);
            cerr<<"Pre-calculo completo: "<<chrono::duration<double>(Clock::now()-t).count()<<" s\n";
        }
        auto t=Clock::now();Resultado res=astar_rotas(m,h,limite);
        double astar_s=chrono::duration<double>(Clock::now()-t).count();
        auto path=m.caminho(res.ordem);Int custo_real=0;Mask vistos=0;
        vector<char> ordem_real;
        ofstream csv(prefixo+"_caminho.csv");if(!csv)throw runtime_error("Nao foi possivel criar a saida.");
        csv<<"passo,linha,coluna,celula,custo_acumulado\n";
        for(size_t i=0;i<path.size();++i) {
            Int p=path[i],r=p/m.largura,c=p%m.largura;char cel=m.linhas[r][c];
            custo_real+=weight(cel);
            if(i&&abs(p/m.largura-path[i-1]/m.largura)+abs(p%m.largura-path[i-1]%m.largura)!=1)
                throw runtime_error("Passo invalido na reconstrucao.");
            if(cel=='U'&&i+1!=path.size())throw runtime_error("Destino atravessado antes do fim.");
            for(Int j=0;j<m.k;++j)if(p==m.pos[j]&&!(vistos>>j&1)){vistos|=Mask(1)<<j;ordem_real.push_back(cel);}
            csv<<i<<','<<r<<','<<c<<','<<cel<<','<<custo_real<<'\n';
        }
        if(custo_real!=res.custo||vistos!=((Mask(1)<<m.k)-1))throw runtime_error("Validacao do caminho falhou.");
        ofstream txt(prefixo+".txt");
        auto relatar=[&](ostream& out) {
            out<<"Ordem no grafo: ";for(Int v:res.ordem)out<<m.nomes[v]<<' ';
            out<<"\nPrimeira visita real: ";for(char c:ordem_real)out<<c<<' ';
            out<<"\nCusto da rota (inclui origem): "<<res.custo
                <<"\nOtimo comprovado pelo A*: "<<(res.otimo?"sim":"nao")
                <<"\nEstados expandidos: "<<res.expandidos<<"\nEstados registrados: "<<res.estados
                <<"\nPico da fila: "<<res.pico_fila<<"\nMSTs calculadas: "<<h.calculadas
                <<"\nConsultas MST: "<<h.consultas<<"\nTempo Dijkstra (s): "<<dj_s
                <<"\nTempo A* (s): "<<astar_s<<"\nPassos no mapa: "<<path.size()-1<<'\n';
        };
        relatar(cout);relatar(txt);return res.otimo?0:2;
    } catch(const exception& e){cerr<<"Erro: "<<e.what()<<'\n';return 1;}
}
