#ifndef _BOOSTER_DAEMON_H_
#define _BOOSTER_DAEMON_H_

#include "simulator.h"
#include "logger.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void booster_daemon_start(useconds_t interval);
void booster_daemon_stop();

#endif
