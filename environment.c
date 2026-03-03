#include "environment.h"

typedef struct {
    int iterations;
    int batch_size;
} EnvArgT;

static pthread_t* env_threads = NULL;
static pthread_t* env_blocking_threads = NULL;
static unsigned int env_thread_count = 0;
static EnvArgT env_args;

// creates blocking processes that run on CPU for a set number of steps
static void* blocking_routine(void *arg){
	int iterations = env_args.iterations;
	int batch_size = env_args.batch_size;

	for (int i = 0; i < iterations; i++){
		ProcessIdT* pids = (ProcessIdT*)checked_malloc(batch_size * sizeof(ProcessIdT));

		for (int j = 0; j < batch_size; j++){
			ProcessIdT pid = simulator_create_process(evaluator_blocking_terminates_after(5));
			
			if (pid == -1){
				checked_free(pids);
				return NULL;
			}
			pids[j] = pid;
		}

		for (int j = 0; j < batch_size; j++){
			simulator_wait(pids[j]);
		}
		checked_free(pids);
	}
	return NULL;
}

// creates terminating processes that run on CPU for a set number of steps
static void* terminating_routine(void *arg){

	int iterations = env_args.iterations;
	int batch_size = env_args.batch_size;

	char buffer[128];

	for (int i = 0; i < iterations; i++){
		ProcessIdT* pids = (ProcessIdT*)checked_malloc(batch_size * sizeof(ProcessIdT));

		for (int j = 0; j < batch_size; j++){
			unsigned int steps = (rand() % 20) + 5;
			ProcessIdT pid = simulator_create_process(evaluator_terminates_after(steps));

			if (pid == -1){
				checked_free(pids);
				return NULL;
			}

			pids[j] = pid;
		}

		for (int j = 0; j < batch_size; j++){
			simulator_wait(pids[j]);
		}

		checked_free(pids);
	}
	return NULL;

}


// creates environment threads that generate processes using the routines
void environment_start(unsigned int thread_count,
		       unsigned int iterations,
		       unsigned int batch_size) {

	char buffer[128];
	int term_thread_created = 0;
	int block_thread_created = 0;

	env_args.iterations = iterations;
	env_args.batch_size = batch_size;

	env_thread_count = thread_count;
	env_threads = (pthread_t*)checked_malloc(sizeof(pthread_t) * thread_count);
	env_blocking_threads = (pthread_t*)checked_malloc(sizeof(pthread_t) * thread_count);
	
	if (!env_threads || !env_blocking_threads){
		logger_write("Environment failed to allocate memory for threads");
		goto cleanup;
	}

    sprintf(buffer, "Environment starting with %d threads (Iteration: %d, Batch Size: %d)...", 
			thread_count, iterations, batch_size);
    logger_write(buffer);

	for (long i = 0; i < thread_count; i++)
	{
		int result = pthread_create(&env_threads[i], NULL, terminating_routine, NULL);
		if (result != 0) {
			sprintf(buffer, "Failed to create terminating thread %ld", i);
			logger_write(buffer);
			goto cleanup_threads;
		}
		term_thread_created++;
	}

	for (long i = 0; i < thread_count; i++)
	{
		int result = pthread_create(&env_blocking_threads[i], NULL, blocking_routine, NULL);
		if (result != 0) {
			sprintf(buffer, "Failed to create blocking thread %ld", i);
			logger_write(buffer);
			goto cleanup_threads;
		}
		block_thread_created++;
	}
	
	return;

cleanup_threads:
	for (int i = 0; i < term_thread_created; i++) {
		pthread_join(env_threads[i], NULL);
	}
	for (int i = 0; i < block_thread_created; i++) {
		pthread_join(env_blocking_threads[i], NULL);
	}
	
cleanup:
	if (env_threads) checked_free(env_threads);
	if (env_blocking_threads) checked_free(env_blocking_threads);
	env_threads = NULL;
	env_blocking_threads = NULL;
}

// wait for all threads to finish and clean up
void environment_stop() {
	char buffer[128];
	sprintf(buffer, "Environment stopping...");
	logger_write(buffer);

	for (int i = 0; i < env_thread_count; i++){
		pthread_join(env_threads[i], NULL);
	}

	for (int i = 0; i < env_thread_count; i++){
		pthread_join(env_blocking_threads[i], NULL);
	}

	sprintf(buffer, "All environment threads have joined");
	logger_write(buffer);

	if (env_threads != NULL){
		checked_free(env_threads);
		env_threads = NULL;
	}

	if (env_blocking_threads != NULL){
		checked_free(env_blocking_threads);
		env_blocking_threads = NULL;
	}
}

