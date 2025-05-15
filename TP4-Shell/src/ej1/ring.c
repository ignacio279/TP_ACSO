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
            close(pipes[prev][1]); // Cerrar escritura del pipe anterior
            close(pipes[next][0]); // Cerrar lectura del pipe siguiente

            int x;
            read(pipes[prev][0], &x, sizeof(x)); // Leer del pipe anterior
            x += 1;  // Incrementar el valor
            printf("Hijo %d: Leyó %d, Incrementado a %d\n", i + 1, x - 1, x);
            write(pipes[next][1], &x, sizeof(x)); // Escribir en el siguiente pipe

            close(pipes[prev][0]); // Cerrar lectura del pipe anterior
            close(pipes[next][1]); // Cerrar escritura del pipe siguiente
            exit(0);
        }
    }

    // Código del padre
    close(pipes[start][0]);
    for (int i = 0; i < n; i++) if (i != start) close(pipes[i][0]);

    printf("Padre: Enviando valor %d al proceso %d\n", val, start + 1);
    write(pipes[start][1], &val, sizeof(val)); // Enviar valor inicial

    int result;
    close(pipes[start][1]);
    read(pipes[start][0], &result, sizeof(result)); // Leer el resultado final

    printf("Resultado final: %d\n", result);

    for (int i = 0; i < n; i++) wait(NULL); // Esperar a que todos los hijos terminen

    return 0;
}
