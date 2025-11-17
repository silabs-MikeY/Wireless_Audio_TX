#ifndef RADIO_H_
#define RADIO_H_

#include "rail_types.h"
#include "sl_rail_util_init.h"
#include "ADC.h"
#include "em_prs.h"
#include "debug.h"
#include "events_prints.h"
#include "state_machine.h"
#include "microseconds.h"
#include "hardware_config.h"
#include "../radio/radio_retry.h"

//#define DEBUG_RADIO

// typedef struct header_s {
//   uint8_t size;
//   uint8_t control_bits;
//   uint16_t sequence_number;
// } __attribute__((aligned(4))) header_t;

// typedef struct payload_s {
//   header_t header;
//   uint8_t data_left[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
//   uint8_t data_right[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
// } __attribute__((aligned(4)))payload_t;

// typedef struct packet_buffer_s {
//   payload_t payload;
//   bool used;
//   bool ready_to_be_sent;
//   bool sent;
//   bool left_data_present;
//   bool right_data_present;
// } __attribute__((aligned(4)))packet_buffer_t;

// #define NUMBER_OF_PACKET_BUFFERS 20

// #define CONTROL_BITS__STEREO BIT(0)
// #define CONTROL_BITS__RETRY BIT(1)
// #define CONTROL_BITS__COMMAND_PACKET BIT(2)
// #define CONTROL_BITS__BIT3_UNUSED BIT(3)
// #define CONTROL_BITS__BIT4_UNUSED BIT(4)
// #define CONTROL_BITS__BIT5_UNUSED BIT(5)
// #define CONTROL_BITS__BIT6_UNUSED BIT(6)
// #define CONTROL_BITS__BIT7_UNUSED BIT(7)




// #define NUMBER_OF_SAMPLES_PER_PACKET (RADIO_PACKET_DATA_SIZE / 2)
// #define NUMBER_OF_TX_PER_SECOND_EXPECTED_STEREO (((SAMPLE_FREQ * 2) / NUMBER_OF_SAMPLES_PER_PACKET))
// #define NUMBER_OF_TX_PER_SECOND_EXPECTED_MONO (SAMPLE_FREQ / NUMBER_OF_SAMPLES_PER_PACKET)

// #define EXPECTED_TX_DELTA_MICROS_MONO (1000000 / NUMBER_OF_TX_PER_SECOND_EXPECTED_MONO)
// #define EXPECTED_TX_DELTA_MICROS_STEREO (1000000 / NUMBER_OF_TX_PER_SECOND_EXPECTED_STEREO)

// #define TX_DELTA_DEVIATION_ALLOWANCE_PERCENT 10

// #define EXPECTED_TX_DELTA_MICROS_MONO_ACCEPTABLE_LOWER_BOUNDARY (EXPECTED_TX_DELTA_MICROS_MONO * (100 - TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100)
// #define EXPECTED_TX_DELTA_MICROS_MONO_ACCEPTABLE_UPPER_BOUNDARY (EXPECTED_TX_DELTA_MICROS_MONO * (100 + TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100)

// #define EXPECTED_TX_DELTA_MICROS_STEREO_ACCEPTABLE_LOWER_BOUNDARY (EXPECTED_TX_DELTA_MICROS_STEREO * (100 - TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100)
// #define EXPECTED_TX_DELTA_MICROS_STEREO_ACCEPTABLE_UPPER_BOUNDARY (EXPECTED_TX_DELTA_MICROS_STEREO * (100 + TX_DELTA_DEVIATION_ALLOWANCE_PERCENT) / 100)

// typedef struct mono_stereo_radio_config_values_s
// {
//   uint32_t expected_tx_microsecond_delta;
//   uint32_t acceptable_tx_microsecond_delta_upper;
//   uint32_t acceptable_tx_microsecond_delta_lower;
//   uint16_t number_of_tx_per_second_expected;
// } mono_stereo_radio_config_values_t;

// static const mono_stereo_radio_config_values_t mono_config = {
//     EXPECTED_TX_DELTA_MICROS_MONO,
//     EXPECTED_TX_DELTA_MICROS_MONO_ACCEPTABLE_UPPER_BOUNDARY,
//     EXPECTED_TX_DELTA_MICROS_MONO_ACCEPTABLE_LOWER_BOUNDARY,
//     NUMBER_OF_TX_PER_SECOND_EXPECTED_MONO
// };

// static const mono_stereo_radio_config_values_t stereo_config = {
//     EXPECTED_TX_DELTA_MICROS_STEREO,
//     EXPECTED_TX_DELTA_MICROS_STEREO_ACCEPTABLE_UPPER_BOUNDARY,
//     EXPECTED_TX_DELTA_MICROS_STEREO_ACCEPTABLE_LOWER_BOUNDARY,
//     NUMBER_OF_TX_PER_SECOND_EXPECTED_STEREO
// };

// void radio__init_packet_buffers(void);
// void radio__run_process(void);
// void radio__add_channel_data_to_buffer(uint8_t* new_data_pointer, bool right_data);
// void radio__process_event(RAIL_Handle_t rail_handle, RAIL_Events_t events);
// bool radio__indicate_data_buffer_was_filled_by_pointer(bool left_or_right_data);
// uint8_t* radio__get_pointer_to_data_buffer_channel(bool left_or_right_data);
// void radio__radio_init(void);
// void radio__radio_deinit(void);
// void radio__increment_channel(void);
// bool radio__get_channel_changed_flag(void);
// void radio__reset_channel_chanegd_flag(void);
// uint32_t radio__get_channel(void);
// uint32_t radio__get_sequence_number(void);
// void radio__get_tx_statistics(uint32_t* tx_timestamp_delta_max, uint32_t* tx_timestamp_delta_min, uint32_t* tx_count);
// void radio__validate_radio_statistics(void);
// bool radio__send_packet_by_sequence_number(uint16_t sequence_number, bool retry);
// bool radio__is_radio_busy(void);

#endif /* RADIO_H_ */
