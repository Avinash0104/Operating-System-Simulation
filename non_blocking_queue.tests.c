#include "non_blocking_queue.h"
#include "utilities.h"

#include <assert.h>
#include <unistd.h>

#define NUM_PRODUCERS 4
#define NUM_CONSUMERS 4
#define ITEMS_PER_PRODUCER 1000

// tests creating and destroying queue
void test_queue(){
  NonBlockingQueueT queue;
  non_blocking_queue_create(&queue);
  assert(non_blocking_queue_empty(&queue));
  assert(non_blocking_queue_length(&queue) == 0);
  non_blocking_queue_destroy(&queue);
}

// tests adding and removing items from queue
void test_push_and_pop(){
  NonBlockingQueueT queue;
  non_blocking_queue_create(&queue);
  non_blocking_queue_push(&queue, 25);
  assert(!non_blocking_queue_empty(&queue));
  non_blocking_queue_push(&queue, 30);
  assert(non_blocking_queue_length(&queue) == 2);

  unsigned int value1;
  unsigned int value2;
  non_blocking_queue_pop(&queue, &value1);
  assert(value1 == 25);
  assert(!non_blocking_queue_empty(&queue));
  non_blocking_queue_pop(&queue, &value2);
  assert(value2 == 30);
  assert(non_blocking_queue_empty(&queue));
  non_blocking_queue_destroy(&queue);
}

NonBlockingQueueT queue;

// tests producer thread adding items to queue
void* producer_thread(void* arg){
  for (int i = 0; i < ITEMS_PER_PRODUCER; i++){
    non_blocking_queue_push(&queue, i);
  }
  return NULL;
}

// tests consumer thread removing items from queue
void* consumer_thread(void* arg){
  long pop_count = 0;
  unsigned int value;
  int* producers_done = (int*) arg;
  while (1){
    int pop_val = non_blocking_queue_pop(&queue, &value);
    if (!pop_val){
      pop_count++;
    } else {
      if (*producers_done){
        break;
      }
      usleep(10);
    }
  }
  long* return_value = malloc(sizeof(long));
  *return_value = pop_count;
  return (void*) return_value;
}


int main() {
  pthread_t producers[NUM_PRODUCERS];
  pthread_t consumers[NUM_CONSUMERS];
  int producers_done = 0;

  test_queue();
  test_push_and_pop();

  // create queue and start producer and consumer threads
  non_blocking_queue_create(&queue);
  for (int i = 0; i < NUM_CONSUMERS; i++){
    pthread_create(&consumers[i], NULL, consumer_thread, &producers_done);
  }

  for (int i = 0; i < NUM_PRODUCERS; i++){
    pthread_create(&producers[i], NULL, producer_thread, NULL);
  }

  // wait for producers to finish
  for (int i = 0; i < NUM_PRODUCERS; i++){
    pthread_join(producers[i], NULL);
  }
  producers_done = 1;

  // wake up consumers and collect results
  long total_popped = 0;
  for (int i = 0; i < NUM_CONSUMERS; i++){
    void* result;
    pthread_join(consumers[i], &result);
    long* item_ptr = (long*) result;
    long item = *item_ptr;
    total_popped += item;
    free(item_ptr);
  }

  // // check number of items produced and consumed are same, then destroy queue
  int final_length = non_blocking_queue_length(&queue);
  assert(final_length == 0 && total_popped == NUM_PRODUCERS * ITEMS_PER_PRODUCER);
  non_blocking_queue_destroy(&queue);

  return 0;
}
