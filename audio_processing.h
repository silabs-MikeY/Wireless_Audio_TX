#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#include <stdbool.h>
#include <stdint.h>

void audio_processing__init(void);
void audio_processing__process_new_audio_data(uint8_t *buffer,
											  uint32_t buffer_size,
											  bool is_right_channel);
void audio_processing__test_adpcm_left(uint8_t *buffer);
void audio_processing__test_adpcm_right(uint8_t *buffer);


#endif // AUDIO_PROCESSING_H
