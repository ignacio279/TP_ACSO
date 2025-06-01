#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMANDS 200
#define MAX_ARGS     65   // 64 tokens (programa + args) + 1 para el NULL

/* 
 * Función que recorta espacios y tabs al inicio y al final de una cadena in-place.
 */
void trim_whitespace(char *s) {
    // Avanzar el puntero mientras haya espacio o tab
    while (*s && ( *s == ' ' || *s == '\t' )) {
        memmove(s, s + 1, strlen(s));
    }
    // Si queda algo, recorta la parte final
    size_t len = strlen(s);
    while (len > 0 && ( s[len-1] == ' ' || s[len-1] == '\t' )) {
        s[len-1] = '\0';
        len--;
    }
}

/*
 * Parser muy básico para separar una línea en tokens respetando comillas simples
 * y dobles. Devuelve el número de tokens encontrados en argv[].
 * Si detecta comillas sin cerrar, devuelve -1.
 * Si supera MAX_ARGS-1 tokens, devuelve -2.
 */
int parse_args_respecting_quotes(char *line, char *argv[]) {
    int argc = 0;
    char *p = line;

    while (*p) {
        // Saltar espacios/tabs
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (!*p) break;

        if (argc >= MAX_ARGS - 1) {
            return -2; // Exceso de tokens
        }

        if (*p == '"' || *p == '\'') {
            char quote = *p;
            p++;  // saltamos la comilla
            char *start = p;
            // buscamos la comilla coincidente
            while (*p && *p != quote) {
                p++;
            }
            if (*p != quote) {
                return -1; // comilla sin cerrar
            }
            *p = '\0';      // terminamos el token
            argv[argc++] = start;
            p++;            // avanzamos más allá de la comilla de cierre
        } else {
            // Token sin comillas: hasta próximo espacio/tab o fin
            char *start = p;
            while (*p && !isspace((unsigned char)*p)) {
                p++;
            }
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
 * Comprueba si el conteo de comillas (simples o dobles) en la línea es impar.
 * Devuelve 1 si hay comillas sin pareja, 0 en caso contrario.
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
 * Comprobaciones de sintaxis previas a ejecutar cualquier comando:
 *   - Línea vacía → OK (no hace nada).
 *   - Comillas sin cerrar → error.
 *   - Si, tras recortar espacios/tabs, empieza o termina con '|' → error.
 *   - Si existe "||" en la línea (dos pipes consecutivos) → error.
 *   - Si existe un pipe que, tras él, sólo hay espacios/tabs hasta otro pipe → error (p. ej. "| |").
 */
int check_syntax(const char *orig) {
    // 1) comillas sin cerrar:
    if (has_unmatched_quotes(orig)) {
        fprintf(stderr, "Error de sintaxis: comillas sin cerrar\n");
        return 0;
    }

    // 2) primero y último carácter no pueden ser '|', ignorando espacios/tabs:
    const char *start = orig;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == '|') {
        fprintf(stderr, "Error de sintaxis: pipe al inicio\n");
        return 0;
    }
    const char *end = orig + strlen(orig) - 1;
    while (end > orig && isspace((unsigned char)*end)) end--;
    if (*end == '|') {
        fprintf(stderr, "Error de sintaxis: pipe al final\n");
        return 0;
    }

    // 3) detectar "||" literalmente:
    if (strstr(orig, "||") != NULL) {
        fprintf(stderr, "Error de sintaxis: '||' no permitido\n");
        return 0;
    }

    // 4) detectar un pipe que sólo tenga espacios/tabs hasta el siguiente pipe:
    for (const char *p = orig; *p; p++) {
        if (*p == '|') {
            // Mirar hacia adelante si hay otro '|' tras espacios/tabs:
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

        // Eliminar el '\n' final
        line[strcspn(line, "\n")] = '\0';

        // Si sólo había espacios/tabs y nada más, volvemos a pedir prompt
        char *p0 = line;
        while (*p0 && isspace((unsigned char)*p0)) p0++;
        if (!*p0) {
            continue;
        }

        // Caso especial: si la línea es exactamente "exit" (sin pipes ni args),
        // salgo del shell sin escribir nada en stdout ni stderr.
        // IMPORTANTE: detectamos "exit" sólo si no forma parte de un pipeline.
        {
            // comprobamos que, tras recortar extremos, sea "exit"
            char tmp[4096];
            strcpy(tmp, line);
            trim_whitespace(tmp);
            if (strcmp(tmp, "exit") == 0) {
                // basta con terminar el programa
                exit(0);
            }
        }

        // ----- VALIDACIÓN SINTÁCTICA PREVIA -----
        if (!check_syntax(line)) {
            // hubo error de sintaxis → no ejecutamos nada
            continue;
        }

        // 1) Partir la línea por '|' (pipes).
        command_count = 0;
        char *saveptr = NULL;
        char *token = strtok_r(line, "|", &saveptr);
        while (token != NULL && command_count < MAX_COMMANDS) {
            // recortar espacios/tabs de cada subcomando
            trim_whitespace(token);
            // si quedó vacío, error:
            if (strlen(token) == 0) {
                fprintf(stderr, "Error de sintaxis: comando vacío\n");
                break;
            }
            commands[command_count++] = token;
            token = strtok_r(NULL, "|", &saveptr);
        }
        if (token != NULL && command_count >= MAX_COMMANDS) {
            // demasiados comandos (más de 200)
            fprintf(stderr, "Error: máximo %d comandos en pipeline\n", MAX_COMMANDS);
            continue;
        }
        if (command_count == 0) {
            // no había nada útil
            continue;
        }

        // 2) Crear los pipes necesarios: command_count subcomandos → (command_count - 1) pipes
        int pipefd[MAX_COMMANDS - 1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipefd[i]) == -1) {
                perror("pipe");
                // cerrar los que se hayan abierto
                for (int j = 0; j < i; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                command_count = 0;
                goto fin_iteracion;
            }
        }

        // 3) Fork para cada subcomando
        pid_t pids[MAX_COMMANDS];
        for (int i = 0; i < command_count; i++) {
            pids[i] = fork();
            if (pids[i] < 0) {
                perror("fork");
                // cerrar pipes abiertos y abortar esta orden
                for (int k = 0; k < command_count - 1; k++) {
                    close(pipefd[k][0]);
                    close(pipefd[k][1]);
                }
                command_count = 0;
                goto fin_iteracion;
            }
            if (pids[i] == 0) {
                // ------ Código del hijo para ejecutar commands[i] ------

                // 3.1) Si NO es el primer comando, redirijo stdin al pipe[i-1][0]
                if (i > 0) {
                    if (dup2(pipefd[i-1][0], STDIN_FILENO) == -1) {
                        perror("dup2 stdin");
                        exit(EXIT_FAILURE);
                    }
                }
                // 3.2) Si NO es el último comando, redirijo stdout al pipe[i][1]
                if (i < command_count - 1) {
                    if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }

                // 3.3) Cerrar todos los extremos de todos los pipes en el hijo
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }

                // 3.4) Parsear la subcadena commands[i] en argv[], respetando comillas
                char *argv[MAX_ARGS];
                int argc = parse_args_respecting_quotes(commands[i], argv);
                if (argc == -1) {
                    // comillas sin cerrar dentro de este comando (sí debería haber sido capturado antes,
                    // pero chequeamos de nuevo por si acaso)
                    fprintf(stderr, "Error de sintaxis: comilla sin cierre en '%s'\n", commands[i]);
                    exit(EXIT_FAILURE);
                }
                if (argc == -2) {
                    // Exceso de tokens
                    fprintf(stderr, "Error: exceso de argumentos en '%s' (máximo %d)\n",
                            commands[i], MAX_ARGS - 1);
                    exit(EXIT_FAILURE);
                }
                if (argc == 0) {
                    fprintf(stderr, "Error: comando vacío en posición %d\n", i);
                    exit(EXIT_FAILURE);
                }

                // 3.5) Ejecutar el comando usando execvp
                execvp(argv[0], argv);
                // Si execvp regresa, hubo error
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            // El padre sigue creando hijos
        }

        // 4) En el padre: cerrar todos los extremos de pipe
        for (int i = 0; i < command_count - 1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }

        // 5) Esperar a que terminen todos los hijos
        for (int i = 0; i < command_count; i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }

    fin_iteracion:
        ; // etiqueta para saltar aquí tras errores de forkeos/pipes en el padre
        command_count = 0;
    }

    return 0;
}
