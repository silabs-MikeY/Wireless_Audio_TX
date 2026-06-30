#ifndef RADIO_STATISTICS_H_
#define RADIO_STATISTICS_H_

#include "radio_base.h"
#include "hardware_config.h"
#include "audio_buffers.h"

#define NUMBER_OF_SAMPLES_PER_PACKET (RADIO_PACKET_DATA_SIZE / 2)
#define NUMBER_OF_TX_PER_SECOND_EXPECTED_STEREO (((SAMPLE_FREQ * 2) / NUMBER_OF_SAMPLES_PER_PACKET))
#define NUMBER_OF_TX_PER_SECOND_EXPECTED_MONO (SAMPLE_FREQ / NUMBER_OF_SAMPLES_PER_PACKET)

#define EXPECTED_TX_DELTA_MICROS_MONO (1000000 / NUMBER_OF_TX_PER_SECOND_EXPECTED_MONO)
#define EXPECTED_TX_DELTA_MICROS_STEREO (1000000 / NUMBER_OF_TX_PER_SECOND_EXPECTED_STEREO)

#define TX_DELTA_DEVIATION_ALLOWANCE_PERCENT 10

#define EXPECTED_TX_DELTA_MICROS_MONO_ACCEPTABLE_LOWER_BOUNDARY (EXPECTED_TX_DELTA_MICROS_MONO * (100 - TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100)
#define EXPECTED_TX_DELTA_MICROS_MONO_ACCEPTABLE_UPPER_BOUNDARY (EXPECTED_TX_DELTA_MICROS_MONO * (100 + TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100)

#define EXPECTED_TX_DELTA_MICROS_STEREO_ACCEPTABLE_LOWER_BOUNDARY (EXPECTED_TX_DELTA_MICROS_STEREO * (100 - TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100)
#define EXPECTED_TX_DELTA_MICROS_STEREO_ACCEPTABLE_UPPER_BOUNDARY (EXPECTED_TX_DELTA_MICROS_STEREO * (100 + TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100)

typedef struct bad_packet_timestamp_s
{
    uint32_t tx_timestamp;
    uint32_t timestamp_delta_from_previous;
    uint16_t sequence_number;
} bad_packet_timestamp_t;

#define BAD_TIMESTAMPS_SIZE 500
typedef struct transmit_statistics_s
{
    uint32_t number_of_TX_packets_sent_this_second;
    uint32_t max_tx_timestamp_delta;
    uint16_t min_tx_timestamp_delta;
    uint32_t last_tx_timestamp_micros;
    bad_packet_timestamp_t bad_timestamps[BAD_TIMESTAMPS_SIZE];
    uint32_t bad_timestamps_count;
    uint32_t moving_average_tx_delta;
} transmit_statistics_t;

typedef struct mono_stereo_radio_config_values_s
{
  uint32_t expected_tx_microsecond_delta;
  uint32_t acceptable_tx_microsecond_delta_upper;
  uint32_t acceptable_tx_microsecond_delta_lower;
    uint32_t number_of_tx_per_second_expected;
} mono_stereo_radio_config_values_t;

static const mono_stereo_radio_config_values_t mono_config = {
    EXPECTED_TX_DELTA_MICROS_MONO,
    EXPECTED_TX_DELTA_MICROS_MONO_ACCEPTABLE_UPPER_BOUNDARY,
    EXPECTED_TX_DELTA_MICROS_MONO_ACCEPTABLE_LOWER_BOUNDARY,
    NUMBER_OF_TX_PER_SECOND_EXPECTED_MONO
};

static const mono_stereo_radio_config_values_t stereo_config = {
    EXPECTED_TX_DELTA_MICROS_STEREO,
    EXPECTED_TX_DELTA_MICROS_STEREO_ACCEPTABLE_UPPER_BOUNDARY,
    EXPECTED_TX_DELTA_MICROS_STEREO_ACCEPTABLE_LOWER_BOUNDARY,
    NUMBER_OF_TX_PER_SECOND_EXPECTED_STEREO
};

void radio_statistics__reset_radio_statistics_for_new_measurement(void);
bool radio_statistics__note_successful_tx(uint32_t sequence_number, uint32_t timestamp_of_tx);
void radio_statistics__init(void);
void radio_statistics__get_min_and_max_transmit_deltas(uint32_t* delta_min, uint32_t* delta_max);
void radio_statistics__get_number_of_transmits_processed(uint32_t* number_of_transmits_processed);
void radio_statistics__set_stereo_or_mono_config(bool is_stereo);

#endif