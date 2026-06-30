#include "counters.h"
#include "RGB.h"
#include "print.h"
#include "scheduler.h"
#include "microseconds.h"
#include "state_machine.h"
#include "radio_statistics.h"
#include "audio_buffers.h"
#include "em_usart.h"
#include "ADC.h"
#include "wdog.h"
#include "app_process.h"

static void counters__save_and_reset_counters(void);
static void counters__debug_print_state_machine_printouts(uint32_t printing_counters_index);
static void counters__fill_missing_counters(void);

bool printing_counters_flag = false;
uint32_t printing_counters_index = 0;

#if (DEBUG_ENABLE == 1)
volatile uint32_t volatile_counters[NUMBER_OF_DEBUG_COUNTER_NAMES];
uint32_t saved_counters[NUMBER_OF_DEBUG_COUNTER_NAMES];
uint32_t saved_counters_timestamp = 0;
#endif

/**
 * @brief Increments a debug counter at the specified index.
 * Only increments if DEBUG_ENABLE is 1 at compile time.
 * Used to track system events (TX attempts, RX packets, errors, etc.).
 * 
 * @param counter_index_to_increment - Index into volatile_counters[] array
 * @return void
 */
void counters__increment_counter(uint32_t counter_index_to_increment)
{
#if (DEBUG_ENABLE == 1)
  volatile_counters[counter_index_to_increment]++;
#endif
}

/**
 * @brief Adds a quantity to a debug counter at the specified index.
 * Increments counter by specified amount (not just by 1).
 * Only updates if DEBUG_ENABLE is 1 at compile time.
 * Used for bulk updates (e.g., adding sample counts).
 * 
 * @param counter_index_to_increment - Index into volatile_counters[] array
 * @param quantity_to_add - Amount to add to the counter
 * @return void
 */
void counters__add_to_counter(uint32_t counter_index_to_increment, uint32_t quantity_to_add)
{
#if (DEBUG_ENABLE == 1)
  volatile_counters[counter_index_to_increment] += quantity_to_add;
#endif
}

/**
 * @brief Sets a debug counter to a specific value.
 * Replaces counter value instead of incrementing.
 * Only updates if DEBUG_ENABLE is 1 at compile time.
 * Used for state/sequence tracking (e.g., sequence numbers).
 * 
 * @param counter_index_to_increment - Index into volatile_counters[] array
 * @param value_to_set_to - New value for the counter
 * @return void
 */
void counters__set_counter(uint32_t counter_index_to_increment, uint32_t value_to_set_to)
{
#if (DEBUG_ENABLE == 1)
  volatile_counters[counter_index_to_increment] = value_to_set_to;
#endif
}

static void counters__fill_missing_counters(void)
{
  uint32_t tx_delta_min_value = 0;
  uint32_t tx_delta_max_value = 0;
  uint32_t tx_packets_processed_value = 0;
  uint32_t audio_level_min_mv_value = 0;
  uint32_t audio_level_max_mv_value = 0;

  radio_statistics__get_min_and_max_transmit_deltas(&tx_delta_min_value, &tx_delta_max_value);
  radio_statistics__get_number_of_transmits_processed(&tx_packets_processed_value);
  rgb__get_audio_level_stats(&audio_level_min_mv_value, &audio_level_max_mv_value);

  volatile_counters[TX_micros_delta_min] = tx_delta_min_value;
  volatile_counters[TX_micros_delta_max] = tx_delta_max_value;
  volatile_counters[number_of_TX_packets_processed_after_sending] = tx_packets_processed_value;
  volatile_counters[audio_level_min_mv] = audio_level_min_mv_value;
  volatile_counters[audio_level_max_mv] = audio_level_max_mv_value;
  volatile_counters[sequence_number_at_end] = radio__get_sequence_number();
  volatile_counters[number_of_micro_ticks] = microseconds__get_micros_count();
}

