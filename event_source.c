#include "event_source.h"

static struct{
    pthread_t thread;
    int interval;
    volatile int running; // flag to control running state - takes value from memory every time accessed
} event_source;

// event source routine: sleeps then triggers events until running flag is cleared
static void* event_source_routine(void* arg){
    while (event_source.running){
        usleep(event_source.interval);
        if (!event_source.running) break;
        simulator_event();
    }
    return NULL;
}

// creates a new thread for the event source routine
void event_source_start(useconds_t interval) {
    event_source.interval = interval;
    event_source.running = 1;

    char buffer[128];
    sprintf(buffer, "Event source starting with interval %d us...", interval);
    logger_write(buffer);

    int result = pthread_create(&event_source.thread, NULL, event_source_routine, NULL); 

    if (result != 0) {
        logger_write("ERROR: Failed to create event source thread");
        event_source.running = 0;
        return;
    }
}

// stops event source routine and waits for thread to finish
void event_source_stop() {
    logger_write("Event source stopping...");
    event_source.running = 0;
    pthread_join(event_source.thread, NULL);
}

