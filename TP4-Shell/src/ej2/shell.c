#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200
#define MAX_ARGS     50   
int main() {
    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count;

    while (1) {
        if (isatty(STDIN_FILENO)) { 
            printf("Shell> ");
        }
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\n");
            break;
        }

        command[strcspn(command, "\n")] = '\0';

        if (command[0] == '\0') {
            continue;
        }

        command_count = 0;
        char *token = strtok(command, "|");
        while (token != NULL && command_count < MAX_COMMANDS) {
            while (*token == ' ') token++;
            char *end = token + strlen(token) - 1;
            while (end > token && (*end == ' ')) {
                *end = '\0';
                end--;
            }
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        if (command_count == 0) {
            continue;
        }

        int pipefd[MAX_COMMANDS - 1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipefd[i]) == -1) {
                perror("pipe");
                for (int j = 0; j < i; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                command_count = 0;
                goto end_iteration; 
            }
        }

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
                goto end_iteration;
            }

            if (pids[i] == 0) {

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

                for (int j = 0; j < command_count - 1; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }

                char *argv[MAX_ARGS];
                int argc = 0;
                char *arg = strtok(commands[i], " ");
                while (arg != NULL && argc < (MAX_ARGS - 1)) {
                    argv[argc++] = arg;
                    arg = strtok(NULL, " ");
                }
                argv[argc] = NULL; 

                if (argc == 0) {
                    if (isatty(STDIN_FILENO)) { 
                        fprintf(stderr, "Error: comando vacío en posición %d\n", i);
                        }
                    exit(EXIT_FAILURE);
                }
                execvp(argv[0], argv);
                perror("execvp");
                exit(EXIT_FAILURE);
            }

        }

        for (int i = 0; i < command_count - 1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }

        for (int i = 0; i < command_count; i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }

    end_iteration:
        command_count = 0;
    }

    return 0;
}
