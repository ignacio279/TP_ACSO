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
    int start = atoi(argv[3]) - 1;  // Convertir a índice base 0

    int pipes[n][2];
    for (int i = 0; i < n; i++) pipe(pipes[i]);

    for (int i = 0; i < n; i++) {
        if (fork() == 0) {
            int prev = (i - 1 + n) % n;
            int next = i;
            close(pipes[prev][1]);  // Cerrar escritura del pipe anterior
            close(pipes[next][0]);  // Cerrar lectura del pipe siguiente

            int x;
            read(pipes[prev][0], &x, sizeof(x));  // Leer del proceso anterior
            printf("Hijo %d: Leyó valor %d\n", i + 1, x);
            x += 1;  // Incrementar el valor
            write(pipes[next][1], &x, sizeof(x));  // Escribir en el siguiente proceso

            close(pipes[prev][0]);  // Cerrar lectura del pipe anterior
            close(pipes[next][1]);  // Cerrar escritura del pipe siguiente
            exit(0);
        }
    }

    close(pipes[start][0]);  // Cerrar lectura del pipe de inicio
    for (int i = 0; i < n; i++) if (i != start) close(pipes[i][0]);  // Cerrar pipes que no se usan

    printf("Padre: Enviando valor %d al proceso %d\n", val, start + 1);
    write(pipes[start][1], &val, sizeof(val));  // Enviar valor inicial al proceso de inicio

    int result;
    // El padre debe leer del pipe del último proceso que completó el ciclo
    int last = (start + n - 1) % n;  // Último proceso que pasó el valor
    close(pipes[last][1]);  // Cerrar escritura en el último pipe
    read(pipes[last][0], &result, sizeof(result));  // Leer el resultado final

    printf("Resultado final: %d\n", result);  // Imprimir el resultado final

    for (int i = 0; i < n; i++) wait(NULL);  // Esperar que todos los hijos terminen

    return 0;
}
