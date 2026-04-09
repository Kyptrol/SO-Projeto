#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


int main () {

    mkfifo("tmp/pipe", 0666); // Cria o pipe chamado "pipe" com permissões de leitura e escrita para todos os usuários

    int fd = open("tmp/pipe", O_RDONLY); // Abre o pipe para leitura
    if (fd == -1) {
        perror("Erro ao abrir o pipe");
        return 1;
    }
    char buffer[100];
    int n = read (fd, buffer, sizeof(buffer)); // Lê a mensagem do pipe
    if (n == -1) {
        perror("Erro ao ler do pipe");
        close(fd);
        return 1;
    }
    write (STDOUT_FILENO, buffer, n); // Escreve a mensagem lida no pipe para a saída padrão

    return 0;
}
