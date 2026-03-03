#ifndef _NON_BLOCKING_QUEUE_SET_H_
#define _NON_BLOCKING_QUEUE_SET_H_

#include "list.h"
#include <pthread.h>
#include <semaphore.h>
#define MAX_PRIORITY 16

struct node;
typedef unsigned int PriorityT;

typedef struct {
  struct node* head;
  struct node* tail;
} QueueLevel;

typedef struct {
  int length;
  int terminated;
  int num_levels;
  QueueLevel levels[MAX_PRIORITY];
  pthread_mutex_t mutex;
  sem_t sem;
} PriorityQueueT;

/* Standard interface, do not change*/
void priority_queue_create(PriorityQueueT* priority_queue, unsigned int number_of_queues);
void priority_queue_destroy(PriorityQueueT* queue);

void priority_queue_push(PriorityQueueT* priority_queue, PriorityT level, unsigned int value);
int priority_queue_pop(PriorityQueueT* queue, unsigned int* value);

int priority_queue_empty(PriorityQueueT* queue);
int priority_queue_length(PriorityQueueT* queue);

void priority_queue_terminate(PriorityQueueT* priority_queue);
/* End of standard interface */

/* You may add functions to the queue interface if required */

#endif
