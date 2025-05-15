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

    int n = atoi(argv[1]);
    int val = atoi(argv[2]);
    int start = atoi(argv[3]) - 1;  // Convertir al índice base 0

    int pipes[n][2];
    for (int i = 0; i < n; i++) pipe(pipes[i]);

    for (int i = 0; i < n; i++) {
        if (fork() == 0) {
            int prev = (i - 1 + n) % n;
            int next = i;
            close(pipes[prev][1]);
            close(pipes[next][0]);

            int x;
            read(pipes[prev][0], &x, sizeof(x));
            printf("Hijo %d: Leyó valor %d\n", i + 1, x);
            x += 1;
            write(pipes[next][1], &x, sizeof(x));

            close(pipes[prev][0]);
            close(pipes[next][1]);
            exit(0);
        }
    }

    close(pipes[start][0]);
    for (int i = 0; i < n; i++) if (i != start) close(pipes[i][0]);

    printf("Padre: Enviando valor %d al proceso %d\n", val, start + 1);
    write(pipes[start][1], &val, sizeof(val));

    int result;
    // El padre debe leer del pipe del proceso que terminó el ciclo
    int last = (start - 1 + n) % n;  // El último proceso que pasó el valor
    close(pipes[last][1]);
    read(pipes[last][0], &result, sizeof(result));

    printf("Resultado final: %d\n", result);

    for (int i = 0; i < n; i++) wait(NULL); // Esperar a que todos los hijos terminen

    return 0;
}
