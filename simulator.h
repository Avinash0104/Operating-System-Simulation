#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_

#include "evaluator.h"
#include "priority_queue.h"
#include "list.h"
#include "non_blocking_queue.h"
#include "blocking_queue.h"
#include "utilities.h"
#include "logger.h"
#include <assert.h>
#include <stdio.h>

typedef unsigned int ProcessIdT;

typedef enum ProcessState {
  unallocated,
  ready,
  running,
  blocked,
  terminated
} ProcessStateT; 

void simulator_start(int threads, int max_processes, PriorityT max_priority_level);
void simulator_stop();

ProcessIdT simulator_create_process(EvaluatorCodeT const code);
void simulator_wait(ProcessIdT pid);
void simulator_event();
void simulator_boost_priority();

#endif
