#include "blocking_queue.h"
#include "utilities.h"

#include <assert.h>

#define NUM_PRODUCERS 4
#define NUM_CONSUMERS 4
#define ITEMS_PER_PRODUCER 1000

// testing creating and destroying blocking queue
void test_queue_creation(){
  BlockingQueueT queue;
  blocking_queue_create(&queue);
  assert(blocking_queue_empty(&queue));
  assert(blocking_queue_length(&queue) == 0);
  blocking_queue_destroy(&queue);
}

// testing adding and removing nodes from blocking queue
void test_push_and_pop(){
  BlockingQueueT queue;
  blocking_queue_create(&queue);

  blocking_queue_push(&queue, 25);
  assert(!blocking_queue_empty(&queue));

  blocking_queue_push(&queue, 30);
  assert(blocking_queue_length(&queue) == 2);

  unsigned int value1;
  unsigned int value2;
  unsigned int value3;

  blocking_queue_pop(&queue, &value1);
  assert(value1 == 25);
  assert(!blocking_queue_empty(&queue));

  blocking_queue_pop(&queue, &value2);
  assert(value2 == 30);
  assert(blocking_queue_empty(&queue));

  blocking_queue_destroy(&queue);
}

BlockingQueueT test_queue;
unsigned int test_result;

void* blocking_pop_thread(void* arg){
  int success = blocking_queue_pop(&test_queue, &test_result);
  assert(success == 0);
  return NULL;
}

// test blocking mechanism of blocking queue
void test_blocking(){
  blocking_queue_create(&test_queue);

  pthread_t consumer;
  pthread_create(&consumer, NULL, blocking_pop_thread, NULL);

  usleep(100 * 1000);

  blocking_queue_push(&test_queue, 40);
  pthread_join(consumer, NULL);
  assert(test_result == 40);

  blocking_queue_destroy(&test_queue);
}

BlockingQueueT queue;

// producers add items to the queue
void* producer_thread(void* arg){
  for (int i = 0; i < ITEMS_PER_PRODUCER; i++){
    blocking_queue_push(&queue, i);
  }
  return NULL;
}

// consumers remove items from the queue
void* consumer_thread(void* arg){
  long pop_count = 0;
  unsigned int value;
  while (1){
    int pop_val = blocking_queue_pop(&queue, &value);
    if (!pop_val){
      pop_count++;
    } else {
      break;
    }
  }
  long* return_value = malloc(sizeof(long));
  assert(return_value != NULL);
  *return_value = pop_count;
  return (void*) return_value; // return number of items popped
}


int main() {
  pthread_t producers[NUM_PRODUCERS];
  pthread_t consumers[NUM_CONSUMERS];

  test_queue_creation();
  test_push_and_pop();
  test_blocking();

  // create queue and start producers and consumers

  blocking_queue_create(&queue);

  for (int i = 0; i < NUM_CONSUMERS; i++){
    pthread_create(&consumers[i], NULL, consumer_thread, NULL);
  }

  for (int i = 0; i < NUM_PRODUCERS; i++){
    pthread_create(&producers[i], NULL, producer_thread, NULL);
  }

  // wait for producers to finish
  for (int i = 0; i < NUM_PRODUCERS; i++){
    pthread_join(producers[i], NULL);
  }
  blocking_queue_terminate(&queue);

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

  // check number of items produced and consumed are same, then destroy queue
  assert(total_popped == NUM_PRODUCERS * ITEMS_PER_PRODUCER);
  blocking_queue_destroy(&queue);

  return 0;
}

