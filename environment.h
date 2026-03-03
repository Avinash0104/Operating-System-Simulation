#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

#include "simulator.h"
#include "utilities.h"
#include "evaluator.h"
#include "list.h"
#include <stdio.h>
#include "logger.h"

void environment_start(unsigned int thread_count,
		       unsigned int iterations,
		       unsigned int batch_size);
void environment_stop();

#endif

