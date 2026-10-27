#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;
namespace fs=std::filesystem;
using Int=int32_t;
const array<string,5> NOMES={"Pikachu","Bulbassauro","Rattata","Caterpie","Weedle"};
const array<Int,5> PODERES={15000,14000,13000,12000,11000};
struct Equipe {Int dificuldade=0;uint32_t participantes=0;};
struct Ponto {char nome;Int custo_rota;};
vector<string> campos(string linha) {
    if(!linha.empty()&&linha.back()=='\r')linha.pop_back();
    vector<string> resultado;istringstream in(linha);string s;
    while(getline(in,s,','))resultado.push_back(s);
    return resultado;
}
Int inteiro(const string& s) {
    size_t lidos=0;long long n=stoll(s,&lidos);
    if(lidos!=s.size()||n<0||n>numeric_limits<Int>::max())throw runtime_error("Valor invalido nos resultados.");
    return static_cast<Int>(n);
}
Int poder(const Equipe& e) {
    Int total=0;for(Int p=0;p<5;++p)if(e.participantes&(1u<<p))total+=PODERES[p];return total;
}
string participantes(const Equipe& e) {
    string nomes;
    for(Int p=0;p<5;++p)if(e.participantes&(1u<<p)) {
        if(!nomes.empty())nomes+=" + ";
        nomes+=NOMES[p];
    }
    return nomes;
}
int main(int argc,char** argv) {
    try {
        fs::path pasta=argc>1?fs::path(argv[1]):fs::absolute(argv[0]).parent_path();
        array<Equipe,256> equipes{};array<Int,5> usos{},energia={6,6,6,6,6};
        set<char> ginasios;
        ifstream pokemon(pasta/"resultado_pokemons_equipes.csv");string linha;
        if(!pokemon)throw runtime_error("Execute pokemon.cpp primeiro: falta resultado_pokemons_equipes.csv.");
        getline(pokemon,linha);
        while(getline(pokemon,linha)) {
            if(linha.empty()||linha=="\r")continue;
            auto v=campos(linha);
            if(v.size()!=6||v[0].size()!=1)throw runtime_error("Resultado dos Pokemon invalido.");
            char g=v[0][0];Int m=inteiro(v[3]),d=inteiro(v[1]);
            if(m==0||m>=32||d==0||g=='1'||g=='U'||!ginasios.insert(g).second)
                throw runtime_error("Equipe ou ginasio invalido.");
            auto& e=equipes[static_cast<unsigned char>(g)];e={d,static_cast<uint32_t>(m)};
            long double informado=stold(v[4]);
            if(!isfinite(informado)||fabsl(informado-static_cast<long double>(poder(e))/10000)>0.00001L)
                throw runtime_error("Poderes diferentes dos resultados calculados.");
            for(Int p=0;p<5;++p)if(e.participantes&(1u<<p))++usos[p];
        }
        Int total_usos=0;
        for(Int n:usos){if(n>6)throw runtime_error("Planejamento excede a energia disponivel.");total_usos+=n;}
        if(ginasios.empty()||total_usos>29)throw runtime_error("Planejamento sem energia final disponivel.");
        ifstream rotas(pasta/"resultado_rotas_caminho.csv");
        if(!rotas)throw runtime_error("Execute rotas.cpp primeiro: falta resultado_rotas_caminho.csv.");
        getline(rotas,linha);vector<Ponto> caminho;set<char> vistos;
        Int passo=0,anterior_r=0,anterior_c=0,anterior_custo=0;char ultima=0;
        while(getline(rotas,linha)) {
            if(linha.empty()||linha=="\r")continue;
            auto v=campos(linha);
            if(v.size()!=5||v[3].size()!=1)throw runtime_error("Resultado da rota invalido.");
            Int r=inteiro(v[1]),c=inteiro(v[2]),custo=inteiro(v[4]);char g=v[3][0];
            Int peso=g=='M'?200:g=='A'?30:g=='F'?15:g=='R'?5:1;
            if(inteiro(v[0])!=passo||custo-anterior_custo!=peso||ultima=='U')
                throw runtime_error("Caminho ou custo inconsistente.");
            if(passo==0) {
                if(g!='1')throw runtime_error("A rota deve comecar em 1.");
                caminho.push_back({g,custo});
            } else if(abs(r-anterior_r)+abs(c-anterior_c)!=1)
                throw runtime_error("Movimento invalido no caminho calculado.");
            if(ginasios.count(g)&&vistos.insert(g).second)caminho.push_back({g,custo});
            if(g=='U')caminho.push_back({g,custo});
            anterior_r=r;anterior_c=c;anterior_custo=custo;ultima=g;++passo;
        }
        if(ultima!='U'||vistos!=ginasios)throw runtime_error("A rota nao visita todos os ginasios antes de U.");
        cout<<"POKEMON DE CADA GINASIO (na ordem do caminho)\n";
        for(const auto& ponto:caminho)if(ginasios.count(ponto.nome)) {
            const auto& e=equipes[static_cast<unsigned char>(ponto.nome)];
            cout<<"  "<<ponto.nome<<" -> "<<participantes(e)<<'\n';
        }
        cout<<fixed<<setprecision(4)<<"\nCaminho: 1\n";
        string percorrido="1",enter;long double custo_batalhas=0;
        for(size_t i=1;i<caminho.size();++i) {
            cout<<"\nPressione Enter para continuar..."<<flush;
            if(!getline(cin,enter))return 0;
            const auto& ponto=caminho[i];percorrido+=" -> "+string(1,ponto.nome);
            cout<<"\nCaminho: "<<percorrido<<'\n';
            if(ponto.nome=='U')cout<<"Destino U. Jornada concluida.\n";
            else {
                const auto& e=equipes[static_cast<unsigned char>(ponto.nome)];
                cout<<"Ginasio "<<ponto.nome<<" | Pokemon: "<<participantes(e);
                for(Int p=0;p<5;++p)if(e.participantes&(1u<<p))--energia[p];
                long double tempo=static_cast<long double>(e.dificuldade)*10000/poder(e);
                custo_batalhas+=tempo;
                cout<<"\nPoder: "<<static_cast<long double>(poder(e))/10000<<" | Tempo da batalha: "<<tempo<<" min\n";
            }
            cout<<"Energia: ";
            for(Int p=0;p<5;++p)cout<<(p?" | ":"")<<NOMES[p]<<' '<<energia[p];
            cout<<"\nRota: "<<ponto.custo_rota<<" min | Batalhas: "<<custo_batalhas
                <<" min | Total: "<<ponto.custo_rota+custo_batalhas<<" min\n";
        }
        cout<<"\nPressione Enter para sair..."<<flush;getline(cin,enter);
        return 0;
    } catch(const exception& e){cerr<<"Erro: "<<e.what()<<'\n';return 1;}
}
