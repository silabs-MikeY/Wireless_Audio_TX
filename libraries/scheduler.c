#include "scheduler.h"

#include "microseconds.h"
#include "RGB.h"

#include <assert.h>
#include <stdbool.h>
#include "em_wdog.h"
#include "counters_new.h"

#define DEBUG 3

typedef enum scheduler_counter_index_s {
  MS_SINCE_LAST_RESET = 0,
  SCHEDULER_NUMBER_OF_COUNTERS
} scheduler_counter_index_t;

static volatile uint32_t scheduler_counter_values[SCHEDULER_NUMBER_OF_COUNTERS] = {0};
static const char *scheduler_counter_names[SCHEDULER_NUMBER_OF_COUNTERS] = {
  "ms_since_last_reset",
};



//volatile uint32_t millisecond_ticks = 0;

bool tick_flag = false;

static uint32_t countdown_5s = 5000;

static void run_scheduler_1_ms(void);
static void run_scheduler_10_ms(void);
static void run_scheduler_100_ms(void);
static void run_scheduler_1s(void);
static void scheduler__reset_milliseconds(void);
//bool tick_flag_10ms = false;
//bool tick_flag_100ms = false
//bool tick_flag_1s = false;

typedef struct millisseconds_s
{
    uint32_t current_millisecond_ticks;
    uint32_t next_timestamp_increment;
} millisseconds_t;
millisseconds_t milliseconds;

void scheduler__reset_milliseconds(void)
{
  milliseconds.current_millisecond_ticks = 0;
  milliseconds.next_timestamp_increment = 1000;
}

void scheduler__reset_5s_countdown(void)
{
  countdown_5s = 5000;
}

uint32_t scheduler__get_millisecond_ticks(void)
{
  return milliseconds.current_millisecond_ticks;
}

uint32_t scheduler__get_microsecond_ticks(void)
{
  return microseconds__get_micros_count();
  //return ((non_volatile_millisecond_ticks * 1000) + microseconds__get_micros_count());
}

uint32_t scheduler__get_number_of_counters(void)
{
  return SCHEDULER_NUMBER_OF_COUNTERS;
}

const char *scheduler__get_counter_name(uint32_t counter_index)
{
  if (counter_index >= SCHEDULER_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return scheduler_counter_names[counter_index];
}

volatile uint32_t *scheduler__get_counter_address(uint32_t counter_index)
{
  if (counter_index >= SCHEDULER_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return &scheduler_counter_values[counter_index];
}

void scheduler__reset_counters(void)
{
  scheduler_counter_values[MS_SINCE_LAST_RESET] = 0;
}

void SysTick_Handler(void)
{
  // non_volatile_millisecond_ticks++;
  //printf_to_buf_append_time(0,"tick : %u\n",microseconds__get_micros_count());
  //microseconds__reset_micros_count();
  tick_flag = true;
}

void scheduler__deinit_SysTick(void)
{
  debug__printf_to_buf_append_time(0,"Starting SysTick DeInit\n");
  SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk;

  debug__printf_to_buf_append_time(0,"Finish SysTick DeInit\n");
}

void scheduler__init_SysTick(void)
{
  debug__printf_to_buf_append_time(0,"Starting SysTick\n");
  uint32_t core_speed = CMU_ClockFreqGet(cmuClock_CORE);
  debug__printf_to_buf_append_time(0,"Core Speed : %u\n", (unsigned int)core_speed);
  if (SysTick_Config(core_speed/1000) != 0)
    {
      debug__printf_to_buf_append_time(0,"Systick Failed to start\n");
      assert(0);
    }
  microseconds__reset_micros_count();
  scheduler__reset_milliseconds();
  debug__printf_to_buf_append_time(0,"Systick Started\n");
}

static void run_scheduler_1_ms(void)
{
  rgb__run_signal_intensity_state_machine(milliseconds.current_millisecond_ticks);
  rgb__run_radio_status_blink(milliseconds.current_millisecond_ticks);
}

static void run_scheduler_10_ms(void)
{

}

static void run_scheduler_100_ms(void)
{
   WDOGn_Feed(WDOG0);
   //printf_to_buf_append_time(0,"Feed WDOG\n");
}


static void run_scheduler_1s(void)
{
  //radio__validate_radio_statistics();
  static uint32_t delay_start = 3;
  if (delay_start == 0)
  {
    //counters__one_second_print();
    counters_new__save_and_print_counters(scheduler__get_microsecond_ticks());
  }
  else
  {
    delay_start--;
  }
}

static void run_scheduler_5s(void)
{
  rgb__start_radio_status_blink(milliseconds.current_millisecond_ticks);
}

void scheduler__update_millis(void)
{
  uint32_t time_now = microseconds__get_micros_count();
  while (time_now > milliseconds.next_timestamp_increment)
  {
    milliseconds.current_millisecond_ticks++;
    milliseconds.next_timestamp_increment += 1000;
  }
}

void scheduler__run_scheduler(void)
{
  static uint32_t tick = 0;
  //printf_to_buf_append_time(0,"tick");
  if (tick < milliseconds.current_millisecond_ticks)
    {
      tick++;
      //printf_to_buf_append_time(0,"**tock**");
      // tick_flag = false;
      // non_volatile_millisecond_ticks = millisecond_ticks;
      scheduler_counter_values[MS_SINCE_LAST_RESET]++;
      run_scheduler_1_ms();

      static uint32_t countdown_10ms = 0;
      countdown_10ms++;
      if (countdown_10ms >= 10)
        {
          countdown_10ms = 0;
          run_scheduler_10_ms();

          //          static uint32_t countdown_100ms = 10;
          //          countdown_100ms--;
          //          if (countdown_100ms == 0)
          //            {
          //              countdown_100ms = 10;
          //              run_scheduler_100_ms();
          //
          //              static uint32_t countdown_1s = 10;
          //              countdown_1s--;
          //              if (countdown_1s == 0)
          //                {
          //                  countdown_1s = 10;
          //                  run_scheduler_1s();
          //                }
          //            }
        }

      static uint32_t countdown_100ms = 0;
      countdown_100ms++;
      if (countdown_100ms >= 100)
        {
          countdown_100ms = 0;
          run_scheduler_100_ms();
        }

      static uint32_t countdown_1s = 0;
      countdown_1s++;
      if (countdown_1s >= 1000)
        {
          countdown_1s = 0;
          run_scheduler_1s();
        }

      static uint32_t countdown_5s = 0;
      countdown_5s++;
      if (countdown_5s >= 5000)
        {
          countdown_5s = 0;
          run_scheduler_5s();
        }
    }
}