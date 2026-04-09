#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "runner.h"
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

int main () {

    mkfifo("tmp/pipe_req", 0666); // Cria o pipe chamado "pipe" com permissões de leitura e escrita para todos os usuários
    mkfifo("tmp/pipe_res", 0666); // Cria o pipe chamado "pipe2" com permissões de leitura e escrita para todos os usuários

    int fd_req = open("tmp/pipe_req", O_RDONLY); // Abre o pipe para leitura
    if (fd_req == -1) {
        perror("Erro ao abrir o pipe");
        return 1;
    }

    Request req; // Inicializa a estrutura Request para armazenar os dados lidos do pipe
    struct timeval start, end;
    int n = read (fd_req, &req, sizeof(Request)); // Lê a estrutura
    if (n == -1) {
        perror("Erro ao ler do pipe");
        close(fd_req);
        return 1;
    }
    gettimeofday(&start, NULL); // Marca o tempo de início da execução do comando

    write (STDOUT_FILENO, req.command, strlen(req.command)); // Escreve o comando lido do pipe para a saída padrão (printf essencialmente)


    //-----------------------------------------------------Resposta-----------------------------------------------
    int fd_res = open("tmp/pipe_res", O_WRONLY); // Abre o pipe de resposta para escrita
    if (fd_res == -1) {
        perror("Erro ao abrir o pipe de resposta");
        close(fd_res);
        return 1;
    }
    char msg[] = "Ok\n";
    write(fd_res, msg, strlen(msg)); // Escreve a mensagem de resposta no pipe

    char buffer[10];
    int n2 = read(fd_req, buffer, sizeof(buffer)); // Lê a resposta do pipe
    if (n2 == -1) {
        perror("Erro ao ler do pipe de resposta\n");
        close(fd_res);
        return 1;
    }
    gettimeofday(&end, NULL); // Marca o tempo de término da execução do comando
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // Calcula o tempo decorrido em segundos
    buffer[n2] = '\0'; // Adiciona o terminador de string
    printf("Resposta recebida: %s\n", buffer); // Imprime a resposta recebida do pipe
    

    //Escrever num ficheiro de log o comando executado e o tempo gasto
    int fd_long = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666); // Abre o arquivo de log para escrita, criando-o se não existir e adicionando ao final do arquivo
    if (fd_long == -1) {
        perror("Erro ao abrir o arquivo de log");
        close(fd_res);
        return 1;
    }
    char buffer_log[512]; // Buffer para armazenar a mensagem de log
    sprintf(buffer_log, "Utilizador: %d\nComando executado: %s\nTempo gasto: %.6f segundos\n\n", req.user_id, req.command, elapsed);
    write(fd_long, buffer_log, strlen(buffer_log)); // Escreve a mensagem de log no arquivo
    close(fd_long); // Fecha o arquivo de log


    close(fd_req); // Fecha o pipe de pedido
    close(fd_res); // Fecha o pipe de resposta

    return 0;
}
