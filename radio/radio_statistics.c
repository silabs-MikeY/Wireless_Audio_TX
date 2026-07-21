#include "radio_statistics.h"
#include "app_process.h"

#include <string.h>

#define RADIO_STATISTICS_PRINT_DELTA_VIOLATIONS 0

__attribute__((weak)) void radio_statistics__printf(bool add_timestamp, const char *format, ...)
{
  (void)add_timestamp;
  (void)format;
}

static volatile uint32_t radio_statistics_counter_values[RADIO_STATISTICS_NUMBER_OF_COUNTERS] = {0};
static const char *radio_statistics_counter_names[RADIO_STATISTICS_NUMBER_OF_COUNTERS] = {
    "number_of_TX_packets_sent_this_second",
    "max_tx_timestamp_delta",
    "min_tx_timestamp_delta",
    "last_tx_timestamp_micros",
  "sequence_number_window_start",
  "sequence_number_window_end",
  "sequence_number_window_difference",
    "bad_timestamps_count",
    "moving_average_tx_delta",
};

const mono_stereo_radio_config_values_t *stereo_or_mono_config = &mono_config;
volatile transmit_statistics_t transmit_statistics;
static uint64_t tx_delta_accumulator = 0;
static uint32_t tx_delta_sample_count = 0;
static uint32_t saved_SEQUENCE_NUMBER_WINDOW_START = 0;
static uint32_t NUMBER_OF_TX_PACKETS_SENT_THIS_SECOND = 0;

static void radio_statistics__update_delta_counters(uint32_t tx_delta)
{
  if ((transmit_statistics.max_tx_timestamp_delta == 0) || (tx_delta > transmit_statistics.max_tx_timestamp_delta))
  {
    transmit_statistics.max_tx_timestamp_delta = tx_delta;
    radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_MAX_TX_TIMESTAMP_DELTA] = tx_delta;
  }

  if ((transmit_statistics.min_tx_timestamp_delta == 0) || (tx_delta < transmit_statistics.min_tx_timestamp_delta))
  {
    transmit_statistics.min_tx_timestamp_delta = tx_delta;
    radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_MIN_TX_TIMESTAMP_DELTA] = tx_delta;
  }
}

static void radio_statistics__accumulate_tx_delta(uint32_t tx_delta)
{
  tx_delta_accumulator += tx_delta;
  tx_delta_sample_count++;
}

void radio_statistics__prepare_counters_for_print(void)
{
  if (tx_delta_sample_count == 0)
  {
    transmit_statistics.moving_average_tx_delta = 0;
  }
  else
  {
    transmit_statistics.moving_average_tx_delta =
        (uint32_t)(tx_delta_accumulator / tx_delta_sample_count);
  }

    radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_SEQUENCE_NUMBER_WINDOW_START] = saved_SEQUENCE_NUMBER_WINDOW_START;
    radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_SEQUENCE_NUMBER_WINDOW_DIFFERENCE] = radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_SEQUENCE_NUMBER_WINDOW_END] - saved_SEQUENCE_NUMBER_WINDOW_START;
    saved_SEQUENCE_NUMBER_WINDOW_START = radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_SEQUENCE_NUMBER_WINDOW_END];
    radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_NUMBER_OF_TX_PACKETS_SENT_THIS_SECOND] = NUMBER_OF_TX_PACKETS_SENT_THIS_SECOND;
    NUMBER_OF_TX_PACKETS_SENT_THIS_SECOND = 0;

  radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_MOVING_AVERAGE_TX_DELTA] = transmit_statistics.moving_average_tx_delta;
}

uint32_t radio_statistics__get_number_of_counters(void)
{
  return RADIO_STATISTICS_NUMBER_OF_COUNTERS;
}

