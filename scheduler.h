#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "stddef.h"
#include <stdint.h>

uint32_t scheduler__get_millisecond_ticks(void);
uint32_t scheduler__get_microsecond_ticks(void);
void scheduler__run_scheduler(void);
void scheduler__init_SysTick(void);
void scheduler__deinit_SysTick(void);
void scheduler__reset_5s_countdown(void);
void scheduler__update_millis(void);

#endif /* SCHEDULER_H_ */
