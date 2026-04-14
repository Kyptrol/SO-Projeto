#ifndef RUNNER_H
#define RUNNER_H

typedef struct {
    int user_id;
    int command_id;
    char command[256];
    char reply_pipe[64];
    int status; // 0 = pendente, 1 = em execução, 2 = concluída, 3 = (-s) pedido de terminação do controller
} Request;

#endif