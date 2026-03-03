#include "simulator.h"
#include "environment.h"
#include "event_source.h"
#include "booster_daemon.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SIMULATOR_THREADS
#define SIMULATOR_THREADS 2
#endif

#ifndef SIMULATOR_MAX_PROCESSES
#define SIMULATOR_MAX_PROCESSES 512
#endif

#ifndef SIMULATOR_MAX_PRIORITY_LEVEL
#define SIMULATOR_MAX_PRIORITY_LEVEL 8
#endif

#ifndef ENVIRONMENT_THREADS
#define ENVIRONMENT_THREADS 2
#endif

#ifndef ITERATIONS
#define ITERATIONS 2
#endif

#ifndef BATCH_SIZE
#define BATCH_SIZE 5
#endif

#ifndef EVENT_SOURCE_INTERVAL
#define EVENT_SOURCE_INTERVAL 1000
#endif

#ifndef BOOSTER_DAEMON_INTERVAL
#define BOOSTER_DAEMON_INTERVAL 1000
#endif

int main(int argc, char* argv[]) {
  if(argc < 4) {
    printf("Usage: %s <logger_stream> <debug_stream> <priority_stream>\n", argv[0]);
    exit(-1);
  }
  int const logger_stream = strcmp(argv[1], "false");
  int const debug_stream = strcmp(argv[2], "false");
  int const priority_stream = strcmp(argv[3], "false");
  logger_start(logger_stream, debug_stream, priority_stream);
  logger_write("Starting simulator");
  simulator_start(SIMULATOR_THREADS, SIMULATOR_MAX_PROCESSES, SIMULATOR_MAX_PRIORITY_LEVEL);
  event_source_start(EVENT_SOURCE_INTERVAL);
  booster_daemon_start(BOOSTER_DAEMON_INTERVAL);
  environment_start(ENVIRONMENT_THREADS, ITERATIONS, BATCH_SIZE);
  environment_stop();
  booster_daemon_stop();
  simulator_stop();
  event_source_stop();
  logger_write("Stopping simulator");
  logger_stop();
  return 0;
}
