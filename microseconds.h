#ifndef MICROSECONDS_H_
#define MICROSECONDS_H_

#include "em_cmu.h"
#include "hardware_config.h"
#include "em_timer.h"
#include "timer_helper.h"
#include "stddef.h"
#include "stdbool.h"
#include "assert.h"
#include "print.h"
#include "counters.h"

uint32_t microseconds__get_micros_count(void);
void microseconds__deinit_microsecond(void);
void microseconds__init_microsecond(void);
void microseconds__init_microsecond_count(void);
void microseconds__reset_micros_count(void);

#endif /* RADIO_H_ */