#ifndef AUDIO_BUFFERS_H_
#define AUDIO_BUFFERS_H_

#include "hardware_config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NUMBER_OF_AUDIO_BUFFERS 20

typedef struct audio_buffer_s {
  uint8_t audio_buffer[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
  bool used;
  uint32_t micros_timestamp;
} audio_buffer_t;

uint32_t audio_buffers__get_number_of_counters(void);
const char *audio_buffers__get_counter_name(uint32_t counter_index);
volatile uint32_t *audio_buffers__get_counter_value(uint32_t counter_index);
void audio_buffers__reset_counters(void);
void audio_buffers__printf(bool add_timestamp, const char *format, ...);

void audio_buffers__add_new_data_to_left_buffer(uint8_t *data_pointer);
void audio_buffers__add_new_data_to_right_buffer(uint8_t *data_pointer);
void audio_buffers__init(bool is_stereo);
void audio_buffers__set_stereo_mode(bool enable);
bool audio_buffers__process_packet(uint8_t *left_or_first_buffer, uint8_t *right_or_second_buffer, bool is_stereo);
bool audio_buffers__run_process(void);
// bool is_stereo_mode(void);

#endif
