#include "debug.h"

static void save_and_reset_counters(void);
static void debug__debug_print_state_machine_printouts(printing_counters_index);

bool printing_counters_flag = false;
uint32_t printing_counters_index = 0;

#if (DEBUG_ENABLE == 1)
volatile uint32_t debug_counters[NUMBER_OF_DEBUG_COUNTER_NAMES];
uint32_t debug_counters_saved[NUMBER_OF_DEBUG_COUNTER_NAMES];
#endif

/**
 * @brief Increments a debug counter at the specified index.
 * Only increments if DEBUG_ENABLE is 1 at compile time.
 * Used to track system events (TX attempts, RX packets, errors, etc.).
 * 
 * @param counter_index_to_increment - Index into debug_counters[] array
 * @return void
 */
void debug__increment_counter(uint32_t counter_index_to_increment)
{
#if (DEBUG_ENABLE == 1)
  debug_counters[counter_index_to_increment]++;
#endif
}

/**
 * @brief Adds a quantity to a debug counter at the specified index.
 * Increments counter by specified amount (not just by 1).
 * Only updates if DEBUG_ENABLE is 1 at compile time.
 * Used for bulk updates (e.g., adding sample counts).
 * 
 * @param counter_index_to_increment - Index into debug_counters[] array
 * @param quantity_to_add - Amount to add to the counter
 * @return void
 */
void debug__add_to_counter(uint32_t counter_index_to_increment, uint32_t quantity_to_add)
{
#if (DEBUG_ENABLE == 1)
  debug_counters[counter_index_to_increment] += quantity_to_add;
#endif
}

/**
 * @brief Sets a debug counter to a specific value.
 * Replaces counter value instead of incrementing.
 * Only updates if DEBUG_ENABLE is 1 at compile time.
 * Used for state/sequence tracking (e.g., sequence numbers).
 * 
 * @param counter_index_to_increment - Index into debug_counters[] array
 * @param value_to_set_to - New value for the counter
 * @return void
 */
void debug__set_counter(uint32_t counter_index_to_increment, uint32_t value_to_set_to)
{
#if (DEBUG_ENABLE == 1)
  debug_counters[counter_index_to_increment] = value_to_set_to;
#endif
}

// typedef struct
// {
//   uint32_t timestamp_last_reset;
//   uint32_t app_process_action_runs;
//   uint32_t number_of_TX_attempts;
//   uint32_t number_of_TX_success;
//   uint32_t number_of_TX_failed;
//   uint32_t number_of_TX_failed_to_send;
//   uint32_t number_of_TX_abort;
//   uint32_t number_of_TX_block;
//   uint32_t number_of_TX_underflow;
//   uint32_t number_of_TX_busy;
//   uint32_t number_of_TX_missed;
//   uint32_t number_of_TX_retry_attempts;
//   uint32_t number_of_TX_retry_success;
//   uint32_t number_of_TX_retry_failed;
//   uint32_t number_of_TX_retry_failed_to_send;
//   uint32_t number_of_TX_retry_abort;
//   uint32_t number_of_TX_retry_block;
//   uint32_t number_of_TX_retry_underflow;
//   uint32_t number_of_TX_retry_busy;
//   uint32_t number_of_TX_retry_missed;
//   uint32_t number_of_micro_ticks;
//   uint32_t samples_received_left;
//   uint32_t samples_received_right;
//   uint32_t left_buffers_written;
//   uint32_t right_buffers_written;
//   uint32_t left_buffers_consumed;
//   uint32_t right_buffers_consumed;
//   uint32_t packet_buffer_overflows;
//   uint32_t sequence_number_at_start;
//   uint32_t sequence_number_at_end;
// } debug_counters_t;

// char* debug_counters_t_strings [(sizeof(debug_counters_t)/4)][100]

// volatile debug_counters_t counters;
// debug_counters_t non_volatile_counters;

// #endif

// static void reset_counters(void)
//{
// #if (DEBUG==1)
//   CORE_DECLARE_IRQ_STATE;
//   CORE_ENTER_CRITICAL();
//   memset(&counters,0,sizeof(debug_counters_t));
//   counters.timestamp_last_reset = DWT->CYCCNT;
//   CORE_EXIT_CRITICAL();
// #endif
// }

