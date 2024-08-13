#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define BUFFER_SIZE 1048576 

typedef struct Node {
    char *data;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ConcurrentQueue;

typedef struct {
    int threadIndex;
    ConcurrentQueue *queue;
    pthread_mutex_t *isWritingCompleteMutex;
    int *isWritingComplete;
} ThreadData;

void initQueue(ConcurrentQueue *queue);
void enqueue(ConcurrentQueue *queue, const char *data);
char *dequeue(ConcurrentQueue *queue);
void destroyQueue(ConcurrentQueue *queue);

void setLoggingParameters(const char *filePath, int *numberOfWrites, int *numberOfThreads);
void startLogging(void);
void logtoFile(void);

#endif // QUEUE_H
