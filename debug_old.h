#ifndef DEBUG_H_
#define DEBUG_H_

#include "em_core.h"
#include "em_chip.h"
#include "sl_core.h"
#include "string.h"
#include "scheduler.h"
#include "sl_iostream.h"
#include "sl_iostream_handles.h"
#include <stdio.h>
#include <stdarg.h>
#include "generic.h"
#include "radio_transmit.h"
#include "app_process.h"

// #include "radio.h"

#define DEBUG_ENABLE 1

#define DEBUG_ALL 0
#define DEBUG_PERIPHERALS
#define DEBUG_COUNTERS
#define DEBUG_STATES
//#define DEBUG_RETRY
//#define DEBUG_PACKETS

#if (DEBUG_ALL == 1)
#ifndef DEBUG_PERIPHERALS
#define DEBUG_PERIPHERALS 1
#endif

#ifndef DEBUG_COUNTERS
#define DEBUG_COUNTERS 1
#endif

#ifndef DEBUG_PACKETS
#define DEBUG_PACKETS 1
#endif
#endif



#ifdef DEBUG_PACKETS
#define DEBUG_PACKETS_LOG(x) x
#else
#define DEBUG_PACKETS_LOG(x)
#endif

#ifdef DEBUG_PERIPHERALS
#define DEBUG_PERIPHERALS_LOG(x) x
#else
#define DEBUG_PERIPHERALS_LOG(x)
#endif

#ifdef DEBUG_COUNTERS
#define DEBUG_COUNTERS_LOG(x) x
#else
#define DEBUG_COUNTERS_LOG(x)
#endif

#ifdef DEBUG_RETRY
#define DEBUG_RETRY_LOG(x) x
#else
#define DEBUG_RETRY_LOG(x)
#endif

#ifdef DEBUG_STATES
#define DEBUG_STATES_LOG(x) x
#else
#define DEBUG_STATES_LOG(x)
#endif

typedef enum
{
  timestamp_of_last_counter_save,
  timestamp_of_this_counter_save,
  app_process_action_runs,
  app_process_action_runs_max_time_between_runs,
  number_of_TX_attempts,
  number_of_TX_attempt_success,
  number_of_TX_attempt_failed,
  number_of_TX_success,
  number_of_TX_fail_failed_to_send,
  number_of_TX_fail_abort,
  number_of_TX_fail_block,
  number_of_TX_fail_underflow,
  number_of_TX_fail_busy,
  number_of_TX_fail_missed,
  number_of_TX_retry_attempts,
  number_of_TX_retry_attempt_success,
  number_of_TX_retry_attempt_failed,
  number_of_TX_retry_success,
  number_of_TX_retry_fail_failed_to_send,
  number_of_TX_retry_fail_abort,
  number_of_TX_retry_fail_block,
  number_of_TX_retry_fail_underflow,
  number_of_TX_retry_fail_busy,
  number_of_TX_retry_fail_missed,
  number_of_TX_packets_processed_after_sending,
  TX_micros_delta_min,
  TX_micros_delta_max,
  number_of_micro_ticks,
  samples_received_left,
  samples_received_right,
  left_buffers_written,
  right_buffers_written,
  left_buffers_consumed,
  right_buffers_consumed,
  packet_buffer_overflows,
  sequence_number_at_start,
  sequence_number_at_end,
  //Add new RMU reset causes here

  NUMBER_OF_DEBUG_COUNTER_NAMES, // Gets the number of RMU reset causes
} debug_counters_t;

#if (DEBUG_ENABLE == 1)
static const char *DEBUG_COUNTERS_STRINGS[] __attribute__ ((used)) = {
  "timestamp_of_last_counter_save",
  "timestamp_of_this_counter_save",
  "app_process_action_runs",
  "app_process_action_runs_max_time_between_runs",
  "number_of_TX_attempts",
  "number_of_TX_attempt_success",
  "number_of_TX_attempt_failed",
  "number_of_TX_success,",
  "number_of_TX_fail_failed_to_send",
  "number_of_TX_fail_abort",
  "number_of_TX_fail_block",
  "number_of_TX_fail_underflow",
  "number_of_TX_fail_busy",
  "number_of_TX_fail_missed",
  "number_of_TX_retry_attempts",
  "number_of_TX_retry_attempt_success",
  "number_of_TX_retry_attempt_failed",
  "number_of_TX_retry_success",
  "number_of_TX_retry_fail_failed_to_send",
  "number_of_TX_retry_fail_abort",
  "number_of_TX_retry_fail_block",
  "number_of_TX_retry_fail_underflow",
  "number_of_TX_retry_fail_busy",
  "number_of_TX_retry_fail_missed",
  "number_of_TX_packets_processed_after_sending",
  "TX_micros_delta_min",
  "TX_micros_delta_max",
  "number_of_micro_ticks",
  "samples_received_left",
  "samples_received_right",
  "left_buffers_written",
  "right_buffers_written",
  "left_buffers_consumed",
  "right_buffers_consumed",
  "packet_buffer_overflows",
  "sequence_number_at_start",
  "sequence_number_at_end",
};
#endif

void counters__init_counters(void);
void counters__increment_counter(uint32_t counter_index_to_increment);
void counters__add_to_counter(uint32_t counter_index_to_increment, uint32_t quantity_to_add);
void counters__set_counter(uint32_t counter_index_to_increment, uint32_t value_to_set_to);

// void debug__increment_number_of_TX_attempts(bool retry);
// void debug__increment_number_of_TX_failed(bool retry);
// void debug__log_TX_success(bool retry);
// void debug__log_TX_fail(bool retry);
// void debug__log_TX_abort(bool retry);
// void debug__log_TX_block(bool retry);
// void debug__log_TX_underflow(bool retry);
// void debug__log_TX_busy(bool retry);
// void debug__log_TX_missed(bool retry);

// void debug__log_TX_retry_success(void);
// void debug__log_TX_retry_fail(void);
// void debug__log_TX_retry_abort(void);
// void debug__log_TX_retry_block(void);
// void debug__log_TX_retry_underflow(void);
// void debug__log_TX_retry_busy(void);
// void debug__log_TX_retry_missed(void);

void counters__one_second_print(void);
// void debug__increment_app_process_action_runs(void);
void counters__run_debug_print_state_machine(void);
// void debug__increment_packet_buffer_overflows(void);
// void debug__set_start_sequence_number(uint16_t sequence_number);
// void debug__set_end_sequence_number(uint16_t sequence_number);
void debug__audio_buffer_printf(const char *format, ...);
// void debug__increment_number_of_left_samples_receiver(uint32_t number_of_new_samples);
// void debug__increment_number_of_right_samples_receiver(uint32_t number_of_new_samples);
// void debug__increment_number_of_micro_ticks(uint32_t size);

#endif /* DEBUG_H_ */
