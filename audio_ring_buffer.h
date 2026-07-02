#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

void copy_data_into_buffer(uint8_t *input_data, bool right_data, uint32_t input_data_size_bytes);

#endif // AUDIO_BUFFER_H
