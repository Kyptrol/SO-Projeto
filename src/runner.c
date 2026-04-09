#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main () {
    int fd = open("tmp/pipe", O_WRONLY); // Abre o pipe para escrita
    if (fd == -1) {
        perror("Erro ao abrir o pipe");
        return 1;
    }
    write (fd, "Hello\n", 6); // Escreve a mensagem "Hello, World!" no pipe
    close(fd); // Fecha o pipe

    return 0;
}
