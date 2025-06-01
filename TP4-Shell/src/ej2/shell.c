#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMANDS 200
#define MAX_ARGS     65   // 64 tokens (prog + args) + 1 para el NULL

/* 
 * Recorta espacios y tabs al principio y al final de s (in‐place).
 */
void trim_whitespace(char *s) {
    while (*s && ( *s == ' ' || *s == '\t' )) {
        memmove(s, s + 1, strlen(s));
    }
    size_t len = strlen(s);
    while (len > 0 && ( s[len-1] == ' ' || s[len-1] == '\t' )) {
        s[len-1] = '\0';
        len--;
    }
}

/*
 * Separa line en tokens respetando comillas simples y dobles.
 * Devuelve número de tokens en argv[], o:
 *  -1 si hay comillas sin cerrar
 *  -2 si supera MAX_ARGS-1 tokens
 */
int parse_args_respecting_quotes(char *line, char *argv[]) {
    int argc = 0;
    char *p = line;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (argc >= MAX_ARGS - 1) {
            return -2; // exceso de tokens
        }

        if (*p == '"' || *p == '\'') {
            char quote = *p;
            p++;
            char *start = p;
            while (*p && *p != quote) p++;
            if (*p != quote) {
                return -1; // comilla sin cerrar
            }
            *p = '\0';     
            argv[argc++] = start;
            p++;
        } else {
            char *start = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            if (*p) {
                *p = '\0';
                p++;
            }
            argv[argc++] = start;
        }
    }
    argv[argc] = NULL;
    return argc;
}

/*
 * Retorna 1 si hay comillas simples o dobles sin pareja (número impar),
 * 0 en caso contrario.
 */
int has_unmatched_quotes(const char *s) {
    int count_single = 0, count_double = 0;
    for (; *s; s++) {
        if (*s == '\'') count_single++;
        if (*s == '"')  count_double++;
    }
    return (count_single % 2) || (count_double % 2);
}

/*
 * Verifica sintaxis antes de ejecutar:
 *  - Comillas sin cerrar?
 *  - Pipe al inicio o final?
 *  - "||" en la línea?
 *  - "|   |" (pipe vacío) en la línea?
 * Devuelve 1 si OK, 0 si hay error (imprime mensaje en stderr).
 */
int check_syntax(const char *orig) {
    if (has_unmatched_quotes(orig)) {
        fprintf(stderr, "Error de sintaxis: comillas sin cerrar\n");
        return 0;
    }
    // Quitar espacios/tabs al inicio
    const char *start = orig;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == '|') {
        fprintf(stderr, "Error de sintaxis: pipe al inicio\n");
        return 0;
    }
    // Quitar espacios/tabs al final
    const char *end = orig + strlen(orig) - 1;
    while (end > orig && isspace((unsigned char)*end)) end--;
    if (*end == '|') {
        fprintf(stderr, "Error de sintaxis: pipe al final\n");
        return 0;
    }
    // Detectar "||"
    if (strstr(orig, "||") != NULL) {
        fprintf(stderr, "Error de sintaxis: '||' no permitido\n");
        return 0;
    }
    // Detectar "|   |" (pipe vacío entre comandos)
    for (const char *p = orig; *p; p++) {
        if (*p == '|') {
            const char *q = p + 1;
            while (*q && isspace((unsigned char)*q)) q++;
            if (*q == '|') {
                fprintf(stderr, "Error de sintaxis: pipe vacío entre comandos\n");
                return 0;
            }
        }
    }
    return 1;
}

int main() {
    char line[4096];
    char *commands[MAX_COMMANDS];
    int command_count;

    while (1) {
        printf("Shell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            // Ctrl+D o EOF → salir
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';

        // Si la línea está vacía (solo espacios/tabs), volvemos a prompt
        char *p0 = line;
        while (*p0 && isspace((unsigned char)*p0)) p0++;
        if (!*p0) continue;

        // Caso especial: "exit" sin más
        {
            char tmp[4096];
            strcpy(tmp, line);
            trim_whitespace(tmp);
            if (strcmp(tmp, "exit") == 0) {
                exit(0);
            }
        }

        // Validación sintáctica previa
        if (!check_syntax(line)) {
            continue;
        }

        // 1) Partir la línea por '|' usando strtok (no strtok_r)
        command_count = 0;
        char *token = strtok(line, "|");
        while (token != NULL && command_count < MAX_COMMANDS) {
            trim_whitespace(token);
            if (strlen(token) == 0) {
                fprintf(stderr, "Error de sintaxis: comando vacío\n");
                break;
            }
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }
        if (token != NULL && command_count >= MAX_COMMANDS) {
            fprintf(stderr, "Error: máximo %d comandos en pipeline\n", MAX_COMMANDS);
            continue;
        }
        if (command_count == 0) {
            continue;
        }

        // 2) Crear pipes: si hay N comandos, se necesitan N-1 pipes
        int pipefd[MAX_COMMANDS - 1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipefd[i]) == -1) {
                perror("pipe");
                for (int j = 0; j < i; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                command_count = 0;
                goto fin_iter;
            }
        }

        // 3) Fork y exec de cada comando
        pid_t pids[MAX_COMMANDS];
        for (int i = 0; i < command_count; i++) {
            pids[i] = fork();
            if (pids[i] < 0) {
                perror("fork");
                for (int k = 0; k < command_count - 1; k++) {
                    close(pipefd[k][0]);
                    close(pipefd[k][1]);
                }
                command_count = 0;
                goto fin_iter;
            }
            if (pids[i] == 0) {
                // Hijo: redirigir stdin/stdout según posición en pipeline
                if (i > 0) {
                    if (dup2(pipefd[i-1][0], STDIN_FILENO) == -1) {
                        perror("dup2 stdin");
                        exit(EXIT_FAILURE);
                    }
                }
                if (i < command_count - 1) {
                    if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }
                // Cerrar todos los pipes en el hijo
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                // Parsear los argumentos respetando comillas
                char *argv[MAX_ARGS];
                int argc = parse_args_respecting_quotes(commands[i], argv);
                if (argc == -1) {
                    fprintf(stderr, "Error de sintaxis: comilla sin cerrar en '%s'\n", commands[i]);
                    exit(EXIT_FAILURE);
                }
                if (argc == -2) {
                    fprintf(stderr, "Error: exceso de argumentos en '%s' (máximo %d)\n",
                            commands[i], MAX_ARGS - 1);
                    exit(EXIT_FAILURE);
                }
                if (argc == 0) {
                    fprintf(stderr, "Error: comando vacío en posición %d\n", i);
                    exit(EXIT_FAILURE);
                }
                execvp(argv[0], argv);
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            // Padre sigue creando hijos
        }

        // 4) Cerrar pipes en el padre
        for (int i = 0; i < command_count - 1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }
        // 5) Esperar a todos los hijos
        for (int i = 0; i < command_count; i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }

    fin_iter:
        command_count = 0;
    }

    return 0;
}
