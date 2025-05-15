#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        return 1;
    }
    int n     = atoi(argv[1]);
    int val   = atoi(argv[2]);
    int start = atoi(argv[3]) - 1;  // base 0

    if (n < 1 || start < 0 || start >= n) {
        fprintf(stderr, "Parámetros inválidos\n");
        return 1;
    }

    int p[n][2];
    for (int i = 0; i < n; i++)
        if (pipe(p[i]) < 0) { perror("pipe"); return 1; }

    for (int i = 0; i < n; i++) {
        if (fork() == 0) {
            int prev = (i - 1 + n) % n;
            int next = i;
            // cerrar todos los fds que no sean p[prev][0] ni p[next][1]
            for (int j = 0; j < n; j++) {
                if (j != prev) close(p[j][0]);
                if (j != next) close(p[j][1]);
            }
            int x;
            read(p[prev][0], &x, sizeof(x));
            printf("Hijo %d: leyó %d\n", i+1, x);
            x++;
            write(p[next][1], &x, sizeof(x));
            return 0;
        }
    }

    // padre
    int entry = (start - 1 + n) % n;
    for (int i = 0; i < n; i++)
        if (i != entry) {
            close(p[i][0]);
            close(p[i][1]);
        }

    printf("Padre: enviando valor %d al proceso %d\n", val, start+1);
    write(p[entry][1], &val, sizeof(val));

    int result;
    read(p[entry][0], &result, sizeof(result));
    printf("Resultado final: %d\n", result);

    close(p[entry][0]);
    close(p[entry][1]);
    while (wait(NULL) > 0);
    return 0;
}
