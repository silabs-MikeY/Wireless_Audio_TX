#include "radio_statistics.h"
#include "print.h"
#include "state_machine.h"

static bool radio__validate_radio_tx_count(void);
void radio_statistics__validate_radio_statistics(void);

const mono_stereo_radio_config_values_t *stereo_or_mono_config = &mono_config;
volatile transmit_statistics_t transmit_statistics;

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

void radio_statistics__set_stereo_or_mono_config(bool is_stereo)
{
    if (is_stereo == false)
    {
        stereo_or_mono_config = &mono_config;
        //memcpy(&stereo_or_mono_config, &mono_config, sizeof(mono_stereo_radio_config_values_t));
        debug__printf_to_buf_append_time(0,"Set Config to Mono\n");
    }
    else
    {
        stereo_or_mono_config = &stereo_config;
        //memcpy(&stereo_or_mono_config, &stereo_config, sizeof(mono_stereo_radio_config_values_t));
        debug__printf_to_buf_append_time(0,"Set Config to Stereo\n");
    }
    debug__printf_to_buf_append_time(0,"- Expected TX Count Per Second : %u\n", stereo_or_mono_config->number_of_tx_per_second_expected);
    debug__printf_to_buf_append_time(0,"- Expected TX Delta : %u\n", stereo_or_mono_config->expected_tx_microsecond_delta);
    debug__printf_to_buf_append_time(0,"- TX Upper Delta Threshold : %u\n", stereo_or_mono_config->acceptable_tx_microsecond_delta_upper);
    debug__printf_to_buf_append_time(0,"- TX Lower Delta Threshold : %u\n", stereo_or_mono_config->acceptable_tx_microsecond_delta_lower);
}

void radio_statistics__init(void)
{
    debug__printf_to_buf_append_time(0,"Init Radio Statistics\n");
    memset((void *)&transmit_statistics, 0, sizeof(transmit_statistics_t));
    transmit_statistics.moving_average_tx_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
    radio_statistics__set_stereo_or_mono_config(false);
}

void radio_statistics__reset_radio_statistics_for_new_measurement(void)
{
    uint32_t saved_last_tx_timestamp_micros = transmit_statistics.last_tx_timestamp_micros;
    memset((void *)&transmit_statistics, 0, sizeof(transmit_statistics_t));
    transmit_statistics.moving_average_tx_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
    transmit_statistics.last_tx_timestamp_micros = saved_last_tx_timestamp_micros;
    debug__printf_to_buf_append_time(0,"Reset Radio Statistics\n");
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

  // validate period
  if (((scheduler__get_millisecond_ticks() - last_validation_timestamp_millis) >= 999) && ((scheduler__get_millisecond_ticks() - last_validation_timestamp_millis) <= 1001))
  {
    bool good_count = radio__validate_radio_tx_count();
    bool good_deltas = radio__validate_radio_tx_timestamp_deltas();
    (void)good_deltas;

    debug__printf_to_buf_append_time(0,"-  Max Delta: %u. , Min Delta : %u\n", (unsigned int)transmit_statistics.max_tx_timestamp_delta, (unsigned int)transmit_statistics.min_tx_timestamp_delta);
    debug__printf_to_buf_append_time(0,"-  TX Count : %u\n", (unsigned int)transmit_statistics.number_of_TX_packets_sent_this_second);
    if (transmit_statistics.bad_timestamps_count > 0)
    {
      debug__printf_to_buf_append_time(0,"-  Bad Timestamps : \n");
      for (uint32_t i = 0; i < transmit_statistics.bad_timestamps_count; i++)
      {
        debug__printf_to_buf_append_time(0,"   -- Seq: %u - Delta: %u - Timestamp: %u\n",
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
    debug__printf_to_buf_append_time(0,"-  Measurement Period Start : %u\n", (unsigned int)last_validation_timestamp_millis);
    debug__printf_to_buf_append_time(0,"-  Measurement Period End : %u\n", (unsigned int)scheduler__get_millisecond_ticks());
    debug__printf_to_buf_append_time(0,"-  Measurement Period Time : %u\n", (unsigned int)(scheduler__get_millisecond_ticks() - last_validation_timestamp_millis));
    // TODO HANDLE ELSE
  }

  last_validation_timestamp_millis = scheduler__get_millisecond_ticks();
  radio_statistics__reset_radio_statistics_for_new_measurement();
}

bool radio_statistics__note_successful_tx(uint32_t sequence_number, uint32_t timestamp_of_tx)
{
    if (transmit_statistics.number_of_TX_packets_sent_this_second == 0)
    {
        // Rest Deltas
        transmit_statistics.max_tx_timestamp_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
        transmit_statistics.min_tx_timestamp_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
        debug__printf_to_buf_append_time(0,"-  Reset Max Delta to: %u\n", (unsigned int)transmit_statistics.max_tx_timestamp_delta);
        debug__printf_to_buf_append_time(0,"-  Reset Min Delta to: %u\n", (unsigned int)transmit_statistics.min_tx_timestamp_delta);

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

    uint32_t tx_delta = timestamp_of_tx - transmit_statistics.last_tx_timestamp_micros;

    if (tx_delta > stereo_or_mono_config->acceptable_tx_microsecond_delta_upper)
    {
        transmit_statistics.max_tx_timestamp_delta = tx_delta;
        //  printf_to_buf_append_time(0,"-  Max Delta Violation: %u  Max: %u\n", (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].sequence_number = sequence_number;
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].timestamp_delta_from_previous = tx_delta;
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].tx_timestamp = timestamp_of_tx;
        transmit_statistics.bad_timestamps_count++;
        debug__printf_to_buf_append_time(0,"-  New Max Delta: %u  Seq : %u\n", (unsigned int)transmit_statistics.max_tx_timestamp_delta, sequence_number);
    }
    else if (tx_delta < stereo_or_mono_config->acceptable_tx_microsecond_delta_lower)
    {
        transmit_statistics.min_tx_timestamp_delta = tx_delta;
        //  printf_to_buf_append_time(0,"-  Min Delta Violation: %u  Min: %u\n", (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].sequence_number = sequence_number;
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].timestamp_delta_from_previous = tx_delta;
        transmit_statistics.bad_timestamps[transmit_statistics.bad_timestamps_count].tx_timestamp = timestamp_of_tx;
        transmit_statistics.bad_timestamps_count++;
        debug__printf_to_buf_append_time(0,"-  New Min Delta: %u  Seq : %u\n", (unsigned int)transmit_statistics.min_tx_timestamp_delta, sequence_number);
    }

    if (transmit_statistics.bad_timestamps_count > BAD_TIMESTAMPS_SIZE)
    {
        debug__printf_to_buf_append_time(0,"-  Bad Timestamps Limit Reached: \n");
        for (uint32_t i = 0; i < transmit_statistics.bad_timestamps_count; i++)
        {
            debug__printf_to_buf_append_time(0,"  %u -- Seq: %u - Delta: %u - Timestamp: %u\n",
                   (unsigned int)i,
                   (unsigned int)transmit_statistics.bad_timestamps[i].sequence_number,
                   (unsigned int)transmit_statistics.bad_timestamps[i].timestamp_delta_from_previous,
                   (unsigned int)transmit_statistics.bad_timestamps[i].tx_timestamp);
        }
        state_machine__force_state_machine_error();
        // assert(0);
    }

    transmit_statistics.last_tx_timestamp_micros = scheduler__get_microsecond_ticks();
    transmit_statistics.number_of_TX_packets_sent_this_second++;
    return true;
}