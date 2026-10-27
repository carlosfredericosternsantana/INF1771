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
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;
using Int = int32_t;
using Mask = uint32_t;
constexpr Int NP=5, NG=24, ENERGIA=6, PARTICIPACOES=29, ESCALA=10000;

// Poder em ponto fixo: 15000 representa 1.5000, sem erro binario na soma.
struct Pokemon {string nome; Int poder;};
struct Ginasio {char nome; Int dificuldade;};
const array<Pokemon,NP> pokemons={{{"Pikachu",15000},{"Bulbassauro",14000},
    {"Rattata",13000},{"Caterpie",12000},{"Weedle",11000}}};
const array<Ginasio,NG> ginasios={{{'2',35},{'3',40},{'4',45},{'5',50},
    {'6',55},{'7',60},{'8',65},{'9',70},{'B',75},{'C',80},{'D',85},{'E',90},
    {'G',95},{'H',100},{'I',110},{'J',120},{'K',130},{'L',140},{'N',150},
    {'O',155},{'P',160},{'Q',165},{'S',170},{'T',180}}};

struct Individuo {
    array<Mask,NG> equipes{};
    long double custo=0;
};
array<Int,32> poder{};
array<Int,NP> usos(const Individuo& x) {
    array<Int,NP> n{};
    for(Mask equipe:x.equipes)for(Int p=0;p<NP;++p)if(equipe&(1u<<p))++n[p];
    return n;
}
bool valido(const Individuo& x) {
    for(Mask m:x.equipes)if(m==0||m>=32)return false;
    auto n=usos(x);Int soma=0;bool acordado=false;
    for(Int v:n){if(v>ENERGIA)return false;soma+=v;acordado|=v<ENERGIA;}
    return soma==PARTICIPACOES&&acordado;
}
void avaliar(Individuo& x) {
    // O argumento de troca de Pokemons.md resolve exatamente o pareamento
    // para estas equipes: dificuldades e poderes ambos em ordem crescente.
    sort(x.equipes.begin(),x.equipes.end(),[](Mask a,Mask b){
        return poder[a]!=poder[b]?poder[a]<poder[b]:a<b;
    });
    x.custo=0;
    for(Int i=0;i<NG;++i)
        x.custo+=static_cast<long double>(ginasios[i].dificuldade)*ESCALA/poder[x.equipes[i]];
    // Nao arredondar parcelas durante a selecao: isso altera o objetivo.
    // Poderes tem quatro casas exatas; tempos sao apresentados com quatro.
}
Int sorteio(mt19937& rng,Int limite) {
    return uniform_int_distribution<Int>(0,limite-1)(rng);
}

