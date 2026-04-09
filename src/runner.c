#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "runner.h"
#include <string.h>
#include <sys/wait.h>

int main (int argc, char *argv[]) {

    if (argc < 4) { // Verifica se o número de argumentos é menor que 4 (programa, modo de utilização, user_id, comando)
        fprintf(stderr, "Uso: %s <modo de utilização> <user_id> <command> [args...]\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "-e") == 0) { // Modo de execução
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
    
        write (fd_req, &req, sizeof(Request)); // Escreve a estrutura Request no pipe
    
        //-----------------------------------------------------Resposta----------------------------------------------
        int fd_res = open("tmp/pipe_res", O_RDONLY); // Abre o pipe de resposta para leitura
        if (fd_res == -1) {
            perror("Erro ao abrir o pipe de resposta");
            return 1;
        }
    
        char buffer[10];
        int n = read (fd_res, buffer, sizeof(buffer)); // Lê a resposta do pipe
        if (n == -1) {
            perror("Erro ao ler do pipe de resposta");
            close(fd_res);
            return 1;
        }
    
        buffer[n] = '\0'; // Adiciona o terminador de string
        if (strcmp(buffer, "Ok\n") == 0) {
    
            pid_t pid = fork(); // Cria um processo filho para executar o comando
            if (pid == -1) {
                perror("Erro ao criar processo filho");
                close(fd_res);
                return 1;
            } else if (pid == 0) { // Processo filho
                
    
                execvp(argv[3], &argv[3]); // Executa o comando usando execvp, passando o comando e seus argumentos a partir do índice 3
                perror("Erro ao executar o comando"); // Se execvp retornar, houve um erro
                exit(1); // Termina o processo filho com erro
            } else { // Processo pai
                wait(NULL); // Espera o processo filho terminar
                printf("Comando executado com sucesso: %s\n", buffer);
                write(fd_req, "FINISHED", 8); // Escreve "FINISHED" no pipe para indicar que o comando foi executado
            }
        } else {
            printf("Erro na execução do comando: %s\n", buffer);
        }
    
    
        close(fd_req); // Fecha o pipe
        close(fd_res); // Fecha o pipe de resposta

    } else if (strcmp(argv[1], "-c") == 0) { // Consultar comandos em execução    
        printf("Modo de listagem não implementado ainda.\n");
    } else if (strcmp(argv[1], "-s") == 0) { // Pede a terminação do programa controller
        printf("Modo de terminação não implementado ainda.\n");
        return 1;
    } else {
        fprintf(stderr, "Modo de utilização inválido. Use -e para execução, -c para consulta ou -s para terminação.\n");
        return 1;
    }
}
