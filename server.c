#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_THREADS 4
#define QUEUE_SIZE  32
#define BUFFER_SIZE 1024

/* ---------------- THREAD POOL ---------------- */

typedef struct {
    int client_fd;
} task_t;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t threads[MAX_THREADS];
    task_t queue[QUEUE_SIZE];
    int head, tail, count;
} threadpool_t;

threadpool_t pool;

/* Worker thread */
void *worker(void *arg) {
    while (1) {
        pthread_mutex_lock(&pool.lock);

        while (pool.count == 0)
            pthread_cond_wait(&pool.notify, &pool.lock);

        task_t task = pool.queue[pool.head];
        pool.head = (pool.head + 1) % QUEUE_SIZE;
        pool.count--;

        pthread_mutex_unlock(&pool.lock);

        /* ---- HANDLE CLIENT ---- */
        char buffer[BUFFER_SIZE];
        int fd = task.client_fd;

        while (1) {
            int n = read(fd, buffer, BUFFER_SIZE - 1);
            if (n <= 0) {
                printf("Client %d disconnected\n", fd);
                close(fd);
                break;
            }
            buffer[n] = '\0';
            printf("[Client %d]: %s", fd, buffer);

            write(fd, buffer, n);  // echo back
        }
    }
    return NULL;
}

/* Initialize thread pool */
void threadpool_init() {
    pool.head = pool.tail = pool.count = 0;
    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.notify, NULL);

    for (int i = 0; i < MAX_THREADS; i++)
        pthread_create(&pool.threads[i], NULL, worker, NULL);
}

/* Add task */
void threadpool_add(int client_fd) {
    pthread_mutex_lock(&pool.lock);

    pool.queue[pool.tail].client_fd = client_fd;
    pool.tail = (pool.tail + 1) % QUEUE_SIZE;
    pool.count++;

    pthread_cond_signal(&pool.notify);
    pthread_mutex_unlock(&pool.lock);
}

/* ---------------- MAIN SERVER ---------------- */

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    threadpool_init();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("Server listening on port %d\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        printf("New client connected: %d\n", client_fd);

        threadpool_add(client_fd);
    }

    close(server_fd);
    return 0;
}
