#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200
#define MAX_ARGS     200

int main() {
    char line[1024];
    char *commands[MAX_COMMANDS];
    int pipefds[2 * (MAX_COMMANDS - 1)];

    while (1) {
        // 1) Mostrar prompt y leer línea
        printf("Shell> ");
        if (!fgets(line, sizeof(line), stdin)) {
            // EOF (Ctrl-D)
            putchar('\n');
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;

        // 2) Separar en comandos por '|'
        int ncmds = 0;
        char *cmd = strtok(line, "|");
        while (cmd && ncmds < MAX_COMMANDS) {
            // recortar espacios al inicio
            while (*cmd == ' ' || *cmd == '\t') cmd++;
            commands[ncmds++] = cmd;
            cmd = strtok(NULL, "|");
        }

        if (ncmds == 0) continue;

        // 3) Crear todos los pipes necesarios
        for (int i = 0; i < ncmds - 1; i++) {
            if (pipe(pipefds + 2*i) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        // 4) Para cada comando, fork + setup de pipes + exec
        for (int i = 0; i < ncmds; i++) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if (pid == 0) {
                // --- en hijo ---
                // 4a) Si no es el primer comando, dup2 lectura del pipe previo a stdin
                if (i > 0) {
                    if (dup2(pipefds[2*(i-1)], STDIN_FILENO) < 0) {
                        perror("dup2 stdin");
                        exit(EXIT_FAILURE);
                    }
                }
                // 4b) Si no es el último comando, dup2 escritura del pipe actual a stdout
                if (i < ncmds - 1) {
                    if (dup2(pipefds[2*i + 1], STDOUT_FILENO) < 0) {
                        perror("dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }
                // 4c) Cerrar todos los fds de pipes en el hijo
                for (int j = 0; j < 2*(ncmds - 1); j++) {
                    close(pipefds[j]);
                }
                // 4d) Preparar argv para execvp (tokenizar por espacios)
                char *argv[MAX_ARGS];
                int argc = 0;
                char *arg = strtok(commands[i], " \t");
                while (arg && argc < MAX_ARGS-1) {
                    argv[argc++] = arg;
                    arg = strtok(NULL, " \t");
                }
                argv[argc] = NULL;
                if (argc == 0) exit(EXIT_SUCCESS);

                // 4e) Ejecutar
                execvp(argv[0], argv);
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            // --- en padre: seguimos creando hijos ---
        }

        // 5) En padre: cerrar pipes y esperar a todos los hijos
        for (int i = 0; i < 2*(ncmds - 1); i++) {
            close(pipefds[i]);
        }
        for (int i = 0; i < ncmds; i++) {
            wait(NULL);
        }
    }

    return 0;
}
