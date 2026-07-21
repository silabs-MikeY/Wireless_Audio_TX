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

uint32_t microseconds__get_number_of_counters(void);
const char *microseconds__get_counter_name(uint32_t counter_index);
volatile uint32_t *microseconds__get_counter_address(uint32_t counter_index);
void microseconds__trigger_counter_update(void);
void microseconds__printf(bool add_timestamp, const char *format, ...);

uint32_t microseconds__get_micros_count(void);
void microseconds__reset_counters(void);
void microseconds__deinit_microsecond(void);
bool microseconds__init_microsecond(void);
bool microseconds__init_microsecond_count(void);

#endif /* RADIO_H_ */