const char *radio_statistics__get_counter_name(uint32_t counter_index)
{
  if (counter_index >= RADIO_STATISTICS_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return radio_statistics_counter_names[counter_index];
}

volatile uint32_t *radio_statistics__get_counter_address(uint32_t counter_index)
{
  if (counter_index >= RADIO_STATISTICS_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return &radio_statistics_counter_values[counter_index];
}

void radio_statistics__reset_counters(void)
{
  radio_statistics__reset_radio_statistics_for_new_measurement();
}

// void radio_statistics__get_tx_statistics(transmit_statistics_t*
// transmit_statistics_t)
// {
//     return transmit_statistics;
// }

void radio_statistics__get_min_and_max_transmit_deltas(uint32_t *delta_min,
                                                       uint32_t *delta_max) {
  *delta_min = radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_MIN_TX_TIMESTAMP_DELTA];
  *delta_max = radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_MAX_TX_TIMESTAMP_DELTA];
}

void radio_statistics__get_number_of_transmits_processed(
    uint32_t *number_of_transmits_processed) {
  *number_of_transmits_processed = radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_NUMBER_OF_TX_PACKETS_SENT_THIS_SECOND];
}

// void radio_statistics__get_bad_timestamps(uint32_t*
// number_of_transmits_processed)
// {
//     *number_of_transmits_processed =
//     transmit_statistics.number_of_TX_packets_sent_this_second;
// }

void radio_statistics__set_audio_mode(bool is_stereo, bool encoder_enabled) {
  uint32_t packet_bytes_per_channel =
      is_stereo ? RADIO_PACKET_DATA_SIZE_PER_CHANNEL : RADIO_PACKET_DATA_SIZE;
  uint32_t samples_per_packet = 0;

  if (encoder_enabled) {
    samples_per_packet = packet_bytes_per_channel * 2;
  } else {
    samples_per_packet = packet_bytes_per_channel / 2;
  }

  static mono_stereo_radio_config_values_t mode_config;
  mode_config.number_of_tx_per_second_expected =
      SAMPLE_FREQ / samples_per_packet;
  mode_config.expected_tx_microsecond_delta =
      1000000 / mode_config.number_of_tx_per_second_expected;
  mode_config.acceptable_tx_microsecond_delta_lower =
      mode_config.expected_tx_microsecond_delta *
      (100 - TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100;
  mode_config.acceptable_tx_microsecond_delta_upper =
      mode_config.expected_tx_microsecond_delta *
      (100 + TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100;

  stereo_or_mono_config = &mode_config;

    radio_statistics__printf(true, "Set Config to %s%s\n", is_stereo ? "Stereo" : "Mono",
          encoder_enabled ? " + ADPCM" : "");
    radio_statistics__printf(
      true, "- Expected TX Count Per Second : %u\n",
      (unsigned int)stereo_or_mono_config->number_of_tx_per_second_expected);
    radio_statistics__printf(
      true, "- Expected TX Delta : %u\n",
      (unsigned int)stereo_or_mono_config->expected_tx_microsecond_delta);
    radio_statistics__printf(true, "- TX Upper Delta Threshold : %u\n",
                (unsigned int)stereo_or_mono_config
                    ->acceptable_tx_microsecond_delta_upper);
    radio_statistics__printf(true, "- TX Lower Delta Threshold : %u\n",
                (unsigned int)stereo_or_mono_config
                    ->acceptable_tx_microsecond_delta_lower);
}

bool radio_statistics__init(void) {
    radio_statistics__printf(true, "Init Radio Statistics\n");
  memset((void *)&transmit_statistics, 0, sizeof(transmit_statistics_t));
    radio_statistics__reset_counters();
  radio_statistics__set_audio_mode(app_process__is_audio_stereo(),
                                   app_process__is_audio_encoder_enabled());
  saved_SEQUENCE_NUMBER_WINDOW_START = 0;
    transmit_statistics.moving_average_tx_delta = 0;
  radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_MOVING_AVERAGE_TX_DELTA] =
      transmit_statistics.moving_average_tx_delta;
  return true;
}

void radio_statistics__reset_radio_statistics_for_new_measurement(void) {
  uint32_t saved_last_tx_timestamp_micros =
      transmit_statistics.last_tx_timestamp_micros;
  memset((void *)&transmit_statistics, 0, sizeof(transmit_statistics_t));
  memset((void *)radio_statistics_counter_values, 0, sizeof(radio_statistics_counter_values));
  transmit_statistics.last_tx_timestamp_micros = saved_last_tx_timestamp_micros;
  tx_delta_accumulator = 0;
  tx_delta_sample_count = 0;
  radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_SEQUENCE_NUMBER_WINDOW_START] = saved_SEQUENCE_NUMBER_WINDOW_START;
  // radio_statistics__printf(true, "Reset Radio Statistics\n");
}

