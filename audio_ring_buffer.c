#include "audio_ring_buffer.h"

#include "radio_packet_format.h"
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

static volatile uint32_t
    ring_buffer_counter_values[RING_BUFFER_NUMBER_OF_COUNTERS] = {0};
static const char *ring_buffer_counter_names[RING_BUFFER_NUMBER_OF_COUNTERS] = {
    "ring_buffer_left_bytes_in", "ring_buffer_right_bytes_in",
    "ring_buffer_left_bytes_out", "ring_buffer_right_bytes_out"};

uint32_t ring_buffer__get_number_of_counters(void) {
  return RING_BUFFER_NUMBER_OF_COUNTERS;
}
const char *ring_buffer__get_counter_name(uint32_t counter_index) {
  if (counter_index >= RING_BUFFER_NUMBER_OF_COUNTERS) {
    return NULL;
  }

  return ring_buffer_counter_names[counter_index];
}
volatile uint32_t *ring_buffer__get_counter_address(uint32_t counter_index) {
  if (counter_index >= RING_BUFFER_NUMBER_OF_COUNTERS) {
    return NULL;
  }

  return &ring_buffer_counter_values[counter_index];
}
void ring_buffer__reset_counters(void) {
  for (uint32_t i = 0; i < RING_BUFFER_NUMBER_OF_COUNTERS; i++) {
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

void ring_buffer__init(bool is_stereo, bool enable_encoder) {
  stereo_flag = is_stereo;
  encoder_enabled = enable_encoder;
  memset(left_audio_ring_buffer, 0, AUDIO_RING_BUFFER_SIZE_BYTES);
  memset(right_audio_ring_buffer, 0, AUDIO_RING_BUFFER_SIZE_BYTES);
  left_audio_ring_buffer_head = 0;
  right_audio_ring_buffer_head = 0;
  left_audio_ring_buffer_tail = 0;
  right_audio_ring_buffer_tail = 0;
}

bool ring_buffer__check_if_enough_data_to_build_radio_packet(bool is_stereo) {
  if (is_stereo) {
    if (left_audio_ring_buffer_tail < left_audio_ring_buffer_head) 
    {
      if ((left_audio_ring_buffer_head - left_audio_ring_buffer_tail) < RADIO_DATA_BYTES_PER_CHANNEL) 
      {
        return false;
      }
    }
    else 
    {
      if ((AUDIO_RING_BUFFER_SIZE_BYTES - left_audio_ring_buffer_tail + left_audio_ring_buffer_head) < RADIO_DATA_BYTES_PER_CHANNEL) 
      {
        return false;
      }
    }
    if (right_audio_ring_buffer_tail < right_audio_ring_buffer_head) 
    {
      if ((right_audio_ring_buffer_head - right_audio_ring_buffer_tail) < RADIO_DATA_BYTES_PER_CHANNEL) 
      {
        return false;
      }
    }
    else 
    {
      if ((AUDIO_RING_BUFFER_SIZE_BYTES - right_audio_ring_buffer_tail + right_audio_ring_buffer_head) < RADIO_DATA_BYTES_PER_CHANNEL) 
      {
        return false;
      }
    }
  }
  else 
  {
    if (right_audio_ring_buffer_tail < right_audio_ring_buffer_head) 
    {
      if ((right_audio_ring_buffer_head - right_audio_ring_buffer_tail) < RADIO_DATA_BYTES_TOTAL) 
      {
        return false;
      }
    }
    else 
    {
      if ((AUDIO_RING_BUFFER_SIZE_BYTES - right_audio_ring_buffer_tail + right_audio_ring_buffer_head) < RADIO_DATA_BYTES_TOTAL) 
      {
        return false;
      }
    }
  }
  return true;
}

bool ring_buffer__build_radio_packet_from_ring_buffer(bool is_stereo, bool enable_encoder, payload_t *radio_packet_payload) {
  // This function should implement the logic to build a radio packet from the
  // ring buffer. The implementation details will depend on the specific
  // requirements of the radio packet format and how the data is structured in
  // the ring buffer.

  if (radio_packet_payload == NULL) {
    return false; // Error: Null pointer for radio packet payload
  }

  if (!ring_buffer__check_if_enough_data_to_build_radio_packet(is_stereo)) {
    return false; // Not enough data to build a packet
  }

  radio_packet_payload->header.control_bits = 0;
  if (is_stereo)
  {
    radio_packet_payload->header.control_bits |= CONTROL_BITS__STEREO;
  }
  if (enable_encoder)
  {
    radio_packet_payload->header.control_bits |= CONTROL_BITS__ENCODED;
  }

  if (is_stereo)
  {
    // Copy left channel data
    if (left_audio_ring_buffer_head < left_audio_ring_buffer_tail) 
    {
      uint32_t bytes_available_to_end = AUDIO_RING_BUFFER_SIZE_BYTES - left_audio_ring_buffer_tail;
      uint32_t first_chunk_size = (bytes_available_to_end < RADIO_DATA_BYTES_PER_CHANNEL)
                                  ? bytes_available_to_end
                                  : RADIO_DATA_BYTES_PER_CHANNEL;
      uint32_t second_chunk_size = RADIO_DATA_BYTES_PER_CHANNEL - first_chunk_size;
      
      memcpy(radio_packet_payload->data_left, &left_audio_ring_buffer[left_audio_ring_buffer_tail], first_chunk_size);
      if (second_chunk_size > 0)
      {
        memcpy(&radio_packet_payload->data_left[first_chunk_size], left_audio_ring_buffer, second_chunk_size);
      }
      left_audio_ring_buffer_tail = second_chunk_size;
    }
    else 
    {
      memcpy(radio_packet_payload->data_left, &left_audio_ring_buffer[left_audio_ring_buffer_tail], RADIO_DATA_BYTES_PER_CHANNEL);
      left_audio_ring_buffer_tail = (left_audio_ring_buffer_tail + RADIO_DATA_BYTES_PER_CHANNEL) % AUDIO_RING_BUFFER_SIZE_BYTES;
    }
    // Copy right channel data
    if (right_audio_ring_buffer_head < right_audio_ring_buffer_tail) 
    {
      uint32_t bytes_available_to_end = AUDIO_RING_BUFFER_SIZE_BYTES - right_audio_ring_buffer_tail;
      uint32_t first_chunk_size = (bytes_available_to_end < RADIO_DATA_BYTES_PER_CHANNEL)
                                  ? bytes_available_to_end
                                  : RADIO_DATA_BYTES_PER_CHANNEL;
      uint32_t second_chunk_size = RADIO_DATA_BYTES_PER_CHANNEL - first_chunk_size;

      memcpy(radio_packet_payload->data_right, &right_audio_ring_buffer[right_audio_ring_buffer_tail], first_chunk_size);
      if (second_chunk_size > 0)
      {
        memcpy(&radio_packet_payload->data_right[first_chunk_size], right_audio_ring_buffer, second_chunk_size);
      }
      right_audio_ring_buffer_tail = second_chunk_size;
    }
    else 
    {
      memcpy(radio_packet_payload->data_right, &right_audio_ring_buffer[right_audio_ring_buffer_tail], RADIO_DATA_BYTES_PER_CHANNEL);
      right_audio_ring_buffer_tail = (right_audio_ring_buffer_tail + RADIO_DATA_BYTES_PER_CHANNEL) % AUDIO_RING_BUFFER_SIZE_BYTES;
    }
  }
  else
  {
    // Copy mono data (right channel)
    if (right_audio_ring_buffer_head < right_audio_ring_buffer_tail) 
    {
      uint32_t bytes_available_to_end = AUDIO_RING_BUFFER_SIZE_BYTES - right_audio_ring_buffer_tail;
      uint32_t first_chunk_size = (bytes_available_to_end < RADIO_DATA_BYTES_TOTAL)
                                  ? bytes_available_to_end
                                  : RADIO_DATA_BYTES_TOTAL;
      uint32_t second_chunk_size = RADIO_DATA_BYTES_TOTAL - first_chunk_size;

      memcpy(radio_packet_payload->data_left, &right_audio_ring_buffer[right_audio_ring_buffer_tail], first_chunk_size);
      if (second_chunk_size > 0)
      {
        memcpy(&radio_packet_payload->data_left[first_chunk_size], right_audio_ring_buffer, second_chunk_size);
      }
      right_audio_ring_buffer_tail = second_chunk_size;
    }
    else 
    {
      memcpy(radio_packet_payload->data_left, &right_audio_ring_buffer[right_audio_ring_buffer_tail], RADIO_DATA_BYTES_TOTAL);
      right_audio_ring_buffer_tail = (right_audio_ring_buffer_tail + RADIO_DATA_BYTES_TOTAL) % AUDIO_RING_BUFFER_SIZE_BYTES;
    }
  }

  return true;
}

void ring_buffer__copy_data_into_ring_buffer(uint8_t *input_data,
                                             bool right_data,
                                             uint32_t input_data_size_bytes) {
  uint8_t *audio_ring_buffer =
      (right_data == false) ? left_audio_ring_buffer : right_audio_ring_buffer;
  uint32_t *audio_ring_buffer_head = (right_data == false)
                                         ? &left_audio_ring_buffer_head
                                         : &right_audio_ring_buffer_head;
  uint32_t *audio_ring_buffer_tail = (right_data == false)
                                         ? &left_audio_ring_buffer_tail
                                         : &right_audio_ring_buffer_tail;

  if (input_data_size_bytes == 0) {
    return;
  }

  if (input_data_size_bytes > AUDIO_RING_BUFFER_SIZE_BYTES) {
    input_data_size_bytes = AUDIO_RING_BUFFER_SIZE_BYTES;
  }

  uint32_t first_chunk_size_bytes =
      AUDIO_RING_BUFFER_SIZE_BYTES - *audio_ring_buffer_head;
  if (first_chunk_size_bytes > input_data_size_bytes) {
    first_chunk_size_bytes = input_data_size_bytes;
  }

  memcpy(&audio_ring_buffer[*audio_ring_buffer_head], input_data,
         first_chunk_size_bytes);
  *audio_ring_buffer_head += first_chunk_size_bytes;

  if (*audio_ring_buffer_head >= AUDIO_RING_BUFFER_SIZE_BYTES) {
    *audio_ring_buffer_head = 0;
  }

  if (first_chunk_size_bytes < input_data_size_bytes) {
    uint32_t remaining_size_bytes =
        input_data_size_bytes - first_chunk_size_bytes;
    memcpy(&audio_ring_buffer[*audio_ring_buffer_head],
           &input_data[first_chunk_size_bytes], remaining_size_bytes);
    *audio_ring_buffer_head += remaining_size_bytes;

    if (*audio_ring_buffer_head >= AUDIO_RING_BUFFER_SIZE_BYTES) {
      *audio_ring_buffer_head = 0;
    }
  }
  if (*audio_ring_buffer_head == *audio_ring_buffer_tail) {
    // Buffer overflow, move tail forward to make space
    *audio_ring_buffer_tail =
        (*audio_ring_buffer_tail + input_data_size_bytes) %
        AUDIO_RING_BUFFER_SIZE_BYTES;
  }
  ring_buffer_counter_values[right_data ? RING_BUFFER_RIGHT_BYTES_IN
                                        : RING_BUFFER_LEFT_BYTES_IN] +=
      input_data_size_bytes;
}