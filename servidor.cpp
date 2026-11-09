
#include <iostream>
#include <string>
#include <vector>
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

// Envia uma linha completa pelo TCP
bool enviar(int socket, const string& mensagem) {
    string dados = mensagem + "\n";
    size_t enviados = 0;

    while (enviados < dados.size()) {
        ssize_t n = send(socket, dados.data() + enviados,
                         dados.size() - enviados, MSG_NOSIGNAL);

        if (n <= 0) return false;
        enviados += n;
    }

    return true;
}

// Recebe uma linha completa pelo TCP
bool receber(int socket, string& mensagem) {
    mensagem.clear();
    char caractere;

    while (true) {
        ssize_t n = recv(socket, &caractere, 1, 0);

        if (n <= 0) return false;
        if (caractere == '\n') return true;

        if (mensagem.size() >= 4096) return false;
        mensagem += caractere;
    }
}

int main() {
    vector<Pergunta> perguntas = {
        {
            "Qual protocolo garante entrega ordenada dos dados?",
            {"UDP", "TCP", "IP", "ICMP"},
            2
        },
        {
            "Qual funcao permite ao servidor aceitar conexoes?",
            {"send()", "recv()", "accept()", "close()"},
            3
        },
        {
            "O que caracteriza um sistema distribuido?",
            {
                "Um unico processador",
                "Processos em computadores conectados por rede",
                "Um programa sem comunicacao",
                "Um computador sem sistema operacional"
            },
            2
        },
        {
            "Qual funcao cria um socket?",
            {"socket()", "bind()", "listen()", "connect()"},
            1
        },
        {
            "Qual protocolo nao garante a entrega dos pacotes?",
            {"TCP", "HTTP", "UDP", "SSH"},
            3
        }
    };

    int servidor = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor < 0) {
        cerr << "Erro ao criar socket!" << endl;
        return 1;
    }

    int opcao = 1;
    setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR,
               &opcao, sizeof(opcao));

    sockaddr_in endereco{};
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(8080);

    if (bind(servidor, (sockaddr*)&endereco,
             sizeof(endereco)) < 0) {
        cerr << "Erro no bind: " << strerror(errno) << endl;
        close(servidor);
        return 1;
    }

    if (listen(servidor, 5) < 0) {
        cerr << "Erro no listen!" << endl;
        close(servidor);
        return 1;
    }

    cout << "==================================" << endl;
    cout << "        SERVIDOR QUIZ TCP         " << endl;
    cout << "==================================" << endl;
    cout << "Aguardando jogador na porta 8080..." << endl;

    int cliente = accept(servidor, nullptr, nullptr);

    if (cliente < 0) {
        cerr << "Erro ao aceitar conexao!" << endl;
        close(servidor);
        return 1;
    }

    cout << "Jogador conectado!" << endl;

    int pontuacao = 0;
    string resposta;

    bool conectado = enviar(cliente,
        "Bem-vindo ao Quiz de Sistemas Distribuidos!");

    for (size_t i = 0; i < perguntas.size() && conectado; i++) {
        const Pergunta& p = perguntas[i];

        string mensagem =
            "PERGUNTA " + to_string(i + 1) + "/" +
            to_string(perguntas.size()) + "\n";

        mensagem += p.enunciado + "\n";

        for (int j = 0; j < 4; j++) {
            mensagem += to_string(j + 1) + ") " +
                        p.alternativas[j] + "\n";
        }

        // Protocolo: primeira linha informa o tipo
        // e quantidade de linhas do enunciado.
        conectado = enviar(cliente, "QUESTAO");
        if (!conectado) break;

        // Enviar enunciado e alternativas separadamente
        conectado = enviar(cliente,
            "Pergunta " + to_string(i + 1) + " de " +
            to_string(perguntas.size()));

        conectado = conectado && enviar(cliente, p.enunciado);

        for (int j = 0; j < 4 && conectado; j++) {
            conectado = enviar(cliente,
                to_string(j + 1) + ") " + p.alternativas[j]);
        }

        if (!conectado || !receber(cliente, resposta)) {
            conectado = false;
            break;
        }

        int escolha = 0;
        try {
            escolha = stoi(resposta);
        } catch (...) {
            escolha = 0;
        }

        if (escolha == p.correta) {
            pontuacao++;
            conectado = enviar(cliente, "CORRETO");
            cout << "Pergunta " << i + 1 << ": correta" << endl;
        } else {
            conectado = enviar(cliente,
                "ERRADO - alternativa correta: " +
                to_string(p.correta));
            cout << "Pergunta " << i + 1 << ": incorreta" << endl;
        }
    }

    if (conectado) {
        enviar(cliente, "FIM");
        enviar(cliente, "Pontuacao final: " +
               to_string(pontuacao) + "/" +
               to_string(perguntas.size()));

        cout << "Quiz finalizado! Pontuacao: "
             << pontuacao << "/" << perguntas.size() << endl;
    } else {
        cout << "Jogador desconectado." << endl;
    }

    close(cliente);
    close(servidor);

    return 0;
}
