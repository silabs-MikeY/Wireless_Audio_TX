#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "hardware_config.h"
#include "em_usart.h"
#include "em_ldma.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "stddef.h"
#include "stdbool.h"
#include "RGB.h"
#include "debug.h"
#include "em_wdog.h"
#include "microseconds.h"

uint32_t scheduler__get_millisecond_ticks(void);
uint32_t scheduler__get_microsecond_ticks(void);
void scheduler__run_scheduler(void);
void scheduler__init_SysTick(void);
void scheduler__deinit_SysTick(void);
void scheduler__reset_5s_countdown(void);

#endif /* SCHEDULER_H_ */
