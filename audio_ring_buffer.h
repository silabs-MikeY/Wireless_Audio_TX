#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "radio_packet_format.h"

// Counters
uint32_t ring_buffer__get_number_of_counters(void);
const char* ring_buffer__get_counter_name(uint32_t counter_index);
volatile uint32_t* ring_buffer__get_counter_address(uint32_t counter_index);
void ring_buffer__reset_counters(void);
// Counters End

void ring_buffer__copy_data_into_ring_buffer(uint8_t *input_data, bool right_data, uint32_t input_data_size_bytes);
bool ring_buffer__init(bool is_stereo, bool enable_encoder);
bool ring_buffer__build_radio_packet_from_ring_buffer(bool is_stereo, bool enable_encoder, payload_t *radio_packet_payload);

#endif // AUDIO_BUFFER_H
