#include "radio_statistics.h"
#include "app_process.h"
#include "radio_base.h"
#include "scheduler.h"

#include <string.h>

static bool radio__validate_radio_tx_count(void);
void radio_statistics__validate_radio_statistics(void);

const mono_stereo_radio_config_values_t *stereo_or_mono_config = &mono_config;
volatile transmit_statistics_t transmit_statistics;
static bool radio_statistics_validate_timing = true;

// void radio_statistics__get_tx_statistics(transmit_statistics_t* transmit_statistics_t)
// {
//     return transmit_statistics;
// }

void radio_statistics__get_min_and_max_transmit_deltas(uint32_t *delta_min, uint32_t *delta_max)
{
    *delta_min = transmit_statistics.min_tx_timestamp_delta;
    *delta_max = transmit_statistics.max_tx_timestamp_delta;
}

void radio_statistics__get_number_of_transmits_processed(uint32_t *number_of_transmits_processed)
{
    *number_of_transmits_processed = transmit_statistics.number_of_TX_packets_sent_this_second;
}

// void radio_statistics__get_bad_timestamps(uint32_t* number_of_transmits_processed)
// {
//     *number_of_transmits_processed = transmit_statistics.number_of_TX_packets_sent_this_second;
// }

void radio_statistics__set_audio_mode(bool is_stereo, bool encoder_enabled)
{
  uint32_t packet_bytes_per_channel = is_stereo ? RADIO_PACKET_DATA_SIZE_PER_CHANNEL : RADIO_PACKET_DATA_SIZE;
  uint32_t samples_per_packet = 0;

  radio_statistics_validate_timing = !(is_stereo == false && encoder_enabled == false);

  if (encoder_enabled)
  {
    samples_per_packet = packet_bytes_per_channel * 2;
  }
  else
  {
    samples_per_packet = packet_bytes_per_channel / 2;
  }

  static mono_stereo_radio_config_values_t mode_config;
  mode_config.number_of_tx_per_second_expected = SAMPLE_FREQ / samples_per_packet;
  mode_config.expected_tx_microsecond_delta = 1000000 / mode_config.number_of_tx_per_second_expected;
  mode_config.acceptable_tx_microsecond_delta_lower = mode_config.expected_tx_microsecond_delta * (100 - TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100;
  mode_config.acceptable_tx_microsecond_delta_upper = mode_config.expected_tx_microsecond_delta * (100 + TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100;

  stereo_or_mono_config = &mode_config;

  radio__printf(true, "Set Config to %s%s\n", is_stereo ? "Stereo" : "Mono", encoder_enabled ? " + ADPCM" : "");
  radio__printf(true, "- Expected TX Count Per Second : %u\n", (unsigned int)stereo_or_mono_config->number_of_tx_per_second_expected);
  radio__printf(true, "- Expected TX Delta : %u\n", (unsigned int)stereo_or_mono_config->expected_tx_microsecond_delta);
  radio__printf(true, "- TX Upper Delta Threshold : %u\n", (unsigned int)stereo_or_mono_config->acceptable_tx_microsecond_delta_upper);
  radio__printf(true, "- TX Lower Delta Threshold : %u\n", (unsigned int)stereo_or_mono_config->acceptable_tx_microsecond_delta_lower);
}

void radio_statistics__init(void)
{
    radio__printf(true, "Init Radio Statistics\n");
    memset((void *)&transmit_statistics, 0, sizeof(transmit_statistics_t));
  radio_statistics__set_audio_mode(app_process__is_audio_stereo(), app_process__is_audio_encoder_enabled());
    transmit_statistics.moving_average_tx_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
}

void radio_statistics__reset_radio_statistics_for_new_measurement(void)
{
    uint32_t saved_last_tx_timestamp_micros = transmit_statistics.last_tx_timestamp_micros;
    memset((void *)&transmit_statistics, 0, sizeof(transmit_statistics_t));
    transmit_statistics.moving_average_tx_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
    transmit_statistics.last_tx_timestamp_micros = saved_last_tx_timestamp_micros;
    radio__printf(true, "Reset Radio Statistics\n");
}

static bool radio__validate_radio_tx_count(void)
{

  if ((transmit_statistics.number_of_TX_packets_sent_this_second >= ((stereo_or_mono_config->number_of_tx_per_second_expected) - 1)) && (transmit_statistics.number_of_TX_packets_sent_this_second <= ((stereo_or_mono_config->number_of_tx_per_second_expected) + 1)))
  {
    return true;
  }
  else
  {
    // MAYBE DO MORE
    return false;
  }
}

static bool radio__validate_radio_tx_timestamp_deltas(void)
{
  bool too_slow_flag = false;
  bool too_fast_flag = false;

  if (transmit_statistics.min_tx_timestamp_delta < (stereo_or_mono_config->acceptable_tx_microsecond_delta_lower))
  {
    // TX ING TOO FAST
    too_fast_flag = true;
  }
  if (transmit_statistics.max_tx_timestamp_delta > (stereo_or_mono_config->acceptable_tx_microsecond_delta_upper))
  {
    // TX ING TOO SLOW
    too_slow_flag = true;
  }
  return (too_fast_flag | too_slow_flag);
}

void radio_statistics__validate_radio_statistics(void)
{
  // TODO CRITICAl SECTION??
  static uint32_t last_validation_timestamp_millis = 0;

  if (radio_statistics_validate_timing == false)
  {
    last_validation_timestamp_millis = scheduler__get_millisecond_ticks();
    radio_statistics__reset_radio_statistics_for_new_measurement();
    return;
  }

  // validate period
  if (((scheduler__get_millisecond_ticks() - last_validation_timestamp_millis) >= 999) && ((scheduler__get_millisecond_ticks() - last_validation_timestamp_millis) <= 1001))
  {
    bool good_count = radio__validate_radio_tx_count();
    bool good_deltas = radio__validate_radio_tx_timestamp_deltas();
    (void)good_deltas;

    radio__printf(true, "-  Max Delta: %u. , Min Delta : %u\n", (unsigned int)transmit_statistics.max_tx_timestamp_delta, (unsigned int)transmit_statistics.min_tx_timestamp_delta);
    radio__printf(true, "-  TX Count : %u\n", (unsigned int)transmit_statistics.number_of_TX_packets_sent_this_second);
    if (transmit_statistics.bad_timestamps_count > 0)
    {
      radio__printf(true, "-  Bad Timestamps : \n");
      for (uint32_t i = 0; i < transmit_statistics.bad_timestamps_count; i++)
      {
        radio__printf(true, "   -- Seq: %u - Delta: %u - Timestamp: %u\n",
               (unsigned int)transmit_statistics.bad_timestamps[i].sequence_number,
               (unsigned int)transmit_statistics.bad_timestamps[i].timestamp_delta_from_previous,
               (unsigned int)transmit_statistics.bad_timestamps[i].tx_timestamp);
      }
    }
    if (good_count == false)
    {
      // if number_of_TX_packets_sent_this_second
    }
  }
  else
  {
    radio__printf(true, "-  Measurement Period Start : %u\n", (unsigned int)last_validation_timestamp_millis);
    radio__printf(true, "-  Measurement Period End : %u\n", (unsigned int)scheduler__get_millisecond_ticks());
    radio__printf(true, "-  Measurement Period Time : %u\n", (unsigned int)(scheduler__get_millisecond_ticks() - last_validation_timestamp_millis));
    // TODO HANDLE ELSE
  }

  last_validation_timestamp_millis = scheduler__get_millisecond_ticks();
  radio_statistics__reset_radio_statistics_for_new_measurement();
}

bool radio_statistics__note_successful_tx(uint32_t sequence_number, uint32_t timestamp_of_tx)
{
  if (radio_statistics_validate_timing == false)
  {
    transmit_statistics.last_tx_timestamp_micros = timestamp_of_tx;
    transmit_statistics.number_of_TX_packets_sent_this_second++;
    return true;
  }

    if (transmit_statistics.number_of_TX_packets_sent_this_second == 0)
    {
        // Rest Deltas
        transmit_statistics.max_tx_timestamp_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
        transmit_statistics.min_tx_timestamp_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
        radio__printf(true, "-  Reset Max Delta to: %u\n", (unsigned int)transmit_statistics.max_tx_timestamp_delta);
        radio__printf(true, "-  Reset Min Delta to: %u\n", (unsigned int)transmit_statistics.min_tx_timestamp_delta);

        //printf_to_buf_append_time(0,"-  New Max Delta: %u. , New Min Delta : %u\n", (unsigned int)transmit_statistics.max_tx_timestamp_delta, (unsigned int)transmit_statistics.min_tx_timestamp_delta);

        if (transmit_statistics.last_tx_timestamp_micros == 0)
        {
            // Only runs once at init
            // No previous TX so can't calculate a delta
            transmit_statistics.number_of_TX_packets_sent_this_second++;
            transmit_statistics.last_tx_timestamp_micros = timestamp_of_tx;
            
            return true;
        }
    }

        if (timestamp_of_tx < transmit_statistics.last_tx_timestamp_micros)
        {
          transmit_statistics.last_tx_timestamp_micros = timestamp_of_tx;
          transmit_statistics.number_of_TX_packets_sent_this_second++;
          return true;
        }

    uint32_t tx_delta = timestamp_of_tx - transmit_statistics.last_tx_timestamp_micros;

    if (tx_delta > stereo_or_mono_config->acceptable_tx_microsecond_delta_upper)
    {
      if (transmit_statistics.bad_timestamps_count >= BAD_TIMESTAMPS_SIZE)
      {
        radio__printf(true, "-  Bad Timestamps Limit Reached: \
  ");
        return true;
      }

        transmit_statistics.max_tx_timestamp_delta = tx_delta;
        //  printf_to_buf_append_time(0,"-  Max Delta Violation: %u  Max: %u\n", (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].sequence_number = sequence_number;
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].timestamp_delta_from_previous = tx_delta;
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].tx_timestamp = timestamp_of_tx;
        transmit_statistics.bad_timestamps_count++;
        radio__printf(true, "-  New Max Delta: %u  Seq : %u\n", (unsigned int)transmit_statistics.max_tx_timestamp_delta, sequence_number);
    }
    else if (tx_delta < stereo_or_mono_config->acceptable_tx_microsecond_delta_lower)
    {
      if (transmit_statistics.bad_timestamps_count >= BAD_TIMESTAMPS_SIZE)
      {
        radio__printf(true, "-  Bad Timestamps Limit Reached: \
  ");
        return true;
      }

        transmit_statistics.min_tx_timestamp_delta = tx_delta;
        //  printf_to_buf_append_time(0,"-  Min Delta Violation: %u  Min: %u\n", (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].sequence_number = sequence_number;
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].timestamp_delta_from_previous = tx_delta;
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].tx_timestamp = timestamp_of_tx;
        transmit_statistics.bad_timestamps_count++;
        radio__printf(true, "-  New Min Delta: %u  Seq : %u\n", (unsigned int)transmit_statistics.min_tx_timestamp_delta, sequence_number);
    }

    transmit_statistics.last_tx_timestamp_micros = timestamp_of_tx;
    transmit_statistics.number_of_TX_packets_sent_this_second++;
    return true;
}