void radio_statistics__validate_radio_statistics(void) {
  return;
}

bool radio_statistics__note_successful_tx(uint32_t sequence_number,
                                          uint32_t timestamp_of_tx) {
  // radio__printf(true, "TX: Seq: %u\n", (unsigned int)sequence_number);

  NUMBER_OF_TX_PACKETS_SENT_THIS_SECOND++;
  radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_SEQUENCE_NUMBER_WINDOW_END] = sequence_number;

  if (NUMBER_OF_TX_PACKETS_SENT_THIS_SECOND == 1) {
    if (transmit_statistics.last_tx_timestamp_micros == 0) {
      // Only runs once at init
      // No previous TX so can't calculate a delta
      transmit_statistics.last_tx_timestamp_micros = timestamp_of_tx;
      radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_LAST_TX_TIMESTAMP_MICROS] = transmit_statistics.last_tx_timestamp_micros;

      return true;
    }
  }

  if (timestamp_of_tx < transmit_statistics.last_tx_timestamp_micros) {
    transmit_statistics.last_tx_timestamp_micros = timestamp_of_tx;
    radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_LAST_TX_TIMESTAMP_MICROS] = transmit_statistics.last_tx_timestamp_micros;
    return true;
  }

  uint32_t tx_delta =
      timestamp_of_tx - transmit_statistics.last_tx_timestamp_micros;

  radio_statistics__update_delta_counters(tx_delta);
  radio_statistics__accumulate_tx_delta(tx_delta);

  if (tx_delta > stereo_or_mono_config->acceptable_tx_microsecond_delta_upper) {
    if (transmit_statistics.bad_timestamps_count >= BAD_TIMESTAMPS_SIZE) {
        radio_statistics__printf(true, "-  Bad Timestamps Limit Reached: \
  ");
      return true;
    }

    //  printf_to_buf_append_time(0,"-  Max Delta Violation: %u  Max: %u\n",
    //  (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
    transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count]
        .sequence_number = sequence_number;
    transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count]
        .timestamp_delta_from_previous = tx_delta;
    transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count]
        .tx_timestamp = timestamp_of_tx;
    transmit_statistics.bad_timestamps_count++;
    radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_BAD_TIMESTAMPS_COUNT] = transmit_statistics.bad_timestamps_count;
#if (RADIO_STATISTICS_PRINT_DELTA_VIOLATIONS == 1)
    radio_statistics__printf(true, "-  New Max Delta: %u  Seq : %u\n",
                  (unsigned int)transmit_statistics.max_tx_timestamp_delta,
                  sequence_number);
#endif
  } else if (tx_delta <
             stereo_or_mono_config->acceptable_tx_microsecond_delta_lower) {
    if (transmit_statistics.bad_timestamps_count >= BAD_TIMESTAMPS_SIZE) {
        radio_statistics__printf(true, "-  Bad Timestamps Limit Reached: \
  ");
      return true;
    }

    //  printf_to_buf_append_time(0,"-  Min Delta Violation: %u  Min: %u\n",
    //  (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
    transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].sequence_number = sequence_number;
    transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count]
        .timestamp_delta_from_previous = tx_delta;
    transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count]
        .tx_timestamp = timestamp_of_tx;
    transmit_statistics.bad_timestamps_count++;
    radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_BAD_TIMESTAMPS_COUNT] = transmit_statistics.bad_timestamps_count;
#if (RADIO_STATISTICS_PRINT_DELTA_VIOLATIONS == 1)
    radio_statistics__printf(true, "-  New Min Delta: %u  Seq : %u\n",
                  (unsigned int)transmit_statistics.min_tx_timestamp_delta,
                  sequence_number);
#endif
  }

  transmit_statistics.last_tx_timestamp_micros = timestamp_of_tx;
  radio_statistics_counter_values[RADIO_STATISTICS_COUNTER_LAST_TX_TIMESTAMP_MICROS] = transmit_statistics.last_tx_timestamp_micros;
  return true;
}
