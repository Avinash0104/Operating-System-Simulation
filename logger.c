#include "logger.h"

pthread_mutex_t logger_mutex;
unsigned long long message_counter = 0;

static int log = 0;
static int debug = 0;
static int priority = 0;

// gets timestamp and writes message with label
static void write(const char* setting, const char* message){
    pthread_mutex_lock(&logger_mutex);
    message_counter++;

    char buffer[32];
    time_t global_time = time(NULL);
    struct tm *local_time = localtime(&global_time);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", local_time);
    printf("[%s][%llu]: %s\n", setting, message_counter, message);
    
    pthread_mutex_unlock(&logger_mutex);
}

// initializes logger with specified streams
void logger_start(int logger_stream, int debug_stream, int priority_stream) {
    log = logger_stream;
    debug = debug_stream;
    priority = priority_stream;

    pthread_mutex_init(&logger_mutex, NULL);
    message_counter = 0;
}

// cleans up logger mutex
void logger_stop() {
    pthread_mutex_destroy(&logger_mutex);
}

// writes message to log stream if enabled
void logger_write(char const* message) {
    if (!log){
        return;
    }
    write("Log", message);
}

// writes message to debug stream if enabled
void debug_write(char const* message) {
    if (!debug){
        return;
    }
    write("Debug", message);
}

// writes message to priority stream if enabled
void priority_write(char const* message) {
    if (!priority){
        return;
    }
    write("Priority", message);
}