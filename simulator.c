#include "simulator.h"

// process control block structure - holds process information
typedef struct ProcessControlBlock {
  ProcessIdT id;
  ProcessStateT state;
  EvaluatorCodeT code;
  PriorityT priority;
  sem_t sem_terminated; // used to make thread wait until process terminates
  unsigned int wait_cycles; // number of cycles process has been waiting in state
  unsigned int PC;
} ProcessControlBlockT;

// simulator global structure
static struct {
  pthread_t* cpu_threads;
  int num_threads;
  int max_processes;
  ProcessControlBlockT* process_table;
  pthread_mutex_t process_table_mutex;
  PriorityQueueT ready_queue;
  BlockingQueueT id_queue;
  BlockingQueueT event_queue;
  BlockingQueueT event_blocking_queue;
} simulator;

static void* simulator_routine(void* arg){
  long long thread_id = (long long) arg;
  char buffer[128];
  sprintf(buffer, "Simulator thread has started. Thread ID: %lld", thread_id);
  logger_write(buffer);

  // main CPU Scheduler loop
  while (1) {
    unsigned int pid;
    int results = priority_queue_pop(&simulator.ready_queue, &pid);
    if (results != 0) break;

    pthread_mutex_lock(&simulator.process_table_mutex);

    // fetch the process control block from process table
    ProcessControlBlockT* pcb = &simulator.process_table[pid];

    if (pcb->state != ready) {
      sprintf(buffer, "Process %u in unexpected state %d, terminating thread %lld", pid, pcb->state, thread_id);
      logger_write(buffer);
      pthread_mutex_unlock(&simulator.process_table_mutex);
      continue;
    }

    // running process
    pcb->state = running;
    pcb->wait_cycles = 0;  
    unsigned int current_PC = pcb->PC;
    pthread_mutex_unlock(&simulator.process_table_mutex);

    int result_value = 0;
    EvaluatorResultT eval_result = evaluator_evaluate(pcb->code, current_PC);

    // handle evaluation result: process finished, time slice ended or process blocked
    pthread_mutex_lock(&simulator.process_table_mutex);
    pcb = &simulator.process_table[pid];
    pcb->PC = eval_result.PC;

    switch (eval_result.reason) {
      case reason_terminated: 
        sprintf(buffer, "Process %u has terminated.", pid);
        logger_write(buffer);

        pcb->state = terminated;
        sem_post(&pcb->sem_terminated);
        pthread_mutex_unlock(&simulator.process_table_mutex);
        blocking_queue_push(&simulator.event_blocking_queue, pid);
        break;
      case reason_timeslice_ended: 
        pcb->state = ready;

        sprintf(buffer, "Process %u time slice ended. Queueing again", pid);
        logger_write(buffer);
        
        pthread_mutex_unlock(&simulator.process_table_mutex);
        priority_queue_push(&simulator.ready_queue, pcb->priority, pid);
        break;
      case reason_blocked:
        pcb->state = blocked;
        
        sprintf(buffer, "Process %u is blocked", pid);
        logger_write(buffer);
        pthread_mutex_unlock(&simulator.process_table_mutex);
        blocking_queue_push(&simulator.event_queue, pid);
        break;
    }
  }
}

// request process ID from to ID queue
static ProcessIdT request_pid(){
  unsigned int pid;

  int result = blocking_queue_pop(&simulator.id_queue, &pid);
  if (result != 0){
    return -1;
  }

  return pid;
}

// return process ID back to ID queue
static int return_pid(ProcessIdT pid, int max_processes){
  if (pid >=0 && pid < max_processes){
    blocking_queue_push(&simulator.id_queue, pid);
    return 0;
  }
  return 1;
}

// creates threads for simulator, initializes the process table and creates queues
void simulator_start(int thread_count, int max_processes, PriorityT max_priority_level) {
  char buffer[128];
  int result;
  int threads_created = 0;

  simulator.num_threads = thread_count;
  simulator.max_processes = max_processes;
  simulator.cpu_threads = (pthread_t*)checked_malloc(sizeof(pthread_t)*thread_count);
  simulator.process_table = (ProcessControlBlockT*) checked_malloc(sizeof(ProcessControlBlockT) * max_processes);

  if (!simulator.cpu_threads || !simulator.process_table){
    logger_write("Simulator failed to allocate memory for threads or process table");
    goto cleanup_malloc;
  }

  if (pthread_mutex_init(&simulator.process_table_mutex, NULL) != 0) {
    logger_write("Failed to initialize process table mutex");
    goto cleanup_malloc;
  }

  priority_queue_create(&simulator.ready_queue, max_priority_level);
  blocking_queue_create(&simulator.id_queue);
  blocking_queue_create(&simulator.event_queue);
  blocking_queue_create(&simulator.event_blocking_queue);
  
  for (int i = 0; i < max_processes; i++){
    simulator.process_table[i].id = i;
    simulator.process_table[i].state = unallocated;

    // initializing termination semaphore for each PCB
    if (sem_init(&simulator.process_table[i].sem_terminated, 0, 0) != 0) {
      logger_write("Failed to initialize semaphore");
      goto cleanup_queues;
    }

    blocking_queue_push(&simulator.id_queue, simulator.process_table[i].id);
  }

  for (long long i = 0; i < simulator.num_threads; i++){
    result = pthread_create(&simulator.cpu_threads[i], NULL, simulator_routine, (void*)i);
    if (result != 0) {
      logger_write("Failed to create simulator thread");
      goto cleanup_threads;
    }
    threads_created++;
  }
  
  logger_write("Simulator started successfully");
  return;

cleanup_threads:
  priority_queue_terminate(&simulator.ready_queue);
  blocking_queue_terminate(&simulator.id_queue);
  blocking_queue_terminate(&simulator.event_queue);
  blocking_queue_terminate(&simulator.event_blocking_queue);
  
  for (int i = 0; i < threads_created; i++) {
    pthread_join(simulator.cpu_threads[i], NULL);
  }
  
cleanup_queues:
  for (int i = 0; i < max_processes; i++) {
    sem_destroy(&simulator.process_table[i].sem_terminated);
  }
  blocking_queue_destroy(&simulator.event_blocking_queue);
  blocking_queue_destroy(&simulator.event_queue);
  blocking_queue_destroy(&simulator.id_queue);
  priority_queue_destroy(&simulator.ready_queue);
  pthread_mutex_destroy(&simulator.process_table_mutex);
  
cleanup_malloc:
  if (simulator.process_table) checked_free(simulator.process_table);
  if (simulator.cpu_threads) checked_free(simulator.cpu_threads);
  simulator.process_table = NULL;
  simulator.cpu_threads = NULL;
}

