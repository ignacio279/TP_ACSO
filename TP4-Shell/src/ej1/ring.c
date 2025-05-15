#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Uso: anillo <n> <c> <s>\n");
        exit(EXIT_FAILURE);
    }
    int n     = atoi(argv[1]);
    int valor = atoi(argv[2]);
    int start = atoi(argv[3]);
    if (n < 3 || start < 1 || start > n) {
        fprintf(stderr, "Error: n>=3 y 1<=s<=n\n");
        exit(EXIT_FAILURE);
    }
    printf("Se crearán %d procesos, se enviará el valor %d desde proceso %d\n",
           n, valor, start);

    int pipes[n][2];
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid;
    for (int i = 0; i < n; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            int idx      = i;
            int read_fd  = pipes[(idx - 1 + n) % n][0];
            int write_fd = pipes[idx][1];
            for (int j = 0; j < n; j++) {
                if (pipes[j][0] != read_fd)  close(pipes[j][0]);
                if (pipes[j][1] != write_fd) close(pipes[j][1]);
            }
            int x;
            if (read(read_fd, &x, sizeof(x)) != sizeof(x)) {
                perror("read hijo");
                exit(EXIT_FAILURE);
            }
            x += 1;
            if (write(write_fd, &x, sizeof(x)) != sizeof(x)) {
                perror("write hijo");
                exit(EXIT_FAILURE);
            }
            close(read_fd);
            close(write_fd);
            exit(EXIT_SUCCESS);
        }
    }

    int start_idx = start - 1;
    int prev      = (start_idx - 1 + n) % n;
    int wfd       = pipes[prev][1];
    int rfd       = pipes[prev][0];
    for (int j = 0; j < n; j++) {
        if (j != prev) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
    }
    if (write(wfd, &valor, sizeof(valor)) != sizeof(valor)) {
        perror("write padre");
        exit(EXIT_FAILURE);
    }
    int resultado;
    if (read(rfd, &resultado, sizeof(resultado)) != sizeof(resultado)) {
        perror("read padre");
        exit(EXIT_FAILURE);
    }
    printf("Resultado final: %d\n", resultado);
    close(wfd);
    close(rfd);
    for (int j = 0; j < n; j++) {
        wait(NULL);
    }
    return 0;
}
