#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

#define MAX_C 200
#define MAX_A 65

int parse_args(char *s, char **argv) {
    int argc = 0;
    char *p = s;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        if (argc >= MAX_A - 1) return -2;
        if (*p == '"' || *p == '\'') {
            char q = *p; p++;
            char *st = p;
            while (*p && *p != q) p++;
            if (*p != q) return -1;
            *p = '\0';
            argv[argc++] = st;
            p++;
        } else {
            char *st = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            if (*p) { *p = '\0'; p++; }
            argv[argc++] = st;
        }
    }
    argv[argc] = NULL;
    return argc;
}

int main() {
    char line[4096], *cmds[MAX_C];
    while (1) {
        if (isatty(0)) { printf("Shell> "); fflush(stdout); }
        if (!fgets(line, sizeof(line), stdin)) {
            if (isatty(0)) printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = 0;

        char *t0 = line;
        while (*t0 && isspace((unsigned char)*t0)) t0++;
        if (!*t0) continue;

        char tmp[4096];
        strcpy(tmp, line);
        char *r = tmp;
        while (*r && isspace((unsigned char)*r)) r++;
        char *e = r + strlen(r) - 1;
        while (e > r && isspace((unsigned char)*e)) *e-- = 0;
        if (strcmp(r, "exit") == 0) exit(0);

        int qc = 0, bad = 0;
        for (char *p = line; *p; p++) {
            if (*p == '"' || *p == '\'') qc++;
            if (*p == '|' && !p[1]) { bad = 1; break; }
            if (p[1] == '|' && (*p == '|' || isspace((unsigned char)p[-1]))) { bad = 1; break; }
        }
        if (bad || (qc % 2)) {
            if (qc % 2) fprintf(stderr, "Error de sintaxis: comillas sin cerrar\n");
            else        fprintf(stderr, "Error de sintaxis\n");
            continue;
        }

        int n = 0;
        char quote = 0;
        char *p = line, *start = line;
        for (; *p; p++) {
            if (quote) {
                if (*p == quote) quote = 0;
            } else {
                if (*p == '"' || *p == '\'') quote = *p;
                else if (*p == '|') {
                    *p = '\0';
                    char *a = start;
                    while (*a && isspace((unsigned char)*a)) a++;
                    char *b = a + strlen(a) - 1;
                    while (b > a && isspace((unsigned char)*b)) *b-- = 0;
                    if (!*a) { fprintf(stderr, "Error de sintaxis: comando vacío\n"); n = -1; break; }
                    cmds[n++] = a;
                    start = p + 1;
                }
            }
        }
        if (n < 0) continue;

        char *a2 = start;
        while (*a2 && isspace((unsigned char)*a2)) a2++;
        char *b2 = a2 + strlen(a2) - 1;
        while (b2 > a2 && isspace((unsigned char)*b2)) *b2-- = 0;
        if (!*a2) { fprintf(stderr, "Error de sintaxis: comando vacío\n"); continue; }
        cmds[n++] = a2;

        if (n > MAX_C) { fprintf(stderr, "Error: máximo %d comandos\n", MAX_C); continue; }

        int fd[MAX_C - 1][2];
        for (int i = 0; i < n - 1; i++) {
            if (pipe(fd[i]) < 0) {
                perror("pipe");
                for (int j = 0; j < i; j++) {
                    close(fd[j][0]); close(fd[j][1]);
                }
                n = 0;
                goto end_loop;
            }
        }

        pid_t pid[MAX_C];
        for (int i = 0; i < n; i++) {
            if ((pid[i] = fork()) < 0) {
                perror("fork");
                for (int k = 0; k < n - 1; k++) {
                    close(fd[k][0]); close(fd[k][1]);
                }
                n = 0;
                goto end_loop;
            }
            if (pid[i] == 0) {
                if (i > 0)  dup2(fd[i - 1][0], 0);
                if (i < n - 1) dup2(fd[i][1], 1);
                for (int j = 0; j < n - 1; j++) {
                    close(fd[j][0]); close(fd[j][1]);
                }
                char *argv[MAX_A];
                int argc = parse_args(cmds[i], argv);
                if (argc < 0) {
                    if (argc == -1) fprintf(stderr, "Error de sintaxis: comillas sin cerrar\n");
                    else            fprintf(stderr, "Error: exceso de argumentos\n");
                    exit(1);
                }
                if (!strcmp(argv[0], "exit")) exit(0);
                execvp(argv[0], argv);
                perror("execvp");
                exit(1);
            }
        }

        for (int i = 0; i < n - 1; i++) {
            close(fd[i][0]); close(fd[i][1]);
        }
        for (int i = 0; i < n; i++) {
            waitpid(pid[i], NULL, 0);
        }

    end_loop:
        n = 0;
    }
    return 0;
}
