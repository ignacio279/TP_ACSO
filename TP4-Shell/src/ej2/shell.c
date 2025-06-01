#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200
#define MAX_ARGS     50   // Número máximo de argumentos por comando (incluyendo el comando en sí)

int main() {
    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count;

    while (1) {
        printf("Shell> ");
        fflush(stdout);

        /* Leer línea completa del usuario */
        if (fgets(command, sizeof(command), stdin) == NULL) {
            // Si el usuario presionó Ctrl+D o hubo error de lectura, salimos
            printf("\n");
            break;
        }

        /* Eliminar el salto de línea al final */
        command[strcspn(command, "\n")] = '\0';

        /* Si el usuario no ingresó nada, volvemos a mostrar prompt */
        if (command[0] == '\0') {
            continue;
        }

        /* 1) Partir la cadena completa por '|' y guardar cada subcomando en commands[] */
        command_count = 0;
        char *token = strtok(command, "|");
        while (token != NULL && command_count < MAX_COMMANDS) {
            /* Quitar posibles espacios en los extremos de cada subcomando */
            while (*token == ' ') token++;
            char *end = token + strlen(token) - 1;
            while (end > token && (*end == ' ')) {
                *end = '\0';
                end--;
            }
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        /* Si no había comandos válidos, volvemos a pedir prompt */
        if (command_count == 0) {
            continue;
        }

        /* 2) Crear (command_count - 1) pipes, en caso de haber más de un comando */
        int pipefd[MAX_COMMANDS - 1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipefd[i]) == -1) {
                perror("pipe");
                // Si falla crear pipe, cerramos los anteriores y salimos del ciclo
                for (int j = 0; j < i; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                command_count = 0;
                goto end_iteration; 
            }
        }

        /* 3) Forkear un hijo por cada subcomando */
        pid_t pids[MAX_COMMANDS];
        for (int i = 0; i < command_count; i++) {
            pids[i] = fork();
            if (pids[i] < 0) {
                perror("fork");
                // Error al forkear: cerrar pipes abiertos y pasar a la siguiente iteración
                for (int k = 0; k < command_count - 1; k++) {
                    close(pipefd[k][0]);
                    close(pipefd[k][1]);
                }
                command_count = 0;
                goto end_iteration;
            }

            if (pids[i] == 0) {
                /***** Código que ejecuta cada hijo *****/

                /* 3.1) Si no es el primer comando, leer del pipe anterior */
                if (i > 0) {
                    // Redirijo la lectura del pipe (i-1) a STDIN (fd 0)
                    if (dup2(pipefd[i-1][0], STDIN_FILENO) == -1) {
                        perror("dup2 stdin");
                        exit(EXIT_FAILURE);
                    }
                }

                /* 3.2) Si no es el último comando, escribir en el pipe actual */
                if (i < command_count - 1) {
                    // Redirijo la escritura al pipe i a STDOUT (fd 1)
                    if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }

                /* 3.3) Cerrar TODOS los descriptores de pipe en el hijo */
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }

                /* 3.4) Partir commands[i] en un array argv[] separado por espacios */
                char *argv[MAX_ARGS];
                int argc = 0;
                char *arg = strtok(commands[i], " ");
                while (arg != NULL && argc < (MAX_ARGS - 1)) {
                    argv[argc++] = arg;
                    arg = strtok(NULL, " ");
                }
                argv[argc] = NULL; // Último elemento debe ser NULL para execvp

                /* 3.5) Ejecutar el comando con execvp */
                if (argc == 0) {
                    fprintf(stderr, "Error: comando vacío en posición %d\n", i);
                    exit(EXIT_FAILURE);
                }
                execvp(argv[0], argv);
                /* Si execvp regresa, hubo un error */
                perror("execvp");
                exit(EXIT_FAILURE);
            }

            /* El padre continúa aquí para crear el siguiente hijo */
        }

        /* 4) En el proceso padre: cerrar TODOS los extremos de pipe */
        for (int i = 0; i < command_count - 1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }

        /* 5) Esperar a que terminen todos los hijos */
        for (int i = 0; i < command_count; i++) {
            int status;
            waitpid(pids[i], &status, 0);
            // Opcional: podrías inspeccionar "status" si quieres saber si algún hijo falló
        }

    end_iteration:
        /* Antes de volver a mostrar el prompt, reiniciamos command_count */
        command_count = 0;
    }

    return 0;
}
