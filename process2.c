#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "shared_memory.h"

volatile sig_atomic_t running = 1;
volatile sig_atomic_t pause_flag = 0;
volatile sig_atomic_t continue_flag = 0;
volatile sig_atomic_t ready_to_send = 0;

void handle_signal(int signal) {
    if (signal == SIGUSR1) {
        ready_to_send = 1; // Proces 3 gotowy do odbioru
        printf("Process 2 received SIGUSR1, ready to send data\n");
    } else if (signal == SIGUSR2) {
        printf("Process 2 received SIGUSR2\n");
        pause_flag = 1;
    } else if (signal == SIGCONT) {
        printf("Process 2 received SIGCONT\n");
        continue_flag = 1;
    }
}

void convert_to_hex(const char *input, char *output) {
    while (*input) {
        sprintf(output, "%02x ", (unsigned char) *input);
        output += 3;
        input++;
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

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int shmid = create_shared_memory();
    char *shmaddr = attach_shared_memory(shmid);

    pid_t pid3 = fork();
    if (pid3 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid3 == 0) { // Proces potomny (Proces 3)
        close(pipefd[1]); // Zamykamy koniec zapisu w procesie potomnym
        dup2(pipefd[0], STDIN_FILENO); // Przekierowanie wejścia z pipe
        execlp("./process3", "process3", NULL);
        perror("execlp"); // Jeśli execlp się nie uda
        exit(EXIT_FAILURE);
    } else { // Proces macierzysty (Proces 2)
        close(pipefd[0]); // Zamykamy koniec odczytu w procesie macierzystym
        printf("Process 2 started\n");
        while (running) {
            if (pause_flag) {
                printf("Process 2 paused\n");
                pause(); // Czekaj na wznowienie
                pause_flag = 0;
            }
            if (continue_flag) {
                printf("Process 2 resuming\n");
                continue_flag = 0;
            }

            printf("Process 2 waiting for data\n");
            char hex_data[SHM_SIZE * 3];
            printf("Process 2 received data: %s\n", shmaddr);
            convert_to_hex(shmaddr, hex_data);
            printf("Process 2 converted to hex: %s\n", hex_data);

            // Wysyłamy sygnał SIGCONT do procesu 3, aby przygotował się na odbiór danych
            kill(pid3, SIGCONT);
            printf("Process 2 sent SIGCONT to Process 3\n");

            // Czekaj na sygnał od procesu 3, że jest gotowy na odbiór danych
            while (!ready_to_send) {
                printf("Process 2 waiting for SIGUSR1 from Process 3\n");
                sleep(1); // Dodanie opóźnienia, aby upewnić się, że sygnał może być odbierany
            }
            ready_to_send = 0; // Reset flagi

            // Wyślij dane do procesu 3
            write(pipefd[1], hex_data, strlen(hex_data) + 1);
            printf("Process 2 sent data to Process 3\n");

            // Przejdź w stan pauzy po zakończeniu pracy
            pause();
        }
        close(pipefd[1]);
        detach_shared_memory(shmaddr);
    }

    return 0;
}

