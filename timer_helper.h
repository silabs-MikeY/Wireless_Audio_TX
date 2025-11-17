#ifndef TIMER_HELPERS_H_
#define TIMER_HELPERS_H_
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_timer.h"
#include "stddef.h"
#include "stdbool.h"
#include "assert.h"

bool timers__init_timer_cmu(TIMER_TypeDef *timer);
bool timers__deinit_timer_cmu(TIMER_TypeDef *timer);
uint32_t timers__get_timer_index(TIMER_TypeDef *timer);
CMU_Clock_TypeDef timers__get_cmu_type(TIMER_TypeDef *timer);
bool timers__is_clock_enabled(CMU_Clock_TypeDef clock);
bool timers__is_timer_enabled(TIMER_TypeDef *timer);
#endif