static void counters__save_and_reset_counters(void)
{
#if (DEBUG_ENABLE == 1)
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  counters__fill_missing_counters();
  memcpy_from_volatile((uint8_t *)&saved_counters, (volatile uint8_t *)&volatile_counters, sizeof(volatile_counters));
  memset_volatile((volatile uint8_t *)&volatile_counters, 0, sizeof(volatile_counters));
  volatile_counters[timestamp_of_last_counter_save] = scheduler__get_millisecond_ticks();
  volatile_counters[sequence_number_at_start] = radio__get_sequence_number();
  // counters.timestamp_last_reset = DWT->CYCCNT;
  //  counters.sequence_number_at_start = radio__get_sequence_number();
  CORE_EXIT_CRITICAL();
#endif
}

void counters__init_counters(void)
{
#if (DEBUG_ENABLE == 1)
  counters__save_and_reset_counters();
#endif
}

// void debug__set_start_sequence_number(uint16_t sequence_number)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.sequence_number_at_start = sequence_number;
// #endif
// }

// void debug__set_end_sequence_number(uint16_t sequence_number)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.sequence_number_at_end = sequence_number;
// #endif
// }

// void debug__increment_number_of_micro_ticks(uint32_t size)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_micro_ticks += size;
// #endif
// }

// void debug__increment_number_of_left_samples_receiver(uint32_t number_of_new_samples)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.samples_received_left += number_of_new_samples;
// #endif
// }

// void debug__increment_number_of_right_samples_receiver(uint32_t number_of_new_samples)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.samples_received_right += number_of_new_samples;
// #endif
// }

// void debug__increment_left_buffers_written(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.left_buffers_written ++;
// #endif
// }

// void debug__increment_right_buffers_written(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.right_buffers_written++;
// #endif
// }

// void debug__increment_left_buffers_consumed(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.left_buffers_consumed;
// #endif
// }

// void debug__increment_right_buffers_consumed(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.right_buffers_consumed;
// #endif
// }

// void debug__increment_number_of_TX_attempts(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_attempts++;
//   }
//   else
//   {
//     counters.number_of_TX_attempts++;
//   }
// #endif
// }

// void debug__log_TX_success(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_success++;
//   }
//   else
//   {
//     counters.number_of_TX_success++;
//   }
// #endif
// }

// void debug__log_TX_fail(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_failed_to_send++;
//   }
//   else
//   {
//     counters.number_of_TX_failed_to_send++;
//   }
// #endif
// }
// void debug__log_TX_abort(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_abort++;
//   }
//   else
//   {
//     counters.number_of_TX_abort++;
//   }
// #endif
// }
// void debug__log_TX_block(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_block++;
//   }
//   else
//   {
//     counters.number_of_TX_block++;
//   }
// #endif
// }
// void debug__log_TX_underflow(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_underflow++;
//   }
//   else
//   {
//     counters.number_of_TX_underflow++;
//   }
// #endif
// }
// void debug__log_TX_busy(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_busy++;
//   }
//   else
//   {
//     counters.number_of_TX_busy++;
//   }
// #endif
// }
// void debug__log_TX_missed(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_missed++;
//   }
//   else
//   {
//     counters.number_of_TX_missed++;
//   }
// #endif
// }

// void debug__increment_number_of_TX_failed(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   if (retry)
//   {
//     counters.number_of_TX_retry_failed++;
//   }
//   else
//   {
//     counters.number_of_TX_failed++;
//   }
// #endif
// }

// void debug__increment_number_of_TX_retry_attempts(bool retry)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_attempts++;
// #endif
// }

// void debug__log_TX_retry_success(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_success++;
// #endif
// }

// void debug__log_TX_retry_fail(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_failed_to_send++;
// #endif
// }
// void debug__log_TX_retry_abort(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_abort++;
// #endif
// }
// void debug__log_TX_retry_block(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_block++;
// #endif
// }
// void debug__log_TX_retry_underflow(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_underflow++;
// #endif
// }
// void debug__log_TX_retry_busy(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_busy++;
// #endif
// }
// void debug__log_TX_retry_missed(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_missed++;
// #endif
// }

// void debug__increment_number_of_TX_retry_failed(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.number_of_TX_retry_failed++;
// #endif
// }

// void debug__increment_app_process_action_runs(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.app_process_action_runs++;
// #endif
// }

// void debug__increment_packet_buffer_overflows(void)
// {
// #if (DEBUG_ENABLE == 1)
//   counters.packet_buffer_overflows++;
// #endif
// }

