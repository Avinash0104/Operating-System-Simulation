#include "priority_queue.h"
#include "utilities.h"

#include <assert.h>
#include <unistd.h>

#define NUM_PRODUCERS 4
#define NUM_CONSUMERS 4
#define NUM_LEVELS 8
#define ITEMS_PER_PRODUCER 1000


void test_queue_creation(){
  PriorityQueueT queue;
  priority_queue_create(&queue, NUM_LEVELS);
  assert(priority_queue_empty(&queue));
  assert(priority_queue_length(&queue) == 0);
  priority_queue_destroy(&queue);
}

void test_push_and_pop(){
  PriorityQueueT queue;
  priority_queue_create(&queue, NUM_LEVELS);

  priority_queue_push(&queue, 3, 25);
  assert(!priority_queue_empty(&queue));

  priority_queue_push(&queue, 1, 30);
  assert(priority_queue_length(&queue) == 2);

  unsigned int value1;
  unsigned int value2;

  priority_queue_pop(&queue, &value1);
  assert(value1 == 30);
  assert(!priority_queue_empty(&queue));

  priority_queue_pop(&queue, &value2);
  assert(value2 == 25);
  assert(priority_queue_empty(&queue));

  priority_queue_push(&queue, 2, 40);
  priority_queue_push(&queue, 2, 50);

  unsigned int value3;
  unsigned int value4;

  priority_queue_pop(&queue, &value3);
  assert(value3 == 40);

  priority_queue_pop(&queue, &value4);
  assert(value4 == 50);

  assert(priority_queue_empty(&queue));
  priority_queue_destroy(&queue);
}

PriorityQueueT test_queue;
unsigned int test_result;
void* blocking_pop_thread(void* arg){
  int success = priority_queue_pop(&test_queue, &test_result);
  assert(success == 0);
  return NULL;
}

void test_blocking(){
  priority_queue_create(&test_queue, NUM_LEVELS);;

  pthread_t consumer;
  pthread_create(&consumer, NULL, blocking_pop_thread, NULL);

  usleep(100 * 1000);

  priority_queue_push(&test_queue, 3, 40);
  pthread_join(consumer, NULL);
  assert(test_result == 40);

  priority_queue_destroy(&test_queue);
}

PriorityQueueT queue;
void* producer_thread(void* arg){
  for (int i = 0; i < ITEMS_PER_PRODUCER; i++){
    unsigned int priority = i % NUM_LEVELS;
    priority_queue_push(&queue, priority, i);
  }
  return NULL;
}

void* consumer_thread(void* arg){
  long pop_count = 0;
  unsigned int value;
  while (1){
    int pop_val = priority_queue_pop(&queue, &value);
    if (!pop_val){
      pop_count++;
    } else {
      break;
    }
  }
  long* return_value = malloc(sizeof(long));
  assert(return_value != NULL);
  *return_value = pop_count;
  return (void*) return_value;
}


int main() {
  pthread_t producers[NUM_PRODUCERS];
  pthread_t consumers[NUM_CONSUMERS];

  test_queue_creation();
  test_push_and_pop();
  test_blocking();

  priority_queue_create(&queue, NUM_LEVELS);
  for (int i = 0; i < NUM_CONSUMERS; i++){
    pthread_create(&consumers[i], NULL, consumer_thread, NULL);
  }

  for (int i = 0; i < NUM_PRODUCERS; i++){
    pthread_create(&producers[i], NULL, producer_thread, NULL);
  }

  for (int i = 0; i < NUM_PRODUCERS; i++){
    pthread_join(producers[i], NULL);
  }
  priority_queue_terminate(&queue);

  long total_popped = 0;
  for (int i = 0; i < NUM_CONSUMERS; i++){
    void* result;
    pthread_join(consumers[i], &result);
    long* item_ptr = (long*) result;
    long item = *item_ptr;
    total_popped += item;
    free(item_ptr);
  }

  assert(total_popped == NUM_PRODUCERS * ITEMS_PER_PRODUCER);
  priority_queue_destroy(&queue);

  return 0;
}

