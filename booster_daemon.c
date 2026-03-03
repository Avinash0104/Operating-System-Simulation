#include "booster_daemon.h"

static struct {
    pthread_t thread;
    useconds_t interval;
    volatile int running; // flag to control the running state
} booster;

// booster routine: sleeps then boosts priority until running flag is cleared
static void* booster_routine(void* arg)
{
    logger_write("Booster daemon started");
    while (booster.running) {
        usleep(booster.interval);
        if (!booster.running) break;
        simulator_boost_priority();
    }
    return NULL;
}

// creates a new thread for the booster routine
void booster_daemon_start(useconds_t interval) {
    booster.interval = interval;
    booster.running = 1;

    char buffer[128];
    sprintf(buffer, "Booster starting with interval %u us...", (unsigned int)interval);
    logger_write(buffer);
    
    int result = pthread_create(&booster.thread, NULL, booster_routine, NULL);

    if (result != 0) {
        logger_write("ERROR: Failed to create booster thread");
        booster.running = 0;
        return;
    }
}

// stops booster routine and waits for thread to finish
void booster_daemon_stop() {
    logger_write("Booster daemon stopping...");
    booster.running = 0;
    pthread_join(booster.thread, NULL);
}
