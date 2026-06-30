#ifndef COUNTERS_H
#define COUNTERS_H

#include "em_core.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "print.h"

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
    audio_level_min_mv,
    audio_level_max_mv,
    left_buffers_written,
    right_buffers_written,
    left_buffers_consumed,
    right_buffers_consumed,
    packet_buffer_overflows,
    sequence_number_at_start,
    sequence_number_at_end,
    // Add new RMU reset causes here

    NUMBER_OF_DEBUG_COUNTER_NAMES, // Gets the number of RMU reset causes
} debug_counters_t;

#ifdef DEBUG_COUNTERS
static const char *DEBUG_COUNTERS_STRINGS[] __attribute__((used)) = {
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
    "audio_level_min_mv",
    "audio_level_max_mv",
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
void counters__one_second_print(void);
void counters__run_debug_print_state_machine(void);
void counters__reset_external_counters(void);

void counters__increment_counter(uint32_t counter_index_to_increment);
void counters__add_to_counter(uint32_t counter_index_to_increment, uint32_t quantity_to_add);
void counters__set_counter(uint32_t counter_index_to_set, uint32_t value_to_set_to);

#endif // COUNTERS_H