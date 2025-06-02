#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

/*
 * Parámetros máximos
 */
#define MAX_COMMANDS 200   // máximo subcomandos en pipeline
#define MAX_ARGS     65    // hasta 64 tokens + 1 NULL

/*
 * trim_whitespace(s):
 *   Elimina espacios y tabs al inicio y al final de la cadena s (in-place).
 */
void trim_whitespace(char *s) {
    // recortar al inicio
    while (*s && ( *s == ' ' || *s == '\t' )) {
        memmove(s, s + 1, strlen(s));
    }
    // recortar al final
    size_t len = strlen(s);
    while (len > 0 && ( s[len-1] == ' ' || s[len-1] == '\t' )) {
        s[len-1] = '\0';
        len--;
    }
}

/*
 * parse_args_respecting_quotes(line, argv[]):
 *   - Divide line en tokens, usando espacios/tabs como separadores,
 *     pero agrupa todo lo que esté entre comillas simples '...' o dobles "..."
 *     como un único token (sin incluir las comillas).
 *   - Devuelve cantidad de tokens en argv[], o:
 *       - -1 si detecta comilla sin cerrar
 *       - -2 si excede MAX_ARGS-1 tokens
 *   - Siempre deja argv[n] == NULL como terminador.
 */
int parse_args_respecting_quotes(char *line, char *argv[]) {
    int argc = 0;
    char *p = line;

    while (*p) {
        // saltar cualquier espacio o tab
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (argc >= MAX_ARGS - 1) {
            return -2; // exceso de tokens
        }

        if (*p == '"' || *p == '\'') {
            char quote = *p;   // '"' o '\''
            p++;
            char *start = p;
            // buscar comilla de cierre
            while (*p && *p != quote) p++;
            if (*p != quote) {
                return -1; // comilla sin cerrar
            }
            *p = '\0';        // cerrar token
            argv[argc++] = start;
            p++;              // avanzar tras la comilla de cierre
        } else {
            // token normal sin comillas:
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
 * has_unmatched_quotes(s):
 *   Cuenta comillas simples y dobles. Si hay número impar de cualquiera,
 *   devuelve 1; si están emparejadas, devuelve 0.
 */
int has_unmatched_quotes(const char *s) {
    int count_single = 0, count_double = 0;
    for (; *s; s++) {
        if (*s == '\'')  count_single++;
        if (*s == '"')   count_double++;
    }
    return (count_single % 2) || (count_double % 2);
}

/*
 * check_syntax(orig):
 *   Verifica errores básicos de sintaxis ANTES de partir en subcomandos:
 *    - Comillas sin cerrar
 *    - Pipe al inicio o al final (ignorando espacios/tabs)
 *    - "||" (dos pipes consecutivos)
 *    - "|   |" (pipe vacío entre comandos)
 *   Si detecta error, imprime mensaje en stderr y retorna 0. Si todo OK, retorna 1.
 */
int check_syntax(const char *orig) {
    // 1) ¿Comillas sin cerrar?
    if (has_unmatched_quotes(orig)) {
        fprintf(stderr, "Error de sintaxis: comillas sin cerrar\n");
        return 0;
    }

    // 2) Pipe al inicio (tras ignorar espacios/tabs)
    const char *start = orig;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == '|') {
        fprintf(stderr, "Error de sintaxis: pipe al inicio\n");
        return 0;
    }

    // 3) Pipe al final (tras ignorar espacios/tabs)
    const char *end = orig + strlen(orig) - 1;
    while (end > orig && isspace((unsigned char)*end)) end--;
    if (*end == '|') {
        fprintf(stderr, "Error de sintaxis: pipe al final\n");
        return 0;
    }

    // 4) Detectar "||"
    if (strstr(orig, "||") != NULL) {
        fprintf(stderr, "Error de sintaxis: '||' no permitido\n");
        return 0;
    }

    // 5) Detectar "|   |" (pipe vacío entre comandos)
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
        // A) Solo imprimir prompt si stdin es un terminal
        if (isatty(STDIN_FILENO)) {
            printf("Shell> ");
            fflush(stdout);
        }

        // Leer línea
        if (!fgets(line, sizeof(line), stdin)) {
            // EOF o Ctrl+D
            if (isatty(STDIN_FILENO)) printf("\n");
            break;
        }
        // Eliminar '\n'
        line[strcspn(line, "\n")] = '\0';

        // Si la línea está vacía (solo espacios/tabs), volvemos a pedir prompt
        char *p0 = line;
        while (*p0 && isspace((unsigned char)*p0)) p0++;
        if (!*p0) continue;

        // 4) Si la línea, tras recortar extremos, es EXACTAMENTE "exit", salimos
        {
            char tmp[4096];
            strcpy(tmp, line);
            trim_whitespace(tmp);
            if (strcmp(tmp, "exit") == 0) {
                exit(0);
            }
        }

        // 5) Validar sintaxis antes de partir en subcomandos
        if (!check_syntax(line)) {
            continue;
        }

        // 6) PARTIR la línea en subcomandos separados por '|'
        command_count = 0;
        char *token = strtok(line, "|");
        while (token != NULL && command_count < MAX_COMMANDS) {
            trim_whitespace(token);
            if (strlen(token) == 0) {
                // comando vacío (por ejemplo, "ls |  | wc")
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

        // C) Si hay MÁS de 100 subcomandos, ignoramos la línea completa
        //    (para evitar saturar forks en pipelines gigantes).
        if (command_count > 100) {
            command_count = 0;
            continue;
        }

        // 7) Crear los (command_count - 1) pipes
        int pipefd[MAX_COMMANDS - 1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipefd[i]) == -1) {
                perror("pipe");
                // cerrar los que sí se abrieron
                for (int j = 0; j < i; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                command_count = 0;
                goto fin_iter;
            }
        }

        // 8) Fork + execvp de cada subcomando
        pid_t pids[MAX_COMMANDS];
        for (int i = 0; i < command_count; i++) {
            pids[i] = fork();
            if (pids[i] < 0) {
                perror("fork");
                // cerrar pipes
                for (int k = 0; k < command_count - 1; k++) {
                    close(pipefd[k][0]);
                    close(pipefd[k][1]);
                }
                command_count = 0;
                goto fin_iter;
            }

            if (pids[i] == 0) {
                // ----- Código del HIJO para ejecutar commands[i] -----

                // Redirigir stdin si no es el primer comando
                if (i > 0) {
                    if (dup2(pipefd[i-1][0], STDIN_FILENO) == -1) {
                        perror("dup2 stdin");
                        exit(EXIT_FAILURE);
                    }
                }
                // Redirigir stdout si no es el último comando
                if (i < command_count - 1) {
                    if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }

                // Cerrar TODOS los extremos de todos los pipes en el hijo
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }

                // Parsear los argumentos de commands[i], respetando comillas
                char *argv[MAX_ARGS];
                int argc = parse_args_respecting_quotes(commands[i], argv);
                if (argc == -1) {
                    fprintf(stderr,
                            "Error de sintaxis: comilla sin cerrar en '%s'\n",
                            commands[i]);
                    exit(EXIT_FAILURE);
                }
                if (argc == -2) {
                    fprintf(stderr,
                            "Error: exceso de argumentos en '%s' (máximo %d)\n",
                            commands[i], MAX_ARGS - 1);
                    exit(EXIT_FAILURE);
                }
                if (argc == 0) {
                    fprintf(stderr, "Error: comando vacío en posición %d\n", i);
                    exit(EXIT_FAILURE);
                }

                // B) Si el subcomando ES “exit”, simplemente terminar con código 0
                if (strcmp(argv[0], "exit") == 0) {
                    exit(0);
                }

                // Ejecutar execvp
                execvp(argv[0], argv);
                // Si retorna, hubo error
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            // El padre continúa creando los demás hijos...
        }

        // 9) En el PADRE, cerrar todos los extremos de pipe
        for (int i = 0; i < command_count - 1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }

        // 10) Esperar a todos los hijos
        for (int i = 0; i < command_count; i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }

    fin_iter:
        command_count = 0;
    }

    return 0;
}
