#include "priority_queue.h"
#include "utilities.h"

// priority level is in descending order: 0 has highest priority

struct node {
  unsigned int value;
  struct node* next;
  struct node* prev;
};

// create priority queue with specified number of levels
void priority_queue_create(PriorityQueueT* queue, unsigned int number_of_queues) {
  queue->length = 0;
  queue->num_levels = number_of_queues;
  queue->terminated = 0;

  for (int i = 0; i < number_of_queues; i++){
    queue->levels[i].head = NULL;
    queue->levels[i].tail = NULL;
  }

  pthread_mutex_init(&queue->mutex, NULL);
  sem_init(&queue->sem, 0, 0);
}

// destroy priority queue and cleanup memory
void priority_queue_destroy(PriorityQueueT* queue) {
  pthread_mutex_lock(&queue->mutex);

  for (int i = 0; i < queue->num_levels; i++){
    struct node* current = queue->levels[i].head;
    while (current != NULL) {
      struct node* temp = current;
      current = current->next;
      checked_free(temp);
    }
  }
  queue->length = 0;
  pthread_mutex_unlock(&queue->mutex);
  pthread_mutex_destroy(&queue->mutex);
  sem_destroy(&queue->sem);
}

// push a new node with value to priority queue at specified level
void priority_queue_push(PriorityQueueT* queue, PriorityT level, unsigned int value) {
  if (level >= queue->num_levels){
    return;
  }

  struct node* new = (struct node*) checked_malloc (sizeof(struct node));
  new->value = value;
  new->next = NULL;
  new->prev = NULL;

  pthread_mutex_lock(&queue->mutex);

  if (queue->terminated){
    pthread_mutex_unlock(&queue->mutex);
    checked_free(new);
    return;
  }

  QueueLevel* list = &queue->levels[level];
  if (list->tail == NULL){
    list->head = new;
    list->tail = new;
  } else {
    list->tail->next = new;
    new->prev = list->tail;
    list->tail = new;
  }

  queue->length++;
  pthread_mutex_unlock(&queue->mutex);
  sem_post(&queue->sem);

}

// remove a node from priority queue at highest priority level
int priority_queue_pop(PriorityQueueT* queue, unsigned int *value) {
  sem_wait(&queue->sem);
  pthread_mutex_lock(&queue->mutex);

  if (queue->terminated && queue->length == 0) {
    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->sem);
    return 1;
  }

  for (int i = 0; i < queue->num_levels; i++){
    if (queue->levels[i].head != NULL){

      struct node* temp = queue->levels[i].head;
      *value = temp->value;
      queue->levels[i].head = temp->next;
      if (queue->levels[i].head != NULL) {
          queue->levels[i].head->prev = NULL;
      } else {
          queue->levels[i].tail = NULL;
      }
      queue->length--;
      checked_free(temp);

      pthread_mutex_unlock(&queue->mutex);
      return 0;
    }
  }

  pthread_mutex_unlock(&queue->mutex);
  return 0;
}

// check if priority queue is empty
int priority_queue_empty(PriorityQueueT* queue) {
  int is_empty;
  pthread_mutex_lock(&queue->mutex);
  is_empty = (queue->length == 0);
  pthread_mutex_unlock(&queue->mutex);
  return is_empty;
}

// return length of priority queue
int priority_queue_length(PriorityQueueT* queue) {
  int len;
  pthread_mutex_lock(&queue->mutex);
  len = queue->length;
  pthread_mutex_unlock(&queue->mutex);
  return len;
}

// to gracefully shut down queue and unblock waiting threads
void priority_queue_terminate(PriorityQueueT* queue) {
  pthread_mutex_lock(&queue->mutex);
  queue->terminated = 1;
  pthread_mutex_unlock(&queue->mutex);
  sem_post(&queue->sem);
}
