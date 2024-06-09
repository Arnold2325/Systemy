#include "shared_memory.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

#define SHM_KEY 1234
#define SHM_SIZE 4096

int create_shared_memory() {
    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

char* attach_shared_memory(int shmid) {
    char *shmaddr = (char*) shmat(shmid, NULL, 0);
    if (shmaddr == (char*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return shmaddr;
}

void detach_shared_memory(char *shmaddr) {
    if (shmdt(shmaddr) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
}

void remove_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
}

