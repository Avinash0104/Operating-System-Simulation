#ifndef _EVENT_SOURCE_H_
#define _EVENT_SOURCE_H_

#include <unistd.h>
#include "utilities.h"
#include "simulator.h"
#include "logger.h"
#include <stdio.h>

void event_source_start(useconds_t interval);
void event_source_stop();

#endif
