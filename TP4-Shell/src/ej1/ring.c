#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv){
    if(argc != 4){
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int n = atoi(argv[1]);
    int c = atoi(argv[2]);
    int s = atoi(argv[3]);
    if(n < 3 || s < 1 || s > n){
        fprintf(stderr, "Error: n>=3 y 1<=s<=n\n");
        exit(EXIT_FAILURE);
    }
    int pipes[n][2];
    for(int i = 0; i < n; i++)
        if(pipe(pipes[i]) < 0){ perror("pipe"); exit(EXIT_FAILURE); }

    for(int i = 0; i < n; i++){
        pid_t pid = fork();
        if(pid < 0){
            perror("fork"); exit(EXIT_FAILURE);
        } else if(pid == 0){
            int idx  = i;
            int pred = (idx - 1 + n) % n;
            for(int j = 0; j < n; j++){
                if(j == idx)        close(pipes[j][0]);
                else if(j == pred)  close(pipes[j][1]);
                else{
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }
            int val;
            if(idx == s - 1){
                read (pipes[pred][0], &val, sizeof(val));
                write(pipes[idx][1], &val, sizeof(val));
                read (pipes[pred][0], &val, sizeof(val));
                printf("Hijo %d: Leyó valor %d\n", idx+1, val);
            } else {
                read (pipes[pred][0], &val, sizeof(val));
                printf("Hijo %d: Leyó valor %d\n", idx+1, val);
                val++;
                write(pipes[idx][1], &val, sizeof(val));
            }
            exit(EXIT_SUCCESS);
        }
    }
    int start_pipe = ( (s-1) - 1 + n ) % n;
    printf("Padre: Enviando valor %d al proceso %d\n", c, s);
    write(pipes[start_pipe][1], &c, sizeof(c));
    for(int j = 0; j < n; j++){
        close(pipes[j][0]);
        close(pipes[j][1]);
    }
    for(int j = 0; j < n; j++)
        wait(NULL);

    return 0;
}
