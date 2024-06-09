#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "shared_memory.h"

volatile sig_atomic_t running = 1;
volatile sig_atomic_t pause_flag = 0;
volatile sig_atomic_t continue_flag = 0;

void handle_signal(int signal) {
    if (signal == SIGUSR1) {
        running = 0;
    } else if (signal == SIGUSR2) {
        printf("Main process received SIGUSR2\n");
        pause_flag = 1;
    } else if (signal == SIGCONT) {
        printf("Main process received SIGCONT\n");
        continue_flag = 1;
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGCONT, &sa, NULL);

    printf("Main process started\n");

    pid_t pid1, pid2, pid3;
    pid1 = fork();
    if (pid1 == 0) {
        execlp("./process1", "process1", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        printf("Process 1 started with PID %d\n", pid1);
        pid2 = fork();
        if (pid2 == 0) {
            execlp("./process2", "process2", NULL);
            perror("execlp");
            exit(EXIT_FAILURE);
        } else {
            printf("Process 2 started with PID %d\n", pid2);
            char pid2_str[10];
            sprintf(pid2_str, "%d", pid2);
            setenv("PID2", pid2_str, 1);

            pid3 = fork();
            if (pid3 == 0) {
                execlp("./process3", "process3", NULL);
                perror("execlp");
                exit(EXIT_FAILURE);
            } else {
                printf("Process 3 started with PID %d\n", pid3);

                while (running) {
                    pause(); // Czekaj na sygnały
                    if (pause_flag) {
                        printf("Pausing processes\n");
                        kill(pid1, SIGUSR2);
                        kill(pid2, SIGUSR2);
                        kill(pid3, SIGUSR2);
                        printf("Sent SIGUSR2 to Process 1, 2, and 3\n");
                        pause_flag = 0; // Reset flagi
                    }
                    if (continue_flag) {
                        printf("Continuing processes\n");
                        kill(pid1, SIGCONT);
                        printf("Sent SIGCONT to Process 1\n");
                        sleep(1); // Opóźnienie, aby upewnić się, że procesy odbierają sygnał
                        kill(pid2, SIGCONT);
                        printf("Sent SIGCONT to Process 2\n");
                        sleep(1); // Dodatkowe opóźnienie
                        kill(pid3, SIGCONT);
                        printf("Sent SIGCONT to Process 3\n");
                        continue_flag = 0; // Reset flagi
                    }
                }
                kill(pid1, SIGUSR1);
                kill(pid2, SIGUSR1);
                kill(pid3, SIGUSR1);
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
                waitpid(pid3, NULL, 0);
                int shmid = create_shared_memory();
                remove_shared_memory(shmid);
            }
        }
    }
    exit(EXIT_SUCCESS);
}