static void save_and_reset_counters(void)
{
#if (DEBUG_ENABLE == 1)
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  memcpy_from_volatile(&debug_counters_saved, &debug_counters, sizeof(debug_counters));
  memset_volatile((volatile uint8_t *)&debug_counters, 0, sizeof(debug_counters));
  debug_counters[timestamp_of_last_counter_save] = scheduler__get_millisecond_ticks();
  debug_counters[sequence_number_at_start] = radio__get_sequence_number();
  // counters.timestamp_last_reset = DWT->CYCCNT;
  //  counters.sequence_number_at_start = radio__get_sequence_number();
  CORE_EXIT_CRITICAL();
#endif
}

void debug__init_counters(void)
{
#if (DEBUG_ENABLE == 1)
  save_and_reset_counters();
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

void debug__one_second_print(void)
{
#if (DEBUG_ENABLE == 1)
  uint32_t time = scheduler__get_millisecond_ticks();
  debug_counters[timestamp_of_this_counter_save] = time;
  save_and_reset_counters();
  printf("Print Counters - Timestamp : %u\n", (unsigned int)time);
  printing_counters_flag = true;
  printing_counters_index = 0;
#endif
}

void debug__run_debug_print_state_machine(void)
{
  if (printing_counters_flag == true)
  {
    // #define NUMBER_OF_PRINTS_BEFORE_COUNTERS 2
    //  #define NUMBER_OF_PRINTS_AFTER_COUNTERS 30

    debug__debug_print_state_machine_printouts(printing_counters_index);
    printing_counters_index++;
    if (printing_counters_index >= NUMBER_OF_DEBUG_COUNTER_NAMES)
    {
      printf("\n");
      printing_counters_flag = false;
    }
  }
}

static void debug__debug_print_state_machine_printouts(printing_counters_index)
{
  switch (printing_counters_index)
  {
  // Counters to modify the printout
  case timestamp_of_this_counter_save:
    static uint32_t core_ticks_last_counter_save = 0;
    uint32_t core_ticks_now = DWT->CYCCNT;
    printf("-  Core Speed : %u , Ticks Since Last Save : %u\n", (unsigned int)CMU_ClockFreqGet(cmuClock_CORE), (unsigned int)(core_ticks_now - core_ticks_last_counter_save));
    core_ticks_last_counter_save = core_ticks_now;

    uint32_t difference = debug_counters_saved[timestamp_of_this_counter_save] - debug_counters_saved[timestamp_of_last_counter_save];
    printf("-  Milliseconds Since Last Save : %u\n", (unsigned int)difference);

    // static uint32_t microseconds_since_last_counter_save = 0;
    // microseconds_since_last_counter_save = 
    printf("-  Microseconds Since Last Save : %u\n", (unsigned int)(debug_counters_saved[number_of_micro_ticks]));
    break;

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
    if (debug_counters_saved[printing_counters_index] != 0)
    {
      printf("-  %s : %u\n", DEBUG_COUNTERS_STRINGS[printing_counters_index], (unsigned int)debug_counters_saved[printing_counters_index]);
    }
    break;

    // Counters to skip
  case timestamp_of_last_counter_save:
  case number_of_micro_ticks:
    break;
    // Print normally
  default:
    printf("-  %s : %u\n", DEBUG_COUNTERS_STRINGS[printing_counters_index], (unsigned int)debug_counters_saved[printing_counters_index]);
    break;
  }
}

// void debug__run_debug_print_state_machine(void)
// {
// #define NUMBER_OF_PRINTS 30

//   if (printing_counters_flag == true)
//   {
//     uint32_t core_ticks_since_last_run = DWT->CYCCNT - debug_counters[timestamp_last_reset];
//     uint32_t core_speed = CMU_ClockFreqGet(cmuClock_CORE);
//     switch (printing_counters_index)
//     {
//     case 0:
//       printf("-  Core Speed %u\n", (unsigned int)core_speed);
//       break;
//     case 1:
//       printf("-  Core Ticks Since Last Save: %u\n", (unsigned int)core_ticks_since_last_run);
//       break;
//     case 2:
//       printf("-  Core Ticks Since Last Save: (milliseconds): %u\n", (unsigned int)(core_ticks_since_last_run / (core_speed / 1000)));
//       break;
//     case 3:
//       printf("-  Ticks Between SysTicks: %u\n", (unsigned int)SysTick->LOAD);
//       break;
//     case 4:
//       printf("-  App Process Action Runs: %u\n", (unsigned int)debug_counters[app_process_action_runs]);
//       break;
//     case 5:
//       printf("-  Number of TX Send Attempt Success: %u\n", (unsigned int)debug_counters[number_of_TX_attempts]);
//       break;
//     case 6:
//       printf("-  Number of TX Send Attempt Failed: %u\n", (unsigned int)debug_counters[number_of_TX_failed]);
//       break;
//     case 7:
//       printf("-  Number of TX Sent Successes: %u\n", (unsigned int)debug_counters[number_of_TX_success]);
//       break;
//     case 8:
//       printf("-  Number of TX Sent Failed: %u\n", (unsigned int)debug_counters[number_of_TX_failed_to_send]);
//       break;
//     case 9:
//       if (debug_counters[number_of_TX_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Sent Failed Abort: %u\n", (unsigned int)debug_counters[number_of_TX_abort]);
//       }
//       break;
//     case 10:
//       if (debug_counters[number_of_TX_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Sent Failed Block: %u\n", (unsigned int)debug_counters[number_of_TX_block]);
//       }
//       break;
//     case 11:
//       if (debug_counters[number_of_TX_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Sent Failed Underflow: %u\n", (unsigned int)debug_counters[number_of_TX_underflow]);
//       }
//       break;
//     case 12:
//       if (debug_counters[number_of_TX_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Sent Failed Busy: %u\n", (unsigned int)debug_counters[number_of_TX_busy]);
//       }
//       break;
//     case 13:
//       if (debug_counters[number_of_TX_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Sent Failed Missed: %u\n", (unsigned int)debug_counters[number_of_TX_missed]);
//       }
//       break;

//     case 14:
//       printf("-  Number of TX Retry Send Attempt Success: %u\n", (unsigned int)debug_counters[number_of_TX_retry_attempts]);
//       break;
//     case 15:
//       printf("-  Number of TX Retry Send Attempt Failed: %u\n", (unsigned int)debug_counters[number_of_TX_retry_failed]);
//       break;
//     case 16:
//       printf("-  Number of TX Retry Sent Successes: %u\n", (unsigned int)debug_counters[number_of_TX_retry_success]);
//       break;
//     case 17:
//       printf("-  Number of TX Retry Sent Failed: %u\n", (unsigned int)debug_counters[number_of_TX_retry_failed_to_send]);
//       break;
//     case 18:
//       if (debug_counters[number_of_TX_retry_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Retry Sent Failed Abort: %u\n", (unsigned int)debug_counters[number_of_TX_retry_abort]);
//       }
//       break;
//     case 19:
//       if (debug_counters[number_of_TX_retry_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Retry Sent Failed Block: %u\n", (unsigned int)debug_counters[number_of_TX_retry_block]);
//       }
//       break;
//     case 20:
//       if (debug_counters[number_of_TX_retry_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Retry Sent Failed Underflow: %u\n", (unsigned int)debug_counters[number_of_TX_retry_underflow]);
//       }
//       break;
//     case 21:
//       if (debug_counters[number_of_TX_retry_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Retry Sent Failed Busy: %u\n", (unsigned int)debug_counters[number_of_TX_retry_busy]);
//       }
//       break;
//     case 22:
//       if (debug_counters[number_of_TX_retry_failed_to_send] > 0)
//       {
//         printf("  -  Number of TX Retry Sent Failed Missed: %u\n", (unsigned int)debug_counters[number_of_TX_retry_missed]);
//       }
//       break;

//     case 23:
//       printf("-  Number of Bytes Sent: %u\n", (unsigned int)debug_counters[number_of_TX_success] * RADIO_PACKET_DATA_SIZE);
//       break;
//     case 24:
//       printf("-  Number of Samples Sent: %u\n", (unsigned int)debug_counters[number_of_TX_success] * RADIO_PACKET_DATA_SIZE / 2);
//       break;
//     case 25:
//       printf("-  Number of Packet Buffer Overflows: %u\n", (unsigned int)debug_counters[packet_buffer_overflows]);
//       break;
//     case 26:
//       printf("-  Number of Left Samples Received: %u\n", (unsigned int)debug_counters[samples_received_left]);
//       break;
//     case 27:
//       printf("-  Number of Micro Ticks Since Last Run: %u\n", (unsigned int)debug_counters[number_of_micro_ticks]);
//       break;
//     case 28:
//       printf("-  Number of Right Samples Received: %u\n", (unsigned int)debug_counters[samples_received_right]);
//       break;
//     case 29:
//       printf("-  Number of Left Samples Received: %u\n", (unsigned int)debug_counters[samples_received_left]);
//       break;
//     case 30:
//       printf("-  Number of Right Buffers Created: %u\n", (unsigned int)debug_counters[right_buffers_written]);
//       break;
//     case 31:
//       printf("-  Number of Left Buffers Created: %u\n", (unsigned int)debug_counters[left_buffers_written]);
//       break;
//     case 32:
//       printf("-  Number of Right Buffers Consumed: %u\n", (unsigned int)debug_counters[right_buffers_consumed]);
//       break;
//     case 33:
//       printf("-  Number of Left Buffers Consumed: %u\n", (unsigned int)debug_counters[left_buffers_consumed]);
//       break;
//     case 29:
//       // uint32_t min_delta;
//       // uint32_t max_delta;
//       // uint32_t count;
//       // radio__get_tx_statistics(&max_delta, &min_delta, &count);
//       // printf("-  Max Delta: %u. , Min Delta : %u\n", (unsigned int)max_delta, (unsigned int)min_delta,);
//       break;
//     case 30:
//       printf("-  Sequence Number Start: %u - End : %u\n", (unsigned int)debug_counters[sequence_number_at_start], (unsigned int)debug_counters[sequence_number_at_end]);
//       break;
//     default:
//       printf("Invalid Print Index\n");
//       break;
//     }
//     printing_counters_index++;
//     if (printing_counters_index > NUMBER_OF_PRINTS)
//     {
//       printf("\n");
//       printing_counters_flag = false;
//     }
//   }
// }

void debug__audio_buffer_printf(const char *format, ...)
{
  va_list args;
  va_start(args, format);

  printf("Time: %u - ", (unsigned int)scheduler__get_millisecond_ticks());
  vprintf(format, args);

  va_end(args);
}
/*
#if (DEBUG==1)
//  save_and_reset_counters();
//  printf("Timestamp: %u\n", (unsigned int)get_millisecond_ticks());
//  uint32_t core_ticks_since_last_run = DWT->CYCCNT - non_volatile_counters.timestamp_last_reset;
//  uint32_t core_speed = CMU_ClockFreqGet(cmuClock_CORE);
//  printf("-  Core Speed %u\n", (unsigned int)core_speed);
//  printf("-  Core Ticks Since Last Save: %u\n", (unsigned int)core_ticks_since_last_run);
//  printf("-  Core Ticks Since Last Save: (milliseconds): %u\n", core_ticks_since_last_run / (core_speed/1000));
//  printf("-  Ticks Between SysTicks: %u\n", (unsigned int)SysTick->LOAD);
//  printf("-  App Process Action Runs: %u\n", (unsigned int)non_volatile_counters.app_process_action_runs);
//  printf("-  Number of TX Send Attempt Success: %u\n", (unsigned int)non_volatile_counters.number_of_TX_attempts);
//  printf("-  Number of TX Send Attempt Failed: %u\n", (unsigned int)non_volatile_counters.number_of_TX_failed);
//  printf("-  Number of TX Sent Successes: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success);
//  printf("-  Number of Bytes Sent: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success * 256);
//  printf("-  Number of Samples Sent: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success * 256  / 4);
//  printf("\n");
  static char buffer[100];
  static uint32_t length;
  //  sl_iostream_write(sl_iostream_inst_handle,buffer,4);

  save_and_reset_counters();
  length = sprintf(buffer,"Timestamp: %u\n", (unsigned int)get_millisecond_ticks());
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

  uint32_t core_ticks_since_last_run = DWT->CYCCNT - non_volatile_counters.timestamp_last_reset;
  uint32_t core_speed = CMU_ClockFreqGet(cmuClock_CORE);

//  printf("-  Core Speed %u\n", (unsigned int)core_speed);
  length = sprintf(buffer,"-  Core Speed %u\n", (unsigned int)core_speed);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  Core Ticks Since Last Save: %u\n", (unsigned int)core_ticks_since_last_run);
  length = sprintf(buffer,"-  Core Ticks Since Last Save: %u\n", (unsigned int)core_ticks_since_last_run);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  Core Ticks Since Last Save: (milliseconds): %u\n", core_ticks_since_last_run / (core_speed/1000));
  length = sprintf(buffer,"-  Core Ticks Since Last Save: (milliseconds): %u\n", core_ticks_since_last_run / (core_speed/1000));
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  Ticks Between SysTicks: %u\n", (unsigned int)SysTick->LOAD);
  length = sprintf(buffer,"-  Ticks Between SysTicks: %u\n", (unsigned int)SysTick->LOAD);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  App Process Action Runs: %u\n", (unsigned int)non_volatile_counters.app_process_action_runs);
  length = sprintf(buffer,"-  App Process Action Runs: %u\n", (unsigned int)non_volatile_counters.app_process_action_runs);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  Number of TX Send Attempt Success: %u\n", (unsigned int)non_volatile_counters.number_of_TX_attempts);
  length = sprintf(buffer,"-  Number of TX Send Attempt Success: %u\n", (unsigned int)non_volatile_counters.number_of_TX_attempts);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  Number of TX Send Attempt Failed: %u\n", (unsigned int)non_volatile_counters.number_of_TX_failed);
  length = sprintf(buffer,"-  Number of TX Send Attempt Failed: %u\n", (unsigned int)non_volatile_counters.number_of_TX_failed);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  Number of TX Sent Successes: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success);
  length = sprintf(buffer,"-  Number of TX Sent Successes: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  Number of Bytes Sent: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success * 256);
  length = sprintf(buffer,"-  Number of Bytes Sent: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success * 256);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

//  printf("-  Number of Samples Sent: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success * 256  / 4);
  length = sprintf(buffer,"-  Number of Samples Sent: %u\n", (unsigned int)non_volatile_counters.number_of_TX_success * 256  / 4);
  sl_iostream_write(sl_iostream_get_handle("swo"),buffer,length);

  printf("\n");
#endif
//  for (uint8_t i=0 ; i<sl_iostream_instances_count ; i++)
//    {
//      printf("Handle: %u\n", (unsigned int)sl_iostream_instances_info[i]->handle);
//      printf("Name: %s\n",sl_iostream_instances_info[i]->name);
//      printf("Id: %u\n", (unsigned int)sl_iostream_instances_info[i]->periph_id);
//      printf("Type: %X\n",sl_iostream_instances_info[i]->type);
//      printf("\n");
//    }
//
//  printf("EUSART En - %X\n",EUSART0->EN);
//  printf("EUSART CFG0 - %X\n",EUSART0->CFG0);
//  printf("EUSART CFG1 - %X\n",EUSART0->CFG1);
//  printf("EUSART CFG2 - %X\n",EUSART0->CFG2);
//  printf("EUSART CMD - %X\n",EUSART0->CMD);
//  printf("EUSART IF - %X\n",EUSART0->IF);
//  printf("EUSART IEN - %X\n",EUSART0->IEN);
//  printf("EUSART Status - %X\n",EUSART0->EN_CLR);

//  sl_iostream_set_default(sl_iostream_inst_handle);
//  sl_iostream_set_default(sl_iostream_get_handle("inst"));
//  printf("INST\n");
//  static char buffer[10] = {'T','E','S','T'};
//  sl_iostream_write(sl_iostream_inst_handle,buffer,4);
//  EUSART0->TXDATA = 0xFF;
//  EUSART0->CMD = 0x4;
//;
//  sl_iostream_set_default(sl_iostream_get_handle("swo"));
//  printf("VCOM\n");
//  printf("\n\n\n");
}
 */
