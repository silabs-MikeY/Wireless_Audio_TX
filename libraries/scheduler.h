#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "stddef.h"
#include <stdbool.h>
#include <stdint.h>

uint32_t scheduler__get_millisecond_ticks(void);
uint32_t scheduler__get_microsecond_ticks(void);
bool scheduler__run_scheduler(void);
bool scheduler__init_SysTick(void);
void scheduler__deinit_SysTick(void);
void scheduler__reset_5s_countdown(void);
void scheduler__update_millis(void);

 void run_scheduler_1_ms(void);
 void run_scheduler_10_ms(void);
 void run_scheduler_100_ms(void);
 void run_scheduler_1s(void);

// Optional debug data functions for counters and logging.
uint32_t scheduler__get_number_of_counters(void);
const char* scheduler__get_counter_name(uint32_t counter_index);
volatile uint32_t* scheduler__get_counter_address(uint32_t counter_index);
void scheduler__reset_counters(void);

void scheduler__printf(bool add_timestamp, const char *format, ...);

#endif /* SCHEDULER_H_ */
