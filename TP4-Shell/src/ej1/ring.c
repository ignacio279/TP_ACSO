#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n     = atoi(argv[1]);
    int val   = atoi(argv[2]);
    int start = atoi(argv[3]) - 1;  // convertir a índice base 0

    if (n < 1 || start < 0 || start >= n) {
        fprintf(stderr, "Parámetros inválidos\n");
        exit(EXIT_FAILURE);
    }

    // Creamos los n pipes
    int pipes[n][2];
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Fork de cada hijo
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            // Código del hijo i
            int prev = (i - 1 + n) % n;
            int next = i;

            // Cerrar todos los extremos que no usamos
            for (int j = 0; j < n; j++) {
                if (j != prev) close(pipes[j][0]);  // sólo dejamos abierto pipes[prev][0]
                if (j != next) close(pipes[j][1]);  // sólo dejamos abierto pipes[next][1]
            }

            // Leer, mostrar, incrementar y reenviar
            int x;
            if (read(pipes[prev][0], &x, sizeof(x)) != sizeof(x)) {
                perror("read hijo");
                exit(EXIT_FAILURE);
            }
            printf("Hijo %d: leyó %d\n", i + 1, x);
            x++;
            if (write(pipes[next][1], &x, sizeof(x)) != sizeof(x)) {
                perror("write hijo");
                exit(EXIT_FAILURE);
            }

            // Cerramos y salimos
            close(pipes[prev][0]);
            close(pipes[next][1]);
            exit(EXIT_SUCCESS);
        }
        // el padre continúa al siguiente fork()
    }

    // Código del padre
    int prev = (start - 1 + n) % n;

    // Cerrar todos los fds salvo los de pipes[prev]
    for (int j = 0; j < n; j++) {
        if (j != prev) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
    }

    // Inyectar el valor inicial
    printf("Padre: enviando valor %d al proceso %d\n", val, start + 1);
    if (write(pipes[prev][1], &val, sizeof(val)) != sizeof(val)) {
        perror("write padre");
        exit(EXIT_FAILURE);
    }

    // Leer el resultado final
    int result;
    if (read(pipes[prev][0], &result, sizeof(result)) != sizeof(result)) {
        perror("read padre");
        exit(EXIT_FAILURE);
    }
    printf("Resultado final: %d\n", result);

    // Esperar a todos los hijos
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    return 0;
}
