#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#define SHM_SIZE 4096

int create_shared_memory();
char* attach_shared_memory(int shmid);
void detach_shared_memory(char *shmaddr);
void remove_shared_memory(int shmid);

#endif // SHARED_MEMORY_H

