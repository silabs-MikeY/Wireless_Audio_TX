#include "audio_pipeline.h"
#include "adc.h"
// #include "audio_buffers.h"
#include "audio_ring_buffer.h"
#include "microseconds.h"
#include "audio_encoding.h"
#include "adpcm.h"
#include "radio_packet_buffers.h"
#include <string.h>

// -----------------------------------------------------------------------------
//                     Exported Counters
// -----------------------------------------------------------------------------

typedef enum audio_buffers_counter_index_s {
  AUDIO_PIPELINE_LEFT_SAMPLES_PROCESSED = 0,
  AUDIO_PIPELINE_RIGHT_SAMPLES_PROCESSED = 1,
  AUDIO_PIPELINE_RADIO_PACKETS_BUILT = 2,
  AUDIO_PIPELINE_NUMBER_OF_COUNTERS
} audio_pipeline_counter_index_t;

static volatile uint32_t audio_pipeline_counter_values[AUDIO_PIPELINE_NUMBER_OF_COUNTERS] = {0};
static const char *audio_pipeline_counter_names[AUDIO_PIPELINE_NUMBER_OF_COUNTERS] = {
    "left_samples_processed",
    "right_samples_processed",
    "radio_packets_built"
};

uint32_t audio_pipeline__get_number_of_counters(void)
{
  return AUDIO_PIPELINE_NUMBER_OF_COUNTERS;
}
const char* audio_pipeline__get_counter_name(uint32_t counter_index)
{
  if (counter_index >= AUDIO_PIPELINE_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return audio_pipeline_counter_names[counter_index];
}
volatile uint32_t* audio_pipeline__get_counter_address(uint32_t counter_index)
{
  if (counter_index >= AUDIO_PIPELINE_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return &audio_pipeline_counter_values[counter_index];
}
void audio_pipeline__reset_counters(void)
{
  for (uint32_t i = 0; i < AUDIO_PIPELINE_NUMBER_OF_COUNTERS; i++)
  {
    audio_pipeline_counter_values[i] = 0;
  }
}

// -----------------------------------------------------------------------------
//                     Exported Counters End
// -----------------------------------------------------------------------------

static adpcm_t left_adpcm_state[1];
static adpcm_t right_adpcm_state[1];
static adpcm_context_t left_adpcm_ctx = {.ch_count = 1, .adpcm_list = left_adpcm_state};
static adpcm_context_t right_adpcm_ctx = {.ch_count = 1, .adpcm_list = right_adpcm_state};
static bool adpcm_initialized = false;

static bool stereo_flag = false;
static bool encoder_enabled = false;

static void audio_pipeline__process_new_adc_data(uint8_t *buffer,
                                               uint32_t buffer_size,
                                               bool right_channel);
static void audio_pipeline__check_for_new_adc_data_and_process(void);

void audio_pipeline__try_to_build_radio_packet_from_ring_buffer(void);

// -----------------------------------------------------------------------------
//                     Audio Pipeline General
// -----------------------------------------------------------------------------

void audio_pipeline__init(bool is_stereo, bool enable_encoder)
{
  stereo_flag = is_stereo;
  encoder_enabled = enable_encoder;
  adpcm_initialized = false;
}

bool audio_pipeline__is_stereo(void)
{
  return stereo_flag;
}

bool audio_pipeline__is_encoder_enabled(void)
{
  return encoder_enabled;
}

// This function is called from the main application loop to process audio data non-blocking
void audio_pipeline__run_process(void)
{
  audio_pipeline__try_to_build_radio_packet_from_ring_buffer();
  audio_pipeline__check_for_new_adc_data_and_process();
}

// -----------------------------------------------------------------------------
//                     Audio Pipeline General End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Building Radio Packets from Ring Buffer
// -----------------------------------------------------------------------------

void audio_pipeline__try_to_build_radio_packet_from_ring_buffer(void)
{
  packet_buffer_t *packet_buffer = NULL;
  uint32_t packet_buffer_index = 0xFFFFFFFF;

  if (radio_packet_buffers__request_available_packet_buffer(&packet_buffer, &packet_buffer_index) == false)
  {
    return;
  }

  if (ring_buffer__build_radio_packet_from_ring_buffer(stereo_flag,
                                                       encoder_enabled,
                                                       &packet_buffer->payload) == true)
  {
    if (radio_packet_buffers__mark_packet_buffer_used(packet_buffer_index) == false)
    {
      assert(0);
      return;
    }
    audio_pipeline_counter_values[AUDIO_PIPELINE_RADIO_PACKETS_BUILT]++;
  }
}

// -----------------------------------------------------------------------------
//                     Building Radio Packets from Ring Buffer End
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//                     Incoming ADC Data Processing
// -----------------------------------------------------------------------------

static void audio_pipeline__check_for_new_adc_data_and_process(void)
{
  uint8_t *left_buffer;
  uint8_t *right_buffer;
  uint32_t left_buffer_index;
  uint32_t right_buffer_index;
  uint32_t left_buffer_size_bytes;
  uint32_t right_buffer_size_bytes;

  if (adc__get_oldest_left_dma_buffer(&left_buffer, &left_buffer_index, &left_buffer_size_bytes) == true)
  {
    audio_pipeline__process_new_adc_data(left_buffer, left_buffer_size_bytes, false);
    adc__mark_left_dma_buffer_stale(left_buffer_index);
    audio_pipeline_counter_values[AUDIO_PIPELINE_LEFT_SAMPLES_PROCESSED] += left_buffer_size_bytes >> 2; // divide by sample size
  }

  if (adc__get_oldest_right_dma_buffer(&right_buffer, &right_buffer_index, &right_buffer_size_bytes) == true)
  {
    audio_pipeline__process_new_adc_data(right_buffer, right_buffer_size_bytes, true);
    adc__mark_right_dma_buffer_stale(right_buffer_index);
    audio_pipeline_counter_values[AUDIO_PIPELINE_RIGHT_SAMPLES_PROCESSED] += right_buffer_size_bytes >> 2; // divide by sample size
  }
}

static void audio_pipeline__process_new_adc_data(uint8_t *buffer,
                                               uint32_t buffer_size,
                                               bool right_channel)
{
  static int16_t temp_buffer[ADC_BUFFER_SIZE >> 1];
  static uint8_t adpcm_buffer[ADC_BUFFER_SIZE >> 2];
  uint32_t sample_size_bytes;
  uint32_t frame_count;

  if (buffer == NULL)
  {
    return;
  }

  if (encoder_enabled == true)
  {
    if (adpcm_initialized == false)
    {
      ADPCM_init(&left_adpcm_ctx);
      ADPCM_init(&right_adpcm_ctx);
      adpcm_initialized = true;
    }

    sample_size_bytes = adc__get_sample_size_bytes();
    if (sample_size_bytes == 0)
    {
      printf("Error: sample_size_bytes is 0\n");
      return;
    }

    frame_count = buffer_size / sample_size_bytes;

    if (audio_encoding__convert_raw_audio_to_pcm16(buffer, temp_buffer, buffer_size) == false)
    {
      return;
    }

    ADPCM_encode(right_channel ? &right_adpcm_ctx : &left_adpcm_ctx,
                 temp_buffer,
                 adpcm_buffer,
                 frame_count);

    ring_buffer__copy_data_into_ring_buffer(adpcm_buffer, right_channel, ADC_BUFFER_SIZE >> 2);
  }
  else
  {
    if (audio_encoding__convert_raw_audio_to_pcm16(buffer, temp_buffer, buffer_size) == false)
    {
      return;
    }

    ring_buffer__copy_data_into_ring_buffer((uint8_t *)temp_buffer, right_channel, ADC_BUFFER_SIZE >> 1);
  }
  return;
}

// -----------------------------------------------------------------------------
//                     Incoming ADC Data Processing End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Weak function implementations, do not rename.
// -----------------------------------------------------------------------------

// This implements the weak function defined in audio_buffers.c to process
// audio packets ready for transmission
bool audio_buffers__process_packet(uint8_t *left_or_first_buffer,
                                   uint8_t *right_or_second_buffer,
                                   bool is_stereo) {
  packet_buffer_t *packet_buffer = NULL;
  uint32_t packet_buffer_index = 0xFFFFFFFF;

  if (radio_packet_buffers__request_available_packet_buffer(&packet_buffer, &packet_buffer_index) == false)
  {
    return false;
  }

  memcpy(packet_buffer->payload.data_left, left_or_first_buffer, RADIO_PACKET_DATA_SIZE_PER_CHANNEL);
  memcpy(packet_buffer->payload.data_right, right_or_second_buffer, RADIO_PACKET_DATA_SIZE_PER_CHANNEL);

  if (is_stereo)
  {
    packet_buffer->payload.header.control_bits = CONTROL_BITS__STEREO;
  }

  return radio_packet_buffers__mark_packet_buffer_used(packet_buffer_index);
}

// Pushes time to the ADC to timestamp the audio data. This implements the weak function defined in adc.c
uint32_t adc__get_microsecond_ticks(void)
{
  return microseconds__get_micros_count();
}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------