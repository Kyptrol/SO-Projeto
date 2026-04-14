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


int substitui_comando_no_array (Request req_arr[], int size, Request req) { // Função para verificar se um command_id específico está presente no array de Request
    for (int i = 0; i < size; i++) { // Loop para percorrer o array de Request
        if (req_arr[i].command_id == req.command_id) { // Verifica se o command_id do elemento atual do array corresponde ao command_id procurado
            req_arr[i].status = req.status; // Atualiza o status do comando para o status do Request recebido
            return 1; // Retorna 1 (verdadeiro) se o command_id for encontrado no array
        }
    }
    return 0; // Retorna 0 (falso) se o command_id não for encontrado no array
}

int main () {

    mkfifo("tmp/pipe_req", 0666); // Cria o pipe chamado "pipe" com permissões de leitura e escrita para todos os usuários


    Request req_arr[100]; // Declara um array de estruturas Request para armazenar os dados lidos do pipe


    int fd_req = open("tmp/pipe_req", O_RDWR); // Abre o pipe para leitura e 
    if (fd_req == -1) {
        perror("Erro ao abrir o pipe");
        return 1;
    }


    int query_size = 0; // Variável para controlar o índice do array de Request
    int running = 0; // Variável para controlar se o processo está executando um comando ou não
    char msg_Ok[] = "Ok\n"; // Mensagem de resposta a ser enviada para o runner
    int comando_em_execucao = 0; // Variável para armazenar o ID do comando em execução (command_id)
    int substitui_req = 0;
    struct timeval start, end; // Variáveis para armazenar o tempo de início e término da execução do comando
    int para_terminacao = 0; // Variável para controlar se o pedido de terminação do controller foi recebido

    while (1) { // Loop infinito para continuar lendo do pipe
        Request req; // Inicializa a estrutura Request para armazenar os dados lidos do pipe
        int n = read (fd_req, &req, sizeof(Request)); // Lê a estrutura
        if (n == -1) {
            perror("Erro ao ler do pipe");
            close(fd_req);
            return 1;
        }
        substitui_req = substitui_comando_no_array(req_arr, query_size, req); // Verifica se o command_id do comando lido do pipe já está presente no array de Request e atualiza o status se encontrado
        if (!substitui_req && !para_terminacao) { // Verifica se o comando lido do pipe não corresponde a nenhum command_id já armazenado no array de Request
            req_arr[query_size++] = req; // Armazena a estrutura lida no array de Request
        }
        printf("Comando recebido: %s\nComando_id: %d\nStatus: %d\n", req.command, req.command_id, req.status); // Imprime o comando recebido no console para fins de depuração

        if (running == 1 && req_arr[comando_em_execucao].status == 2) { // Verifica se há um comando em execução e se o status do comando lido é 2 (concluída)
            running = 0;
            comando_em_execucao++; // Incrementa o índice do comando em execução para o próximo comando no array
            gettimeofday(&end, NULL); // Marca o tempo de término da execução do comando
            double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // Calcula o tempo decorrido em segundos
            //Escrever num ficheiro de log o comando executado e o tempo gasto
            int fd_long = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666); // Abre o arquivo de log para escrita, criando-o se não existir e adicionando ao final do arquivo
            if (fd_long == -1) {
                perror("Erro ao abrir o arquivo de log");
                close(fd_req);
                return 1;
            }
            char buffer_log[512]; // Buffer para armazenar a mensagem de log
            sprintf(buffer_log, "Utilizador: %d\nComando executado: %s\nTempo gasto: %.6f segundos\n\n", req_arr[comando_em_execucao].user_id, req_arr[comando_em_execucao].command, elapsed);
            write(fd_long, buffer_log, strlen(buffer_log)); // Escreve a mensagem de log no arquivo
            close(fd_long); // Fecha o arquivo de log

        }
        if (running == 0 && req_arr[comando_em_execucao].status == 0 && comando_em_execucao < query_size) { // Verifica se não há um comando em execução e se o status do comando lido é 0 (pendente)
            running = 1; // Define a variável running como 1 para indicar que um comando está sendo executado
            gettimeofday(&start, NULL); // Marca o tempo de início da execução do comando

            //envia a resposta para o runner            
            int fd_res_this_pipe_reply = open(req_arr[comando_em_execucao].reply_pipe, O_WRONLY); // Abre o pipe de resposta para escrita
            if (fd_res_this_pipe_reply == -1) {
                perror("Erro ao abrir o pipe de resposta");
                close(fd_res_this_pipe_reply);
                return 1;
            }
            write(fd_res_this_pipe_reply, msg_Ok, strlen(msg_Ok)); // Envia a mensagem de resposta Ok pelo pipe
            close(fd_res_this_pipe_reply); // Fecha o pipe de resposta após enviar a mensagem
        }
        if (para_terminacao && running == 0) { // Verifica se o pedido de terminação do controller foi recebido e não há comando em execução
            printf("Encerrando o runner após a conclusão do comando em execução.\n");
            break; // Sai do loop para encerrar o runner
        }
        if (req.status == 3) { // Verifica se o status do comando lido do pipe é 3 (pedido de terminação do controller)
            printf("Pedido de terminação do controller recebido. Encerrando o runner.\n");
            para_terminacao = 1; // Define a variável para_terminacao como 1 para indicar que o pedido de terminação do controller foi recebido
        }
    }

    close(fd_req); // Fecha o pipe de pedido

    return 0;
}
