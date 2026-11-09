
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

struct Pergunta {
    string enunciado;
    string alternativas[4];
    int correta;
};

struct Jogador {
    string nome;
    int pontos;
};

mutex mutexRanking;
vector<Jogador> ranking;

const vector<Pergunta> perguntas = {
    {
        "Qual protocolo garante entrega ordenada dos dados?",
        {"UDP", "TCP", "IP", "ICMP"}, 2
    },
    {
        "Qual funcao permite ao servidor aceitar conexoes?",
        {"send()", "recv()", "accept()", "close()"}, 3
    },
    {
        "O que caracteriza um sistema distribuido?",
        {
            "Um unico processador",
            "Processos em computadores conectados por rede",
            "Um programa sem comunicacao",
            "Um computador sem sistema operacional"
        }, 2
    },
    {
        "Qual funcao cria um socket?",
        {"socket()", "bind()", "listen()", "connect()"}, 1
    },
    {
        "Qual protocolo nao garante a entrega dos pacotes?",
        {"TCP", "HTTP", "UDP", "SSH"}, 3
    }
};

bool enviar(int socketCliente, const string& mensagem) {
    string dados = mensagem + "\n";
    size_t total = 0;

    while (total < dados.size()) {
        ssize_t n = send(
            socketCliente,
            dados.data() + total,
            dados.size() - total,
            MSG_NOSIGNAL
        );

        if (n <= 0) return false;
        total += n;
    }

    return true;
}

bool receber(int socketCliente, string& mensagem) {
    mensagem.clear();
    char c;

    while (true) {
        ssize_t n = recv(socketCliente, &c, 1, 0);

        if (n <= 0) return false;
        if (c == '\n') return true;

        if (mensagem.size() >= 4096) return false;
        mensagem += c;
    }
}

void atenderCliente(int socketCliente) {
    string nome;

    if (!enviar(socketCliente, "NOME") ||
        !receber(socketCliente, nome)) {
        close(socketCliente);
        return;
    }

    if (nome.empty()) nome = "Anonimo";

    if (nome.size() > 40) {
        nome.resize(40);
    }

    cout << "[CONEXAO] Jogador: " << nome << endl;

    int pontos = 0;
    bool conectado = true;
    string resposta;

    conectado = enviar(
        socketCliente,
        "Bem-vindo ao Quiz TCP, " + nome + "!"
    );

    for (size_t i = 0; i < perguntas.size() && conectado; i++) {
        const Pergunta& p = perguntas[i];

        conectado = enviar(socketCliente, "QUESTAO");
        if (!conectado) break;

        conectado = enviar(
            socketCliente,
            "Pergunta " + to_string(i + 1) +
            " de " + to_string(perguntas.size())
        );

        conectado = conectado &&
                    enviar(socketCliente, p.enunciado);

        for (int j = 0; j < 4 && conectado; j++) {
            conectado = enviar(
                socketCliente,
                to_string(j + 1) + ") " + p.alternativas[j]
            );
        }

        if (!conectado ||
            !receber(socketCliente, resposta)) {
            conectado = false;
            break;
        }

        if (resposta == to_string(p.correta)) {
            pontos++;
            conectado = enviar(socketCliente, "CORRETO");
        } else {
            conectado = enviar(
                socketCliente,
                "ERRADO - alternativa correta: " +
                to_string(p.correta)
            );
        }

        cout << "[" << nome << "] Pergunta "
             << i + 1 << " respondida." << endl;
    }

    if (conectado) {
        vector<Jogador> rankingAtual;

        // Regiao critica: somente uma thread
        // pode modificar o ranking por vez.
        {
            lock_guard<mutex> bloqueio(mutexRanking);

            ranking.push_back({nome, pontos});
            rankingAtual = ranking;
        }

        sort(
            rankingAtual.begin(),
            rankingAtual.end(),
            [](const Jogador& a, const Jogador& b) {
                return a.pontos > b.pontos;
            }
        );

        conectado = enviar(socketCliente, "FIM");
        conectado = conectado && enviar(
            socketCliente,
            "Pontuacao final: " + to_string(pontos) +
            "/" + to_string(perguntas.size())
        );

        conectado = conectado && enviar(
            socketCliente, "RANKING"
        );

        conectado = conectado && enviar(
            socketCliente, to_string(rankingAtual.size())
        );

        for (size_t i = 0;
             i < rankingAtual.size() && conectado; i++) {
            conectado = enviar(
                socketCliente,
                to_string(i + 1) + ". " +
                rankingAtual[i].nome + " - " +
                to_string(rankingAtual[i].pontos) + " pontos"
            );
        }

        cout << "[FINALIZADO] " << nome
             << " - " << pontos << " pontos" << endl;
    } else {
        cout << "[DESCONECTADO] " << nome << endl;
    }

    close(socketCliente);
}

int main() {
    int servidor = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor < 0) {
        cerr << "Erro ao criar socket!" << endl;
        return 1;
    }

    int opcao = 1;
    setsockopt(
        servidor, SOL_SOCKET, SO_REUSEADDR,
        &opcao, sizeof(opcao)
    );

    sockaddr_in endereco{};
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(8080);

    if (bind(
        servidor,
        (sockaddr*)&endereco,
        sizeof(endereco)
    ) < 0) {
        cerr << "Erro no bind: "
             << strerror(errno) << endl;
        close(servidor);
        return 1;
    }

    if (listen(servidor, 10) < 0) {
        cerr << "Erro no listen!" << endl;
        close(servidor);
        return 1;
    }

    cout << "===================================" << endl;
    cout << "      SERVIDOR QUIZ TCP           " << endl;
    cout << "        MULTIJOGADOR              " << endl;
    cout << "===================================" << endl;
    cout << "Porta: 8080" << endl;
    cout << "Aguardando jogadores..." << endl;

    while (true) {
        int cliente = accept(servidor, nullptr, nullptr);

        if (cliente < 0) {
            cerr << "Erro ao aceitar conexao!" << endl;
            continue;
        }

        // Cada cliente recebe uma thread propria
        thread jogador(atenderCliente, cliente);
        jogador.detach();
    }

    close(servidor);
    return 0;
}
