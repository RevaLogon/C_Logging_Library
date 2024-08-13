#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

static ConcurrentQueue queue;
static pthread_mutex_t isWritingCompleteMutex;
static int isWritingComplete;
static char *filePath = "/home/kuzu/Desktop/output3.txt";
static int numberOfWrites = 100000;
static int numberOfThreads = 10;

void initQueue(ConcurrentQueue *queue) {
    queue->front = queue->rear = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

void enqueue(ConcurrentQueue *queue, const char *data) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->data = strdup(data);
    node->next = NULL;

    pthread_mutex_lock(&queue->mutex);
    if (queue->rear == NULL) {
        queue->front = queue->rear = node;
    } else {
        queue->rear->next = node;
        queue->rear = node;
    }
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

char *dequeue(ConcurrentQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->front == NULL) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    Node *node = queue->front;
    char *data = node->data;
    queue->front = node->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    pthread_mutex_unlock(&queue->mutex);
    free(node);
    return data;
}

void destroyQueue(ConcurrentQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    Node *node = queue->front;
    while (node) {
        Node *temp = node;
        node = node->next;
        free(temp->data);
        free(temp);
    }
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
}

void *worker(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = 0; i < numberOfWrites; i++) {
        char logMessage[256];
        snprintf(logMessage, sizeof(logMessage), "Thread %d :: %ld - Write %d - : %ld\n", 
                 data->threadIndex, (long)getpid(), i + 1, time(NULL));
        enqueue(data->queue, logMessage);
    }
    printf("Thread %d completed its work.\n", data->threadIndex);
    free(data);
    return NULL;
}

void *fileWriter(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    char buffer[BUFFER_SIZE];
    size_t bufferPos = 0;

    while (1) {
        char *logMessage = dequeue(data->queue);
        if (logMessage) {
            size_t len = strlen(logMessage);
            if (bufferPos + len >= BUFFER_SIZE) {
                write(fd, buffer, bufferPos);
                bufferPos = 0;
            }
            memcpy(buffer + bufferPos, logMessage, len);
            bufferPos += len;
            free(logMessage);
        }
        pthread_mutex_lock(data->isWritingCompleteMutex);
        if (*data->isWritingComplete && data->queue->front == NULL) {
            pthread_mutex_unlock(data->isWritingCompleteMutex);
            break;
        }
        pthread_mutex_unlock(data->isWritingCompleteMutex);
    }

    if (bufferPos > 0) {
        write(fd, buffer, bufferPos);
    }

    close(fd);
    return NULL;
}

void setLoggingParameters(const char *path, int *writes, int *threads) {

    if(*writes>200000 || *threads>10 ){
        int wr,th;
        printf("Inappropriate parameters\n");
        printf("Enter number of writes (max 200.000): ");
        scanf("%d", &wr);
        printf("Enter number of threads (max 10): ");   
        scanf("%d", &th);
        filePath = strdup(path);
        numberOfWrites = wr;
        numberOfThreads = th;
    }
    else{
    filePath = strdup(path);
    numberOfWrites = *writes;
    numberOfThreads = *threads;
    }
}

void startLogging(void) {
    pthread_t threads[numberOfThreads], fileWriterThread;

    initQueue(&queue);
    pthread_mutex_init(&isWritingCompleteMutex, NULL);
    isWritingComplete = 0;

    for (int i = 0; i < numberOfThreads; i++) {
        ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
        data->threadIndex = i;
        data->queue = &queue;
        data->isWritingCompleteMutex = &isWritingCompleteMutex;
        data->isWritingComplete = &isWritingComplete;
        pthread_create(&threads[i], NULL, worker, data);
    }

    ThreadData fileWriterData = { -1, &queue, &isWritingCompleteMutex, &isWritingComplete };
    pthread_create(&fileWriterThread, NULL, fileWriter, &fileWriterData);

    for (int i = 0; i < numberOfThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_lock(&isWritingCompleteMutex);
    isWritingComplete = 1;
    pthread_cond_broadcast(&queue.cond);
    pthread_mutex_unlock(&isWritingCompleteMutex);

    pthread_join(fileWriterThread, NULL);

    destroyQueue(&queue);
    pthread_mutex_destroy(&isWritingCompleteMutex);
    free(filePath);
}

void logtoFile(void) {
    struct timeval start, end;
    gettimeofday(&start, NULL);

    startLogging();

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0;
    elapsed += (end.tv_usec - start.tv_usec) / 1000.0;

    printf("Total execution time: %.2f ms\n", elapsed);
    printf("All threads have finished executing. Timestamp written to file.\n");
}
