#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

volatile sig_atomic_t running = 1;
volatile sig_atomic_t pause_flag = 0;
volatile sig_atomic_t continue_flag = 0;

void handle_signal(int signal) {
    if (signal == SIGUSR1) {
        running = 0;
    } else if (signal == SIGUSR2) {
        printf("Process 3 received SIGUSR2\n");
        pause_flag = 1;
    } else if (signal == SIGCONT) {
        printf("Process 3 received SIGCONT\n");
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

    printf("Process 3 started\n");

    while (running) {
        printf("Process 3 waiting for data\n");
        pause(); // Czekaj na sygnał

        if (pause_flag) {
            printf("Process 3 pausing\n");
            pause_flag = 0; // Reset flagi
            pause(); // Wstrzymaj proces
        }

        if (continue_flag) {
            printf("Process 3 continuing\n");
            continue_flag = 0; // Reset flagi
        }

        if (running) {
            printf("Process 3 resuming after signal\n");

            char buffer[1024];
            ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Process 3 received: %s\n", buffer);

                // Split hex data into lines of 15 characters
                int count = 0;
                for (char *token = strtok(buffer, " "); token; token = strtok(NULL, " ")) {
                    printf("%s ", token);
                    count++;
                    if (count == 15) {
                        printf("\n");
                        count = 0;
                    }
                }
                if (count > 0) {
                    printf("\n");
                }

                // Wysyłamy sygnał SIGUSR1 do procesu 2 po przetworzeniu danych
                char *pid2_str = getenv("PID2");
                if (pid2_str) {
                    printf("Process 3 found PID2: %s\n", pid2_str);
                    pid_t pid2 = (pid_t) atoi(pid2_str);
                    printf("Process 3 converting PID2 to int: %d\n", pid2);
                    if (kill(pid2, SIGUSR1) == -1) {
                        perror("Error sending SIGUSR1 to Process 2");
                    } else {
                        printf("Process 3 sent SIGUSR1 to Process 2 (PID: %d)\n", pid2);
                    }
                } else {
                    printf("Error: PID2 environment variable not set\n");
                }
            }
        }
    }

    return 0;
}

