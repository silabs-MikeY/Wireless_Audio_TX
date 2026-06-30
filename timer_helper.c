#include "timer_helper.h"

#include "print.h"

/**
 * @brief Checks if a clock is enabled by querying its frequency.
 * Returns true if clock frequency is non-zero (enabled).
 * Slower method but works for all clocks; use timers__is_timer_enabled() for timers.
 * 
 * @param clock - CMU clock type to check
 * @return true if clock is enabled, false otherwise
 */
bool timers__is_clock_enabled(CMU_Clock_TypeDef clock)
{
  if (CMU_ClockFreqGet(clock) != 0)
  {
    return true;
  }
  return false;
}

/**
 * @brief Checks if a specific timer is enabled (fast method).
 * Directly reads CLKEN0 register bits for TIMER0-4.
 * Much faster than timers__is_clock_enabled() but limited to CLKEN0 timers.
 * Returns 0xFFFFFFFF for invalid timer input.
 * 
 * @param timer - TIMER peripheral pointer (TIMER0-4)
 * @return true if enabled, false if disabled, 0xFFFFFFFF if invalid
 */
bool timers__is_timer_enabled(TIMER_TypeDef *timer)
{
  if (timer == TIMER0)
  {
    return (CMU->CLKEN0 & CMU_CLKEN0_TIMER0);
  }
  else if (timer == TIMER1)
  {
    return (CMU->CLKEN0 & CMU_CLKEN0_TIMER1);
  }
  else if (timer == TIMER2)
  {
    return (CMU->CLKEN0 & CMU_CLKEN0_TIMER2);
  }
  else if (timer == TIMER3)
  {
    return (CMU->CLKEN0 & CMU_CLKEN0_TIMER3);
  }
  else if (timer == TIMER4)
  {
    return (CMU->CLKEN0 & CMU_CLKEN0_TIMER4);
  }
  else
  {
    //BAD timer value
    return 0xFFFFFFFF;
  }
}

/**
 * @brief Gets the index number of a timer peripheral.
 * Maps TIMER0-4 pointers to indices 0-4.
 * Returns 0xFFFFFFFF for invalid timer input.
 * 
 * @param timer - TIMER peripheral pointer
 * @return Timer index (0-4), or 0xFFFFFFFF if invalid
 */
uint32_t timers__get_timer_index(TIMER_TypeDef *timer)
{
  if (timer == TIMER0)
  {
    return 0;
  }
  else if (timer == TIMER1)
  {
    return 1;
  }
  else if (timer == TIMER2)
  {
    return 2;
  }
  else if (timer == TIMER3)
  {
    return 3;
  }
  else if (timer == TIMER4)
  {
    return 4;
  }
  else
  {
    //BAD timer value
    return 0xFFFFFFFF;
  }
}

/**
 * @brief Gets the CMU clock type for a timer peripheral.
 * Maps TIMER0-4 pointers to cmuClock_TIMER0-4 constants.
 * Validates timer before returning. Asserts on invalid input.
 * 
 * @param timer - TIMER peripheral pointer
 * @return CMU clock type for the timer
 */
CMU_Clock_TypeDef timers__get_cmu_type(TIMER_TypeDef *timer)
{
  if(TIMER_Valid(timer) == false)
  {
    debug__printf_to_buf_append_time(0,"Bad timer choice\n");
    assert(0);
  }

  if (timer == TIMER0)
  {
    return cmuClock_TIMER0;
  }
  else if (timer == TIMER1)
  {
    return cmuClock_TIMER1;
  }
  else if (timer == TIMER2)
  {
    return cmuClock_TIMER2;
  }
  else if (timer == TIMER3)
  {
    return cmuClock_TIMER3;
  }
  else if (timer == TIMER4)
  {
    return cmuClock_TIMER4;
  }
  else
  {
    //BAD timer value
    return 0xFFFFFFFF;
  }
}

/**
 * @brief Disables the clock for a specified timer.
 * Calls CMU_ClockEnable() with false to turn off timer clock.
 * Validates timer before attempting to disable. Asserts on invalid input.
 * 
 * @param timer - TIMER peripheral pointer to disable
 * @return true on success, false otherwise
 */
bool timers__deinit_timer_cmu(TIMER_TypeDef *timer)
{
    if(TIMER_Valid(timer) == false)
    {
        debug__printf_to_buf_append_time(0,"Bad timer choice\n");
        assert(0);
    }

  if (timer == TIMER0)
  {
    CMU_ClockEnable(cmuClock_TIMER0, false);
    return true;
  }
  else if (timer == TIMER1)
  {
    CMU_ClockEnable(cmuClock_TIMER1, false);
    return true;
  }
  else if (timer == TIMER2)
  {
    CMU_ClockEnable(cmuClock_TIMER2, false);
    return true;
  }
  else if (timer == TIMER3)
  {
    CMU_ClockEnable(cmuClock_TIMER3, false);
    return true;
  }
  else if (timer == TIMER4)
  {
    CMU_ClockEnable(cmuClock_TIMER4, false);
    return true;
  }
  else
  {
    //BAD timer value
    //TODO Handle Error
  }
    return false;
}

/**
 * @brief Enables the clock for a specified timer.
 * Calls CMU_ClockEnable() with true to turn on timer clock.
 * Validates timer before attempting to enable. Asserts on invalid input.
 * 
 * @param timer - TIMER peripheral pointer to enable
 * @return true on success, false otherwise
 */
bool timers__init_timer_cmu(TIMER_TypeDef *timer)
{
    if(TIMER_Valid(timer) == false)
    {
        debug__printf_to_buf_append_time(0,"Bad timer choice\n");
        assert(0);
    }

  if (timer == TIMER0)
  {
    CMU_ClockEnable(cmuClock_TIMER0, true);
    return true;
  }
  else if (timer == TIMER1)
  {
    CMU_ClockEnable(cmuClock_TIMER1, true);
    return true;
  }
  else if (timer == TIMER2)
  {
    CMU_ClockEnable(cmuClock_TIMER2, true);
    return true;
  }
  else if (timer == TIMER3)
  {
    CMU_ClockEnable(cmuClock_TIMER3, true);
    return true;
  }
  else if (timer == TIMER4)
  {
    CMU_ClockEnable(cmuClock_TIMER4, true);
    return true;
  }
  else
  {
    //BAD timer value
    //TODO Handle Error
  }
    return false;
}