
#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

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

int main(int argc, char* argv[]) {
    string ip = "127.0.0.1";

    // Permite informar IP na execucao:
    // ./cliente 192.168.1.10
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

    if (inet_pton(AF_INET, ip.c_str(),
                  &endereco.sin_addr) != 1) {
        cerr << "Endereco IP invalido!" << endl;
        close(cliente);
        return 1;
    }

    cout << "Conectando ao servidor " << ip << "..." << endl;

    if (connect(cliente, (sockaddr*)&endereco,
                sizeof(endereco)) < 0) {
        cerr << "Erro ao conectar ao servidor!" << endl;
        close(cliente);
        return 1;
    }

    cout << "Conectado com sucesso!" << endl;

    string mensagem;

    if (!receber(cliente, mensagem)) {
        cerr << "Falha ao receber boas-vindas!" << endl;
        close(cliente);
        return 1;
    }

    cout << "\n" << mensagem << endl;

    while (receber(cliente, mensagem)) {
        if (mensagem == "QUESTAO") {
            cout << "\n==================================" << endl;

            // Receber titulo, enunciado e alternativas
            for (int i = 0; i < 6; i++) {
                if (!receber(cliente, mensagem)) {
                    cerr << "Conexao encerrada!" << endl;
                    close(cliente);
                    return 1;
                }
                cout << mensagem << endl;
            }

            string resposta;

            do {
                cout << "\nSua resposta (1-4): ";
                if (!getline(cin, resposta)) {
                    close(cliente);
                    return 1;
                }
            } while (resposta != "1" &&
                     resposta != "2" &&
                     resposta != "3" &&
                     resposta != "4");

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
            if (receber(cliente, mensagem)) {
                cout << "\n==================================" << endl;
                cout << mensagem << endl;
                cout << "==================================" << endl;
            }
            break;
        }
    }

    close(cliente);
    return 0;
}
