#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "runner.h"
#include <string.h>
#include <sys/wait.h>

void pede_terminacao_controller() {
    int fd_req = open("tmp/pipe_req", O_WRONLY); // Abre o pipe para escrita
    if (fd_req == -1) {
        perror("Erro ao abrir o pipe");
        return;
    }

    Request req;
    req.user_id = 0; // User_id 0 para indicar que é um pedido de terminação
    req.command_id = 0; // Command_id 0 para indicar que é um pedido de terminação
    strcpy(req.command, "Controller Shutdown"); // Comando de terminação
    req.status = 3; // Status 3 para indicar que é um pedido de terminação

    write(fd_req, &req, sizeof(Request)); // Envia o Request pelo pipe

    close(fd_req); // Fecha o pipe
}

int main (int argc, char *argv[]) {

    if (strcmp(argv[1], "-e") == 0) { // Modo de execução
        if (argc < 4) { // Verifica se o número de argumentos é menor que 4 (programa, modo de utilização, user_id, comando)
            fprintf(stderr, "Uso: %s -e <user_id> <command> [args...]\n", argv[0]);
            return 1;
        }
        int fd_req = open("tmp/pipe_req", O_WRONLY); // Abre o pipe para escrita
        if (fd_req == -1) {
            perror("Erro ao abrir o pipe");
            return 1;
        }

        Request req;
        req.user_id = atoi(argv[2]); // Converte o argumento de string para inteiro e atribui a user_id
        req.command_id = getpid(); // Usa o PID do processo como command_idº
        req.command[0] = '\0'; // Inicializa a string command com o terminador de string
        for (int i = 3; i < argc; i++) { // Concatena os argumentos do comando a partir do índice 3
            strcat(req.command, argv[i]);
            if (i < argc - 1) {
                strcat(req.command, " "); // Adiciona um espaço entre os argumentos, até o penúltimo argumento
            }
        }
        req.status = 0; // Define o status como 0 (pendente)
        printf("[runner] command %d submitted\n", req.command_id); // Imprime uma mensagem de status no console
        // Preenche o campo reply_pipe com o nome do pipe de resposta específico para este comando
        snprintf(req.reply_pipe, sizeof(req.reply_pipe), "tmp/pipe_res_%d", req.command_id);
        mkfifo(req.reply_pipe, 0666); // Cria o pipe de resposta específico para este comando com permissões de leitura e escrita para todos os usuários
    
        write (fd_req, &req, sizeof(Request)); // Envia o Request pelo pipe
    
        //-----------------------------------------------------Resposta----------------------------------------------
        int fd_res_this_pipe_reply = open(req.reply_pipe, O_RDONLY); // Abre o pipe de resposta específico para este comando para leitura
        if (fd_res_this_pipe_reply == -1) {
            perror("Erro ao abrir o pipe de resposta");
            return 1;
        }
    
        char buffer[10];
        int n = read (fd_res_this_pipe_reply, buffer, sizeof(buffer)); // Lê a resposta do pipe
        if (n == -1) {
            perror("Erro ao ler do pipe de resposta");
            close(fd_res_this_pipe_reply);
            return 1;
        }
    
        buffer[n] = '\0'; // Adiciona o terminador de string
        if (strcmp(buffer, "Ok\n") == 0) {
    
            pid_t pid = fork(); // Cria um processo filho para executar o comando
            if (pid == -1) {
                perror("Erro ao criar processo filho");
                close(fd_res_this_pipe_reply);
                return 1;
            } else if (pid == 0) { // Processo filho
                req.status = 1; // Define o status como 1 (em execução)
                write(fd_req, &req, sizeof(Request)); // Escreve o status atualizado de volta no pipe de pedido para que o controller possa atualizar o status do comando
                printf("[runner] executing command %d...\n", req.command_id); // Imprime uma mensagem de status no console
                execvp(argv[3], &argv[3]); // Executa o comando usando execvp, passando o comando e seus argumentos a partir do índice 3
                perror("Erro ao executar o comando"); // Se execvp retornar, houve um erro
                exit(1); // Termina o processo filho com erro
            } else { // Processo pai
                int status;
                pid_t w = waitpid(pid, &status, 0); // Espera o processo filho terminar
                if (w == -1) {
                    perror("Erro ao esperar o processo filho");
                    close(fd_res_this_pipe_reply);
                    return 1;
                }
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) { // Verifica se o processo filho terminou com sucesso
                    req.status = 2; // Define o status como 2 (concluída)
                }
                printf("[runner] command %d finished.\n", req.command_id); // Imprime uma mensagem de status no console
                write(fd_req, &req, sizeof(Request)); // Escreve o status atualizado de volta no pipe de pedido para que o controller possa atualizar o status do comando
            }
        } else {
            printf("Erro na execução do comando: %s\n", buffer);
        }
    
    
        close(fd_req); // Fecha o pipe
        close(fd_res_this_pipe_reply); // Fecha o pipe de resposta

    } else if (strcmp(argv[1], "-c") == 0) { // Consultar comandos em execução    
        printf("Modo de listagem não implementado ainda.\n");
    } else if (strcmp(argv[1], "-s") == 0) { // Pede a terminação do programa controller
        if (argc != 2) { // Verifica se o número de argumentos é diferente de 2 (programa, modo de utilização)
            fprintf(stderr, "Uso: %s -s\n", argv[0]);
            return 1;
        }
        pede_terminacao_controller();
    } else {
        if (argc != 2) { // Verifica se o número de argumentos é diferente de 2 (programa, modo de utilização)
            fprintf(stderr, "Uso: %s -c\n", argv[0]);
            return 1;
        }
        fprintf(stderr, "Modo de utilização inválido. Use -e para execução, -c para consulta ou -s para terminação.\n");
        return 1;
    }
}
