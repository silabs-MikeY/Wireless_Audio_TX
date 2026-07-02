#include "audio_ring_buffer.h"

#include <string.h>

#define AUDIO_RING_BUFFER_SIZE_BYTES 4096

uint8_t left_audio_ring_buffer[AUDIO_RING_BUFFER_SIZE_BYTES];
uint8_t right_audio_ring_buffer[AUDIO_RING_BUFFER_SIZE_BYTES];

uint32_t left_audio_ring_buffer_head = 0;
uint32_t right_audio_ring_buffer_head = 0;
uint32_t left_audio_ring_buffer_tail = 0;
uint32_t right_audio_ring_buffer_tail = 0;

void copy_data_into_buffer(uint8_t *input_data, bool right_data, uint32_t input_data_size_bytes)
{
    uint8_t *audio_ring_buffer = (right_data == false) ? left_audio_ring_buffer : right_audio_ring_buffer;
    uint32_t *audio_ring_buffer_head = (right_data == false) ? &left_audio_ring_buffer_head : &right_audio_ring_buffer_head;

    if (input_data_size_bytes == 0)
    {
        return;
    }

    if (input_data_size_bytes > AUDIO_RING_BUFFER_SIZE_BYTES)
    {
        input_data_size_bytes = AUDIO_RING_BUFFER_SIZE_BYTES;
    }

    uint32_t first_chunk_size_bytes = AUDIO_RING_BUFFER_SIZE_BYTES - *audio_ring_buffer_head;
    if (first_chunk_size_bytes > input_data_size_bytes)
    {
        first_chunk_size_bytes = input_data_size_bytes;
    }

    memcpy(&audio_ring_buffer[*audio_ring_buffer_head], input_data, first_chunk_size_bytes);
    *audio_ring_buffer_head += first_chunk_size_bytes;

    if (*audio_ring_buffer_head >= AUDIO_RING_BUFFER_SIZE_BYTES)
    {
        *audio_ring_buffer_head = 0;
    }

    if (first_chunk_size_bytes < input_data_size_bytes)
    {
        uint32_t remaining_size_bytes = input_data_size_bytes - first_chunk_size_bytes;
        memcpy(&audio_ring_buffer[*audio_ring_buffer_head], &input_data[first_chunk_size_bytes], remaining_size_bytes);
        *audio_ring_buffer_head += remaining_size_bytes;

        if (*audio_ring_buffer_head >= AUDIO_RING_BUFFER_SIZE_BYTES)
        {
            *audio_ring_buffer_head = 0;
        }
    }
}