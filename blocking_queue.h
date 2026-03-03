#ifndef _BLOCKING_QUEUE_H_
#define _BLOCKING_QUEUE_H_

#include "list.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "utilities.h"

typedef struct BlockingQueue {
  struct node* head;
  struct node* tail;
  unsigned int length;
  pthread_mutex_t mutex;
  int terminated;
  sem_t sem;
} BlockingQueueT;

void blocking_queue_create(BlockingQueueT* queue);
void blocking_queue_destroy(BlockingQueueT* queue);

void blocking_queue_push(BlockingQueueT* queue, unsigned int value);
int blocking_queue_pop(BlockingQueueT* queue, unsigned int* value);

int blocking_queue_empty(BlockingQueueT* queue);
int blocking_queue_length(BlockingQueueT* queue);

void blocking_queue_terminate(BlockingQueueT* queue);

#endif
