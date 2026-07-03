#ifndef AUDIO_ENCODING_H
#define AUDIO_ENCODING_H

#include <stdint.h>
#include <stdbool.h>

void audio_encoding__init(void);
void audio_encoding__test_adpcm_left(uint8_t *buffer);
void audio_encoding__test_adpcm_right(uint8_t *buffer);

bool audio_encoding__convert_raw_audio_to_pcm16(const uint8_t *input,
                                                       int16_t *output,
                                                       uint32_t input_size);


#endif // AUDIO_ENCODING_H