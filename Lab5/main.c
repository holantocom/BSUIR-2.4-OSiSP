#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>

sem_t semaphore;

static int fd[2];
static pthread_t thread;

void* worker1(void* args) {
    for (int i = 0; i < 75; i++) {
        sem_wait(&semaphore);
        char string[30];
        sprintf (string, "%d. (1) ID: %d; %ld", i, pthread_self(), clock());
        write(fd[1], string, 30);
        sem_post(&semaphore);
    }
    kill(thread, SIGUSR1);
}

void* worker2(void* args) {
    for (int i = 0; i < 75; i++) {
        sem_wait(&semaphore);
        char string[30];
        sprintf (string, "%d. (2) ID: %d; %ld", i, pthread_self(), clock());
        write(fd[1], string, 30);
        sem_post(&semaphore);
    }
    kill(thread, SIGUSR1);
}

void* worker(void* args) {

    char resstring[30];
    pthread_t thread1, thread2;

    sem_init(&semaphore, 0, 1);
    pipe(fd);

    for(int i=0; i < 10; i++) {
        pthread_create(&thread1, NULL, worker1, NULL);
        pthread_create(&thread2, NULL, worker2, NULL);
        kill(thread1, SIGUSR2);
        kill(thread2, SIGUSR2);

        printf("===%d===\n", i);
        for (int j = 0; j < 10; j++) {
            sem_wait(&semaphore);
            read(fd[0], resstring, 30);
            printf("%s\n", resstring);
            sem_post(&semaphore);
        }

        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
    }

    close(fd[0]);
    close(fd[1]);

    sem_destroy(&semaphore);

}
void my_handler(int signum)
{}

int main() {
    signal(SIGUSR1, my_handler);
    signal(SIGUSR2, my_handler);

    pthread_create(&thread, NULL, worker, NULL);
    pthread_join(thread, NULL);

    return 0;
}