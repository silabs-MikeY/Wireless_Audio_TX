#include "audio_ring_buffer.h"

#include <string.h>

// -----------------------------------------------------------------------------
//                     Exported Counters
// -----------------------------------------------------------------------------

typedef enum ring_buffer_counter_index_s {
  RING_BUFFER_LEFT_BYTES_IN = 0,
  RING_BUFFER_RIGHT_BYTES_IN = 1,
  RING_BUFFER_LEFT_BYTES_OUT = 2,
  RING_BUFFER_RIGHT_BYTES_OUT = 3,
  RING_BUFFER_NUMBER_OF_COUNTERS
} ring_buffer_counter_index_t;

static volatile uint32_t ring_buffer_counter_values[RING_BUFFER_NUMBER_OF_COUNTERS] = {0};
static const char *ring_buffer_counter_names[RING_BUFFER_NUMBER_OF_COUNTERS] = {
    "ring_buffer_left_bytes_in",
    "ring_buffer_right_bytes_in",
    "ring_buffer_left_bytes_out",
    "ring_buffer_right_bytes_out"
};

uint32_t ring_buffer__get_number_of_counters(void)
{
  return RING_BUFFER_NUMBER_OF_COUNTERS;
}
const char* ring_buffer__get_counter_name(uint32_t counter_index)
{
  if (counter_index >= RING_BUFFER_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return ring_buffer_counter_names[counter_index];
}
volatile uint32_t* ring_buffer__get_counter_address(uint32_t counter_index)
{
  if (counter_index >= RING_BUFFER_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return &ring_buffer_counter_values[counter_index];
}
void ring_buffer__reset_counters(void)
{
  for (uint32_t i = 0; i < RING_BUFFER_NUMBER_OF_COUNTERS; i++)
  {
    ring_buffer_counter_values[i] = 0;
  }
}

// -----------------------------------------------------------------------------
//                     Exported Counters End
// -----------------------------------------------------------------------------

#define AUDIO_RING_BUFFER_SIZE_BYTES 4096

uint8_t left_audio_ring_buffer[AUDIO_RING_BUFFER_SIZE_BYTES];
uint8_t right_audio_ring_buffer[AUDIO_RING_BUFFER_SIZE_BYTES];

uint32_t left_audio_ring_buffer_head = 0;
uint32_t right_audio_ring_buffer_head = 0;
uint32_t left_audio_ring_buffer_tail = 0;
uint32_t right_audio_ring_buffer_tail = 0;

static bool stereo_flag = false;
static bool encoder_enabled = false;

void ring_buffer__init(bool is_stereo, bool enable_encoder)
{
    stereo_flag = is_stereo;
    encoder_enabled = enable_encoder;
    memset(left_audio_ring_buffer, 0, AUDIO_RING_BUFFER_SIZE_BYTES);
    memset(right_audio_ring_buffer, 0, AUDIO_RING_BUFFER_SIZE_BYTES);
    left_audio_ring_buffer_head = 0;
    right_audio_ring_buffer_head = 0;
    left_audio_ring_buffer_tail = 0;
    right_audio_ring_buffer_tail = 0;
}

void ring_buffer__copy_data_into_ring_buffer(uint8_t *input_data, bool right_data, uint32_t input_data_size_bytes)
{
    uint8_t *audio_ring_buffer = (right_data == false) ? left_audio_ring_buffer : right_audio_ring_buffer;
    uint32_t *audio_ring_buffer_head = (right_data == false) ? &left_audio_ring_buffer_head : &right_audio_ring_buffer_head;
    uint32_t *audio_ring_buffer_tail = (right_data == false) ? &left_audio_ring_buffer_tail : &right_audio_ring_buffer_tail;

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
    if (*audio_ring_buffer_head == *audio_ring_buffer_tail)
    {
        // Buffer overflow, move tail forward to make space
        *audio_ring_buffer_tail = (*audio_ring_buffer_tail + input_data_size_bytes) % AUDIO_RING_BUFFER_SIZE_BYTES;
    }
    ring_buffer_counter_values[right_data ? RING_BUFFER_RIGHT_BYTES_IN : RING_BUFFER_LEFT_BYTES_IN] += input_data_size_bytes;
}