// waits for scheduler threads to finish and cleans up resources
void simulator_stop() {
  char buffer[128];
  sprintf(buffer, "Simulator is stopping %d threads...", simulator.num_threads);
  logger_write(buffer);

  priority_queue_terminate(&simulator.ready_queue);
  blocking_queue_terminate(&simulator.id_queue);
  blocking_queue_terminate(&simulator.event_queue);
  blocking_queue_terminate(&simulator.event_blocking_queue);

  for (int i = 0; i < simulator.num_threads; i++){
    pthread_join(simulator.cpu_threads[i], NULL);
  }

  if (simulator.cpu_threads != NULL){
    checked_free(simulator.cpu_threads);
  }

  if (simulator.process_table != NULL){

    for (int i = 0; i < simulator.max_processes; i++){
      sem_destroy(&simulator.process_table[i].sem_terminated);
    }

    checked_free(simulator.process_table);
  }
  
  blocking_queue_destroy(&simulator.id_queue);
  blocking_queue_destroy(&simulator.event_queue);
  blocking_queue_destroy(&simulator.event_blocking_queue);
  priority_queue_destroy(&simulator.ready_queue);
  pthread_mutex_destroy(&simulator.process_table_mutex);
}

// creates a new process and adds it to ready queue
ProcessIdT simulator_create_process(EvaluatorCodeT const code) {
  ProcessIdT pid = request_pid();
  if (pid == -1){
    return -1;
  }

  pthread_mutex_lock(&simulator.process_table_mutex);

  ProcessControlBlockT* pcb = &simulator.process_table[pid];
  pcb->code = code;
  pcb->state = ready;
  pcb->priority = simulator.ready_queue.num_levels - 1;
  pcb->wait_cycles = 0;
  pcb->PC = 0;

  sem_destroy(&pcb->sem_terminated);
  sem_init(&pcb->sem_terminated, 0, 0);

  pthread_mutex_unlock(&simulator.process_table_mutex);
  priority_queue_push(&simulator.ready_queue, pcb->priority, pid);

  char buffer[128];
  sprintf(buffer, "Process %u created.", pid);
  logger_write(buffer);

  return pid;
} 

// waits for a process to terminate
void simulator_wait(ProcessIdT pid) {
  if (pid >= simulator.max_processes) return;

  char buffer[128];
  sprintf(buffer, "waiting for process %u to terminate.", pid);
  logger_write(buffer);

  sem_wait(&simulator.process_table[pid].sem_terminated);

  pthread_mutex_lock(&simulator.process_table_mutex);
  ProcessControlBlockT* pcb_pointer = &simulator.process_table[pid];
  pcb_pointer->state = unallocated;
  pthread_mutex_unlock(&simulator.process_table_mutex);

  return_pid(pid, simulator.max_processes);
}

// handles event to unblock a blocked process
void simulator_event() {
  if (blocking_queue_empty(&simulator.event_queue)) return;

  unsigned int pid;
  int result = blocking_queue_pop(&simulator.event_queue, &pid);
  if (result != 0) return;

  pthread_mutex_lock(&simulator.process_table_mutex);
  ProcessControlBlockT* pcb = &simulator.process_table[pid];

  if (pcb->state == blocked){
    pcb->state = ready;
    pthread_mutex_unlock(&simulator.process_table_mutex);
    priority_queue_push(&simulator.ready_queue, pcb->priority, pid);

    char buffer[128];
    sprintf(buffer, "Event occurred: Process %u moved from blocked to ready", pid);
    logger_write(buffer);
  }
  else
  {
    pthread_mutex_unlock(&simulator.process_table_mutex);
  }
}

// boosts priority of waiting processes in ready and blocked states
void simulator_boost_priority(){
  int boosted_count = 0;
  int boost_threshold = (int)(simulator.ready_queue.num_levels * 0.7);

  pthread_mutex_lock(&simulator.process_table_mutex);

  for (int i = 0; i < simulator.max_processes; i++){
    ProcessControlBlockT* pcb = &simulator.process_table[i];

    if (pcb->state != ready && pcb->state != blocked) {
      continue;
    }
    pcb->wait_cycles += 1;

    if (pcb->priority >= boost_threshold)
    {
      // boost priority for processes that have waited too long
      int boost_amount = pcb->wait_cycles / 5;
      if (boost_amount < 1) boost_amount = 1;
      if (boost_amount > 3) boost_amount = 3;

      PriorityT old_priority = pcb->priority;
      pcb->priority -= boost_amount;
      if (pcb->priority < 0) pcb->priority = 0;
      pcb->wait_cycles = 0;

      boosted_count++;
    }
  }

  pthread_mutex_unlock(&simulator.process_table_mutex);

  char buffer[128];
  sprintf(buffer, "Booster daemon: Boosted %d processes in total", boosted_count);
  logger_write(buffer);
}
