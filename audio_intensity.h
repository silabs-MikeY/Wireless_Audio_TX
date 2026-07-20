#ifndef AUDIO_INTENSITY_H
#define AUDIO_INTENSITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HIGH_THRESHOLD 3
#define MEDIUM_THRESHOLD 2
#define LOW_THRESHOLD 1

uint32_t audio_intensity__get_microsecond_ticks(void);
int32_t audio_intensity__check_buffer(uint8_t *buffer);
uint32_t audio_intensity__get_intensity(void);
int32_t audio_intensity__init(bool little_endian_flag_param,
                              uint32_t sample_size_bytes_param,
                              uint32_t buffer_size_bytes_param);
uint32_t audio_intensity__get_number_of_counters(void);
const char *audio_intensity__get_counter_name(uint32_t counter_index);
volatile uint32_t *audio_intensity__get_counter_address(uint32_t counter_index);
void audio_intensity__reset_counters(void);

#endif // AUDIO_INTENSITY_H
