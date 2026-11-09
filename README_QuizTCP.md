# Quiz TCP Distribuído

**Trabalho III — Comunicação entre Processos | Sistemas Distribuídos**  
**Integrantes:** Alexandre Castanhede e Fernanda Bledow

## Descrição

Aplicação cliente-servidor em **C++**, desenvolvida com **sockets TCP** para Linux. O servidor apresenta cinco perguntas de múltipla escolha sobre redes e sistemas distribuídos, recebe as respostas dos participantes, calcula a pontuação e mantém um ranking dos jogadores que terminaram o quiz.

O sistema permite **múltiplos clientes conectados simultaneamente**. Cada conexão é atendida por uma thread, e o acesso ao ranking compartilhado é protegido com um mutex.

## Funcionalidades

- Comunicação entre processos em computadores diferentes pela rede TCP/IP.
- Identificação dos jogadores pelo nome.
- Cinco perguntas com quatro alternativas.
- Correção das respostas e pontuação individual.
- Atendimento simultâneo de vários clientes com `std::thread`.
- Proteção do ranking compartilhado com `std::mutex` e `std::lock_guard`.
- Ranking mostrado ao final de cada participação, contendo os jogadores que já concluíram até aquele momento.

## Tecnologias

- Linguagem: C++17.
- Rede: sockets POSIX / TCP (IPv4).
- Concorrência: biblioteca padrão `thread`.
- Sincronização: `mutex` e `lock_guard`.
- Sistemas: Linux (testado com Fedora e Ubuntu).
- Compilador: `g++`.

## Arquivos

```text
QuizTCP/
├── servidor.cpp     # Gerencia conexões, perguntas, pontuação e ranking
├── cliente.cpp      # Conecta ao servidor e apresenta o quiz no terminal
├── perguntas.txt    # Arquivo criado no projeto; não é lido pela implementação atual
├── .gitignore       # Ignora executáveis locais
└── README.md        # Documentação
```

> **Observação:** as perguntas da implementação atual estão definidas diretamente em `servidor.cpp`, não em `perguntas.txt`.

## Compilação

Execute na pasta do projeto:

```bash
g++ -std=c++17 -pthread servidor.cpp -o servidor
g++ -std=c++17 -pthread cliente.cpp -o cliente
```

## Execução

### 1. No computador servidor

Descubra o endereço IP da rede local:

```bash
hostname -I
```

Inicie o servidor:

```bash
./servidor
```

O servidor aguarda conexões TCP na **porta 8080** e permanece em execução para aceitar novos jogadores.

### 2. No computador cliente

Compile o arquivo `cliente.cpp` nesse computador e execute:

```bash
./cliente IP_DO_SERVIDOR
```

Exemplo ilustrativo:

```bash
./cliente 192.168.1.10
```

Para testar um cliente no mesmo computador do servidor:

```bash
./cliente
```

Nesse caso, o cliente utiliza `127.0.0.1` por padrão.

### 3. Jogo simultâneo

Inicie o servidor e conecte dois ou mais clientes, em terminais ou computadores diferentes. Cada jogador informa seu nome e responde às perguntas. A pontuação é individual; ao terminar, o participante recebe um ranking com os resultados registrados até aquele instante.

Se a conexão falhar, confira o IP atual do servidor, se a porta 8080 está acessível e se o firewall/rede permite conexões TCP entre os dispositivos.

## Arquitetura e fluxo de mensagens

```text
Cliente A ──TCP──┐
                │
Cliente B ──TCP──┼── Servidor TCP :8080
                │       ├── Thread A → Quiz do jogador A
Cliente C ──TCP──┘       ├── Thread B → Quiz do jogador B
                        ├── Thread C → Quiz do jogador C
                        └── Ranking compartilhado (mutex)
```

Sequência simplificada:

1. O cliente se conecta ao servidor por TCP.
2. O servidor solicita o nome e envia uma mensagem de boas-vindas.
3. Para cada pergunta, o servidor envia o identificador `QUESTAO`, o número da pergunta, o enunciado e quatro alternativas.
4. O cliente envia a alternativa escolhida (1 a 4).
5. O servidor valida a resposta e retorna o resultado.
6. Após as cinco perguntas, o servidor registra a pontuação no ranking, envia `FIM`, a pontuação e o ranking com marcador `RANKING`.

As mensagens são delimitadas por `\n`. Como TCP é um fluxo de bytes, as funções auxiliares `enviar()` e `receber()` tratam o envio completo e a leitura de uma linha por vez.

## Conceitos de Sistemas Distribuídos demonstrados

| Conceito | Aplicação |
|---|---|
| Comunicação entre processos | Cliente e servidor trocam mensagens por sockets TCP. |
| Distribuição | Cliente e servidor são processos distintos, executados em nós de rede diferentes. |
| Arquitetura cliente-servidor | O servidor concentra as perguntas, validação e pontuação. |
| Concorrência | Uma thread é criada para atender cada cliente conectado. |
| Exclusão mútua | Um mutex protege modificações e cópias do ranking compartilhado. |
| Protocolo de aplicação | Mensagens como `NOME`, `QUESTAO`, `FIM` e `RANKING` estruturam a interação. |

## Limitações da versão entregue

- O ranking fica somente na memória e é perdido quando o servidor é encerrado.
- O ranking recebido por cada cliente é uma visão do instante da sua finalização, não uma atualização em tempo real.
- As perguntas estão fixas no código-fonte do servidor.
- O servidor cria threads destacadas para os clientes e não implementa encerramento coordenado ou limite de conexões.
- A aplicação utiliza comunicação TCP sem criptografia ou autenticação.

## Demonstração

O trabalho foi preparado para ser demonstrado com o servidor em um computador Fedora e um cliente em Ubuntu, além de outro cliente que pode ser executado no Fedora. A demonstração evidencia dois nós e clientes participando simultaneamente.
