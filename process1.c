#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "shared_memory.h"

volatile sig_atomic_t running = 1;

void handle_signal(int signal) {
    if (signal == SIGUSR1) {
        running = 0;
    } else if (signal == SIGUSR2) {
        printf("Process 1 received SIGUSR2\n");
        pause();
    } else if (signal == SIGCONT) {
        printf("Process 1 received SIGCONT\n");
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

    int shmid = create_shared_memory();
    char *shmaddr = attach_shared_memory(shmid);

    printf("Process 1 started\n");
    while (running) {
        ssize_t read_bytes = read(STDIN_FILENO, shmaddr, SHM_SIZE);
        if (read_bytes > 0) {
            shmaddr[read_bytes] = '\0';
            printf("Process 1 read: %s\n", shmaddr);
            char *pid2_str = getenv("PID2");
            if (pid2_str) {
                pid_t pid2 = (pid_t)atoi(pid2_str);
                if (kill(pid2, SIGCONT) == -1) {
                    perror("Error sending SIGCONT to Process 2");
                } else {
                    printf("Process 1 sent SIGCONT to Process 2\n");
                }
            } else {
                printf("Error: PID2 environment variable not set\n");
            }
            pause(); // Czekaj na wznowienie
        }
    }

    detach_shared_memory(shmaddr);
    exit(EXIT_SUCCESS);
}