void reparar(Individuo& x,mt19937& rng) {
    for(Mask& m:x.equipes){m&=31u;if(!m)m=1u<<sorteio(rng,NP);}
    auto n=usos(x);
    // Elimina sobrecarga sem deixar qualquer ginasio sem participante.
    for(Int p=0;p<NP;++p)while(n[p]>ENERGIA) {
        vector<Int> extras,singles;
        for(Int i=0;i<NG;++i)if(x.equipes[i]&(1u<<p)) {
            if(__builtin_popcount(x.equipes[i])>1)extras.push_back(i);
            else singles.push_back(i);
        }
        if(!extras.empty()) {
            Int i=extras[sorteio(rng,static_cast<Int>(extras.size()))];
            x.equipes[i]&=~(1u<<p);--n[p];
        } else {
            vector<Int> livres;for(Int q=0;q<NP;++q)if(n[q]<ENERGIA)livres.push_back(q);
            if(livres.empty()||singles.empty())throw runtime_error("Falha no reparo de energia.");
            Int i=singles[sorteio(rng,static_cast<Int>(singles.size()))];
            Int q=livres[sorteio(rng,static_cast<Int>(livres.size()))];
            x.equipes[i]=1u<<q;--n[p];++n[q];
        }
    }
    Int total=accumulate(n.begin(),n.end(),Int(0));
    while(total>PARTICIPACOES) {
        vector<pair<Int,Int>> opcoes;
        for(Int i=0;i<NG;++i)if(__builtin_popcount(x.equipes[i])>1)
            for(Int p=0;p<NP;++p)if(x.equipes[i]&(1u<<p))opcoes.emplace_back(i,p);
        auto [i,p]=opcoes[sorteio(rng,static_cast<Int>(opcoes.size()))];
        x.equipes[i]&=~(1u<<p);--n[p];--total;
    }
    while(total<PARTICIPACOES) {
        vector<pair<Int,Int>> opcoes;
        for(Int i=0;i<NG;++i)for(Int p=0;p<NP;++p)
            if(n[p]<ENERGIA&&!(x.equipes[i]&(1u<<p)))opcoes.emplace_back(i,p);
        if(opcoes.empty())throw runtime_error("Nao foi possivel completar as participacoes.");
        auto [i,p]=opcoes[sorteio(rng,static_cast<Int>(opcoes.size()))];
        x.equipes[i]|=1u<<p;++n[p];++total;
    }
    if(!valido(x))throw runtime_error("Individuo invalido apos reparo.");
    avaliar(x);
}
Individuo aleatorio(mt19937& rng) {
    Individuo x;
    for(Mask& m:x.equipes)m=1u<<sorteio(rng,NP);
    reparar(x,rng);return x;
}
void mutar(Individuo& x,mt19937& rng) {
    Int tipo=sorteio(rng,3),a=sorteio(rng,NG),b=sorteio(rng,NG);
    Int p=sorteio(rng,NP),q=sorteio(rng,NP);
    if(tipo==0) {
        // Substituicao de participante.
        if((x.equipes[a]&(1u<<p))&&!(x.equipes[a]&(1u<<q)))
            x.equipes[a]=(x.equipes[a]&~(1u<<p))|(1u<<q);
    } else if(tipo==1) {
        // Transfere um reforco para outra equipe.
        if(__builtin_popcount(x.equipes[a])>1&&(x.equipes[a]&(1u<<p))&&!(x.equipes[b]&(1u<<p))) {
            x.equipes[a]&=~(1u<<p);x.equipes[b]|=1u<<p;
        }
    } else {
        // Troca participantes de duas equipes, preservando o total por Pokemon.
        if((x.equipes[a]&(1u<<p))&&!(x.equipes[a]&(1u<<q))&&
           (x.equipes[b]&(1u<<q))&&!(x.equipes[b]&(1u<<p))) {
            x.equipes[a]^=(1u<<p)|(1u<<q);x.equipes[b]^=(1u<<p)|(1u<<q);
        }
    }
}
struct Config {
    Int populacao=120,geracoes=600,execucoes=30;
    uint32_t seed=1771;
    string saida="resultado_pokemons";
};
Individuo genetico(const Config& c,uint32_t seed,ofstream& evolucao,Int rodada) {
    mt19937 rng(seed);vector<Individuo> pop(c.populacao);
    for(auto& x:pop)x=aleatorio(rng);
    auto ordenar=[&](){sort(pop.begin(),pop.end(),[](const auto& a,const auto& b){return a.custo<b.custo;});};
    ordenar();
    auto torneio=[&]()->const Individuo& {
        Int vencedor=sorteio(rng,c.populacao);
        for(Int i=1;i<3;++i){Int j=sorteio(rng,c.populacao);if(pop[j].custo<pop[vencedor].custo)vencedor=j;}
        return pop[vencedor];
    };
    evolucao<<rodada<<",0,"<<pop[0].custo<<'\n';
    for(Int geracao=1;geracao<=c.geracoes;++geracao) {
        vector<Individuo> nova;nova.reserve(c.populacao);
        nova.push_back(pop[0]);nova.push_back(pop[1]); // Elitismo.
        while(static_cast<Int>(nova.size())<c.populacao) {
            // Imigrantes reduzem a perda de diversidade causada pela ordenacao.
            if(sorteio(rng,100)<5){nova.push_back(aleatorio(rng));continue;}
            Individuo filho=torneio();const Individuo& outro=torneio();
            if(sorteio(rng,100)<80)
                for(Int i=0;i<NG;++i)if(sorteio(rng,2))filho.equipes[i]=outro.equipes[i];
            if(sorteio(rng,100)<35) {
                Int vezes=1+sorteio(rng,3);for(Int i=0;i<vezes;++i)mutar(filho,rng);
            }
            reparar(filho,rng);nova.push_back(filho);
        }
        pop=move(nova);ordenar();evolucao<<rodada<<','<<geracao<<','<<pop[0].custo<<'\n';
    }
    return pop[0];
}
string nomes(Mask equipe,const string& sep=" + ") {
    string s;for(Int p=0;p<NP;++p)if(equipe&(1u<<p)) {
        if(!s.empty())s+=sep;
        s+=pokemons[p].nome;
    }
    return s;
}
// Exporta os 24 pareamentos como codigo C++ explicito, inclusive as equipes
// com um unico Pokemon que foram associadas por ordenacao de poder.
void exportar_codigo(const Individuo& melhor,const string& arquivo) {
    ofstream out(arquivo);
    if(!out)throw runtime_error("Nao foi possivel salvar o pareamento em C++.");
    out<<"#pragma once\n#include <array>\n#include <cstdint>\n\n"
        <<"// Gerado por pokemon.cpp. Valores fixos desta execucao do algoritmo genetico.\n"
        <<"// true = participa; false = nao participa. Todas as 24 batalhas estao listadas.\n"
        <<"// O pareamento e otimo para estas equipes; nao e prova de otimo global.\n"
        <<"namespace pareamento_calculado {\n"
        <<"struct Pokemon { const char* nome; std::int32_t poder_10000; std::int32_t energia_inicial; };\n"
        <<"inline constexpr std::array<Pokemon, 5> POKEMONS = {{\n";
    for(const auto& p:pokemons)
        out<<"    {\""<<p.nome<<"\", "<<p.poder<<", "<<ENERGIA<<"},\n";
    out<<"}};\n\n"
        <<"struct Participantes {\n"
        <<"    bool pikachu;\n    bool bulbassauro;\n    bool rattata;\n    bool caterpie;\n    bool weedle;\n};\n"
        <<"struct Batalha {\n    char ginasio;\n    std::int32_t dificuldade;\n"
        <<"    Participantes participantes;\n    std::int32_t poder_10000;\n"
        <<"    long double tempo_exibido; // Arredondado a quatro casas apenas para exibicao.\n};\n\n"
        <<"// Participantes na ordem: Pikachu, Bulbassauro, Rattata, Caterpie, Weedle.\n"
        <<"inline constexpr std::array<Batalha, 24> BATALHAS = {{\n";
    out<<fixed<<setprecision(4);
    for(Int i=0;i<NG;++i) {
        Mask m=melhor.equipes[i];
        out<<"    {'"<<ginasios[i].nome<<"', "<<ginasios[i].dificuldade<<", {";
        for(Int p=0;p<NP;++p) {
            if(p)out<<", ";
            out<<((m&(1u<<p))?"true":"false");
        }
        out<<"}, "<<poder[m]<<", "
            <<static_cast<long double>(ginasios[i].dificuldade)*ESCALA/poder[m]
            <<"L}, // "<<nomes(m)<<'\n';
    }
    out<<"}};\n\n"
        <<"// Calcule com dificuldade * 10000.0L / poder_10000 para manter precisao\n"
        <<"// interna; nao some tempo_exibido para obter o total exato.\n"
        <<"inline constexpr std::array<std::int32_t, 5> ENERGIA_FINAL = {";
    auto n=usos(melhor);
    for(Int p=0;p<NP;++p){if(p)out<<", ";out<<ENERGIA-n[p];}
    out<<"};\n} // namespace pareamento_calculado\n";
    if(!out)throw runtime_error("Falha ao gravar o pareamento em C++.");
}
int main(int argc,char** argv) {
    try {
        Config c;
        for(Int i=1;i<argc;++i) {
            string a=argv[i];
            if(a=="--help") {
                cout<<"Uso: pokemon [--populacao 120] [--geracoes 600] [--execucoes 30] [--seed 1771] [--saida resultado_pokemons]\n";
                return 0;
            }
            if(i+1>=argc)throw runtime_error("Falta valor para "+a);
            string v=argv[++i];
            if(a=="--populacao")c.populacao=stoi(v);
            else if(a=="--geracoes")c.geracoes=stoi(v);
            else if(a=="--execucoes")c.execucoes=stoi(v);
            else if(a=="--seed") {
                auto n=stoull(v);if(n>UINT32_MAX||v[0]=='-')throw runtime_error("Seed invalida.");
                c.seed=static_cast<uint32_t>(n);
            }
            else if(a=="--saida")c.saida=v;
            else throw runtime_error("Opcao desconhecida: "+a);
        }
        if(c.populacao<4||c.geracoes<0||c.execucoes<1)
            throw runtime_error("Populacao >= 4, geracoes >= 0 e execucoes >= 1.");
        for(Mask m=1;m<32;++m)for(Int p=0;p<NP;++p)if(m&(1u<<p))poder[m]+=pokemons[p].poder;
        for(Int i=1;i<NG;++i)if(ginasios[i].dificuldade<ginasios[i-1].dificuldade)
            throw runtime_error("Ordene os ginasios por dificuldade crescente.");
        auto inicio=chrono::steady_clock::now();
        ofstream evolucao(c.saida+"_evolucao.csv"),execucoes(c.saida+"_execucoes.csv");
        if(!evolucao||!execucoes)throw runtime_error("Nao foi possivel criar os arquivos de saida.");
        evolucao<<fixed<<setprecision(4)<<"execucao,geracao,melhor_custo\n";
        execucoes<<fixed<<setprecision(4)<<"execucao,seed,custo\n";
        Individuo melhor;melhor.custo=numeric_limits<long double>::infinity();
        vector<long double> custos;
        for(Int r=0;r<c.execucoes;++r) {
            uint32_t seed=c.seed+static_cast<uint32_t>(r);
            auto x=genetico(c,seed,evolucao,r+1);
            if(!valido(x))throw runtime_error("Validacao final falhou.");
            custos.push_back(x.custo);if(x.custo<melhor.custo)melhor=x;
            execucoes<<r+1<<','<<seed<<','<<x.custo<<'\n';
            cerr<<fixed<<setprecision(4)<<"Execucao "<<r+1<<'/'<<c.execucoes<<": "<<x.custo<<'\n';
        }
        long double media=accumulate(custos.begin(),custos.end(),0.0L)/c.execucoes,variancia=0;
        for(auto v:custos)variancia+=(v-media)*(v-media);
        long double desvio=sqrt(variancia/c.execucoes); // Desvio populacional das execucoes.
        auto n=usos(melhor);
        ofstream txt(c.saida+".txt"),csv(c.saida+"_equipes.csv");
        if(!txt||!csv)throw runtime_error("Nao foi possivel salvar o resultado.");
        csv<<fixed<<setprecision(4)<<"ginasio,dificuldade,equipe,mascara,poder,tempo\n";
        for(Int i=0;i<NG;++i) {
            Mask m=melhor.equipes[i];long double tempo=static_cast<long double>(ginasios[i].dificuldade)*ESCALA/poder[m];
            csv<<ginasios[i].nome<<','<<ginasios[i].dificuldade<<','<<nomes(m," + ")<<','<<m<<','
                <<static_cast<long double>(poder[m])/ESCALA<<','<<tempo<<'\n';
        }
        auto relatar=[&](ostream& out) {
            out<<fixed<<setprecision(4)<<"Algoritmo genetico: melhor solucao encontrada; sem garantia de otimo global.\n"
                <<"Pareamento por ordenacao: otimo para o conjunto de equipes encontrado.\n"
                <<"Poderes em ponto fixo com 4 casas; tempos arredondados somente na saida.\n\n";
            for(Int i=0;i<NG;++i) {
                Mask m=melhor.equipes[i];out<<"Ginasio "<<ginasios[i].nome<<" | dificuldade "<<ginasios[i].dificuldade
                    <<" | "<<nomes(m)<<" | poder "<<static_cast<long double>(poder[m])/ESCALA
                    <<" | tempo "<<static_cast<long double>(ginasios[i].dificuldade)*ESCALA/poder[m]<<'\n';
            }
            out<<"\nEnergia final:\n";for(Int p=0;p<NP;++p)
                out<<pokemons[p].nome<<": "<<ENERGIA-n[p]<<" ("<<n[p]<<" participacoes)\n";
            out<<"\nParticipacoes: "<<accumulate(n.begin(),n.end(),Int(0))
                <<"\nCusto total das batalhas: "<<melhor.custo<<"\nExecucoes: "<<c.execucoes
                <<"\nPopulacao: "<<c.populacao<<"\nGeracoes: "<<c.geracoes<<"\nSeed inicial: "<<c.seed
                <<"\nMedia: "<<media<<"\nDesvio-padrao populacional: "<<desvio
                <<"\nTempo total (s): "<<chrono::duration<double>(chrono::steady_clock::now()-inicio).count()<<'\n';
        };
        exportar_codigo(melhor,c.saida+".hpp");
        relatar(cout);relatar(txt);return 0;
    } catch(const exception& e){cerr<<"Erro: "<<e.what()<<'\n';return 1;}
}
