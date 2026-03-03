#include "non_blocking_queue.h"
#include "utilities.h"

#include <pthread.h>

struct node {
    unsigned int value;
    struct node* next;
    struct node* prev;
};

// create non-blocking queue
void non_blocking_queue_create(NonBlockingQueueT* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
    pthread_mutex_init(&queue->mutex, NULL);
}

// destroy non-blocking queue and cleanup memory
void non_blocking_queue_destroy(NonBlockingQueueT* queue) {
    pthread_mutex_lock(&queue->mutex);

    struct node* current = queue->head;
    while(current != NULL){
        struct node* temp = current;
        current = current->next;
        checked_free(temp);
    }

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
}

// add a new node with value to non-blocking queue
void non_blocking_queue_push(NonBlockingQueueT* queue, unsigned int value) {
    
    struct node* new = (struct node*) checked_malloc(sizeof(struct node));
    new->value = value;
    new->next = NULL;
    new->prev = NULL;

    pthread_mutex_lock(&queue->mutex);

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
}

// remove a node from non-blocking queue
int non_blocking_queue_pop(NonBlockingQueueT* queue, unsigned int* value) {
    pthread_mutex_lock(&queue->mutex);

    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->mutex);
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

// check if non-blocking queue is empty
int non_blocking_queue_empty(NonBlockingQueueT* queue) {
  int is_empty;
  pthread_mutex_lock(&queue->mutex);
  is_empty = (queue->length == 0);
  pthread_mutex_unlock(&queue->mutex);
  return is_empty;
}

// return length of non-blocking queue
int non_blocking_queue_length(NonBlockingQueueT* queue) {
  int len;
  pthread_mutex_lock(&queue->mutex);
  len = queue->length;
  pthread_mutex_unlock(&queue->mutex);
  return len;
}