void counters__reset_external_counters(void)
{
  radio_statistics__reset_radio_statistics_for_new_measurement();
  reset_app_process_action_run_delta_micros();
}

void counters__one_second_print(void)
{
#if (DEBUG_ENABLE == 1)
  uint32_t time = scheduler__get_millisecond_ticks();
  volatile_counters[timestamp_of_this_counter_save] = time;
  saved_counters_timestamp = microseconds__get_micros_count();
  counters__save_and_reset_counters();
  debug__printf_to_buf_append_time(saved_counters_timestamp,"Print Counters - Timestamp : %u\n", (unsigned int)time);
  printing_counters_flag = true;
  printing_counters_index = 0;
  counters__reset_external_counters();
#endif
}

void counters__run_debug_print_state_machine(void)
{
  if (printing_counters_flag == true)
  {
    // #define NUMBER_OF_PRINTS_BEFORE_COUNTERS 2
    //  #define NUMBER_OF_PRINTS_AFTER_COUNTERS 30

    counters__debug_print_state_machine_printouts(printing_counters_index);
    printing_counters_index++;
    if (printing_counters_index >= NUMBER_OF_DEBUG_COUNTER_NAMES)
    {
      debug__printf_to_buf_append_time(saved_counters_timestamp,"\n");
      printing_counters_flag = false;
    }
  }
}

static void counters__debug_print_state_machine_printouts(uint32_t printing_counters_index)
{
  switch (printing_counters_index)
  {
  // Counters to modify the printout
  case timestamp_of_this_counter_save:
  {
    static uint32_t core_ticks_last_counter_save = 0;
    static uint32_t micro_ticks_last_counter_save = 0;
    uint32_t core_ticks_now = DWT->CYCCNT;
    uint32_t micro_ticks_now = saved_counters[number_of_micro_ticks];
    debug__printf_to_buf_append_time(saved_counters_timestamp,"-  Core Speed : %u , Ticks Since Last Save : %u\n", (unsigned int)CMU_ClockFreqGet(cmuClock_CORE), (unsigned int)(core_ticks_now - core_ticks_last_counter_save));
    core_ticks_last_counter_save = core_ticks_now;

    uint32_t difference = saved_counters[timestamp_of_this_counter_save] - saved_counters[timestamp_of_last_counter_save];
    debug__printf_to_buf_append_time(saved_counters_timestamp,"-  Milliseconds Since Last Save : %u\n", (unsigned int)difference);

    // static uint32_t microseconds_since_last_counter_save = 0;
    // microseconds_since_last_counter_save = 
    debug__printf_to_buf_append_time(saved_counters_timestamp,"-  Microseconds Since Last Save : %u\n", (unsigned int)(micro_ticks_now - micro_ticks_last_counter_save));
    micro_ticks_last_counter_save = micro_ticks_now;
    break;
  }

    // TX error counters, only print if non-zero
  case number_of_TX_fail_failed_to_send:
  case number_of_TX_fail_abort:
  case number_of_TX_fail_block:
  case number_of_TX_fail_underflow:
  case number_of_TX_fail_busy:
  case number_of_TX_fail_missed:
  case number_of_TX_retry_fail_failed_to_send:
  case number_of_TX_retry_fail_abort:
  case number_of_TX_retry_fail_block:
  case number_of_TX_retry_fail_underflow:
  case number_of_TX_retry_fail_busy:
  case number_of_TX_retry_fail_missed:
    if (saved_counters[printing_counters_index] != 0)
    {
      debug__printf_to_buf_append_time(saved_counters_timestamp,"-  %s : %u\n", DEBUG_COUNTERS_STRINGS[printing_counters_index], (unsigned int)saved_counters[printing_counters_index]);
    }
    break;

    // Counters to skip
  case timestamp_of_last_counter_save:
  case number_of_micro_ticks:
    break;
    // Print normally
  default:
    debug__printf_to_buf_append_time(saved_counters_timestamp,"-  %s : %u\n", DEBUG_COUNTERS_STRINGS[printing_counters_index], (unsigned int)saved_counters[printing_counters_index]);
    break;
  }
}