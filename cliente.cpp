
#include <iostream>
#include <string>
#include <limits>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

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

int main(int argc, char* argv[]) {
    string ip = "127.0.0.1";

    if (argc > 1) {
        ip = argv[1];
    }

    int cliente = socket(AF_INET, SOCK_STREAM, 0);

    if (cliente < 0) {
        cerr << "Erro ao criar socket!" << endl;
        return 1;
    }

    sockaddr_in endereco{};
    endereco.sin_family = AF_INET;
    endereco.sin_port = htons(8080);

    if (inet_pton(
        AF_INET, ip.c_str(),
        &endereco.sin_addr
    ) != 1) {
        cerr << "IP invalido!" << endl;
        close(cliente);
        return 1;
    }

    cout << "Conectando ao servidor " << ip << "..." << endl;

    if (connect(
        cliente,
        (sockaddr*)&endereco,
        sizeof(endereco)
    ) < 0) {
        cerr << "Erro ao conectar!" << endl;
        close(cliente);
        return 1;
    }

    cout << "Conectado com sucesso!" << endl;

    string mensagem;

    while (receber(cliente, mensagem)) {

        if (mensagem == "NOME") {
            string nome;

            do {
                cout << "\nDigite seu nome: ";
                if (!getline(cin, nome)) {
                    close(cliente);
                    return 1;
                }
            } while (nome.empty());

            if (!enviar(cliente, nome)) {
                cerr << "Erro ao enviar nome!" << endl;
                break;
            }
        }

        else if (mensagem == "QUESTAO") {
            cout << "\n==================================" << endl;

            // Receber titulo, pergunta e 4 alternativas
            bool sucesso = true;

            for (int i = 0; i < 6; i++) {
                if (!receber(cliente, mensagem)) {
                    sucesso = false;
                    break;
                }

                cout << mensagem << endl;
            }

            if (!sucesso) {
                cerr << "Conexao encerrada!" << endl;
                break;
            }

            string resposta;

            do {
                cout << "\nSua resposta (1-4): ";
                if (!getline(cin, resposta)) {
                    close(cliente);
                    return 1;
                }
            } while (
                resposta != "1" &&
                resposta != "2" &&
                resposta != "3" &&
                resposta != "4"
            );

            if (!enviar(cliente, resposta)) {
                cerr << "Erro ao enviar resposta!" << endl;
                break;
            }

            if (!receber(cliente, mensagem)) {
                cerr << "Conexao encerrada!" << endl;
                break;
            }

            cout << "\nResultado: " << mensagem << endl;
        }

        else if (mensagem == "FIM") {
            if (!receber(cliente, mensagem)) break;

            cout << "\n==================================" << endl;
            cout << mensagem << endl;
            cout << "==================================" << endl;
        }

        else if (mensagem == "RANKING") {
            if (!receber(cliente, mensagem)) break;

            int quantidade;

            try {
                quantidade = stoi(mensagem);
            } catch (...) {
                cerr << "Ranking invalido!" << endl;
                break;
            }

            if (quantidade < 0 || quantidade > 10000) {
                cerr << "Tamanho do ranking invalido!" << endl;
                break;
            }

            cout << "\n========== RANKING ==========" << endl;

            for (int i = 0; i < quantidade; i++) {
                if (!receber(cliente, mensagem)) {
                    cerr << "Conexao encerrada!" << endl;
                    close(cliente);
                    return 1;
                }

                cout << mensagem << endl;
            }

            cout << "=============================" << endl;
            break;
        }

        else {
            cout << "\n" << mensagem << endl;
        }
    }

    close(cliente);
    return 0;
}
