#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n     = atoi(argv[1]);
    int val   = atoi(argv[2]);
    int start = atoi(argv[3]);

    if (n < 1 || start < 1 || start > n) {
        fprintf(stderr, "Parámetros inválidos\n");
        return EXIT_FAILURE;
    }

    // Creamos n pipes
    int pipes[n][2];
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return EXIT_FAILURE;
        }
    }

    // Fork de cada hijo i (índice base 0)
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return EXIT_FAILURE;
        }
        if (pid == 0) {
            // --- Código del hijo i ---
            int idx_prev = (i - 1 + n) % n;
            int idx_next = i;

            // Cerrar todos los fds que NO vamos a usar:
            for (int j = 0; j < n; j++) {
                if (j != idx_prev) close(pipes[j][0]);  // sólo leer de prev
                if (j != idx_next) close(pipes[j][1]);  // sólo escribir a next
            }

            // Leer, incrementar y reenviar
            int x;
            if (read(pipes[idx_prev][0], &x, sizeof(x)) != sizeof(x)) {
                perror("hijo read");
                exit(EXIT_FAILURE);
            }
            printf("Hijo %d leyó %d\n", i + 1, x);
            x++;
            if (write(pipes[idx_next][1], &x, sizeof(x)) != sizeof(x)) {
                perror("hijo write");
                exit(EXIT_FAILURE);
            }

            // Cerrar los dos fds que quedó usando y terminar
            close(pipes[idx_prev][0]);
            close(pipes[idx_next][1]);
            exit(EXIT_SUCCESS);
        }
        // el padre sigue iterando para fork() siguiente
    }

    // --- Código del padre ---
    int s0    = start - 1;                      // convertimos a base 0
    int entry = (s0 - 1 + n) % n;               // pipe por donde entra y sale el valor

    // Cerrar TODOS los fds que no sean pipes[entry][0] o [1]:
    for (int i = 0; i < n; i++) {
        if (i != entry) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
    }

    // Inyectar el valor inicial
    printf("Padre: enviando valor %d al proceso %d\n", val, start);
    if (write(pipes[entry][1], &val, sizeof(val)) != sizeof(val)) {
        perror("padre write");
        return EXIT_FAILURE;
    }

    // Leer el resultado final
    int result;
    if (read(pipes[entry][0], &result, sizeof(result)) != sizeof(result)) {
        perror("padre read");
        return EXIT_FAILURE;
    }
    printf("Resultado final: %d\n", result);

    // Cerrar los fds que el padre quedó usando
    close(pipes[entry][0]);
    close(pipes[entry][1]);

    // Esperar a todos los hijos para evitar zombies
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    return EXIT_SUCCESS;
}
