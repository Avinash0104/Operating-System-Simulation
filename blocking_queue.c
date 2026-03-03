#include "blocking_queue.h"

struct node {
    unsigned int value;
    struct node* next;
    struct node* prev;
};

// to gracefully shut down queue and unblock waiting threads
void blocking_queue_terminate(BlockingQueueT* queue) {
  pthread_mutex_lock(&queue->mutex);
  queue->terminated = 1;
  pthread_mutex_unlock(&queue->mutex);
  sem_post(&queue->sem);
}

// create blocking queue
void blocking_queue_create(BlockingQueueT* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
    queue->terminated = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    sem_init(&queue->sem, 0, 0);
}

// destroy blocking queue
void blocking_queue_destroy(BlockingQueueT* queue) {
    pthread_mutex_lock(&queue->mutex);

    struct node* current = queue->head;
    while(current != NULL){
        struct node* temp = current;
        current = current->next;
        checked_free(temp);
    }

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    sem_destroy(&queue->sem);
}

// push a new node with value to blocking queue
void blocking_queue_push(BlockingQueueT* queue, unsigned int value) {
    struct node* new = (struct node*) checked_malloc(sizeof(struct node));
    new->value = value;
    new->next = NULL;
    new->prev = NULL;

    pthread_mutex_lock(&queue->mutex);

    if (queue->terminated){
      pthread_mutex_unlock(&queue->mutex);
      checked_free(new);
      return;
    }

    new->prev = queue->tail;
    if (queue->tail == NULL) {
        queue->head = new;
        queue->tail = new;
    } else {
        queue->tail->next = new;
        queue->tail = new;
    }
    queue->length++;

    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->sem);
}

// remove a node from the blocking queue
int blocking_queue_pop(BlockingQueueT* queue, unsigned int* value) {

    sem_wait(&queue->sem);
    pthread_mutex_lock(&queue->mutex);

    if (queue->terminated && queue->length == 0) {
        pthread_mutex_unlock(&queue->mutex);
        sem_post(&queue->sem);
        return 1;
    }

    struct node* temp = queue->head;
    *value = queue->head->value;
    queue-> head = queue->head->next;
    if (queue->head != NULL) {
        queue->head->prev = NULL;
    } else {
        queue->tail = NULL;
    }
    queue->length--;
    checked_free(temp);

    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

// check if blocking queue is empty
int blocking_queue_empty(BlockingQueueT* queue) {
  int is_empty;
  pthread_mutex_lock(&queue->mutex);
  is_empty = (queue->length == 0);
  pthread_mutex_unlock(&queue->mutex);
  return is_empty;
}

// return length of blocking queue
int blocking_queue_length(BlockingQueueT* queue) {
  int len;
  pthread_mutex_lock(&queue->mutex);
  len = queue->length;
  pthread_mutex_unlock(&queue->mutex);
  return len;
}
