#include "audio_processing.h"
#include "adc.h"
#include "adpcm.h"
#include "audio_buffers.h"
#include "audio_ring_buffer.h"
#include "radio/radio_transmit.h"

#include <string.h>

#define N_FRAMES 64
#define ADPCM_OUT_BYTES (N_FRAMES / 2)

static adpcm_t left_state[1];
static adpcm_t right_state[1];

static adpcm_context_t left_ctx = {.ch_count = 1, .adpcm_list = left_state};

static adpcm_context_t right_ctx = {.ch_count = 1, .adpcm_list = right_state};

static uint8_t left_adpcm[ADPCM_OUT_BYTES];
static uint8_t right_adpcm[ADPCM_OUT_BYTES];

static uint8_t audio_processing__get_adpcm_input_shift(void)
{
  if (adc__get_sample_size_bytes() == 4)
  {
    if (adc__is_sample_left_justified() == true)
    {
      return 16;
    }

    return 8;
  }

  return 0;
}

static int16_t audio_processing__pcm32_to_pcm16(int32_t sample)
{
  uint8_t input_shift = audio_processing__get_adpcm_input_shift();
  int32_t shifted_sample;
  int64_t scaled_sample;

  if (input_shift == 0)
  {
    shifted_sample = sample;
  }
  else if (input_shift >= 31)
  {
    shifted_sample = (sample < 0) ? -1 : 0;
  }
  else
  {
    int32_t round = (int32_t)1 << (input_shift - 1);

    if (sample >= 0)
    {
      shifted_sample = (sample + round) >> input_shift;
    }
    else
    {
      shifted_sample = -(((-sample) + round) >> input_shift);
    }
  }

  scaled_sample = (int64_t)shifted_sample * 32768;
  if (scaled_sample >= 0)
  {
    scaled_sample += 16384;
  }
  else
  {
    scaled_sample -= 16384;
  }

  scaled_sample >>= 15;

  if (scaled_sample > 32767)
  {
    return 32767;
  }

  if (scaled_sample < -32768)
  {
    return -32768;
  }

  return (int16_t)scaled_sample;
}

static bool audio_processing__convert_raw_audio_to_pcm16(const uint8_t *input,
                                                         int16_t *output,
                                                         uint32_t input_size)
{
  if ((input_size & 0x3) != 0)
  {
    return false;
  }

  const uint32_t sample_size_bytes = adc__get_sample_size_bytes();

  if (sample_size_bytes != 4)
  {
    return false;
  }

  uint32_t output_index = 0;
  for (uint32_t input_index = 0; input_index < input_size; input_index += sample_size_bytes)
  {
    int32_t raw_sample;

    memcpy(&raw_sample, &input[input_index], sizeof(raw_sample));
    output[output_index] = audio_processing__pcm32_to_pcm16(raw_sample);
    output_index++;
  }

  return true;
}

void audio_processing__init(void) {
  ADPCM_init(&left_ctx);
  ADPCM_init(&right_ctx);
}

void audio_processing__process_new_audio_data(uint8_t *buffer,
                                              uint32_t buffer_size,
                                              bool is_right_channel) {
  (void)buffer;
  (void)buffer_size;
  (void)is_right_channel;
}

static void audio_processing__process_left_audio_data(uint8_t *buffer)
{
  int16_t pcm16_buffer[RADIO_PACKET_DATA_SIZE_PER_CHANNEL / sizeof(int16_t)];

  if (audio_processing__convert_raw_audio_to_pcm16(buffer,
                                                   pcm16_buffer,
                                                   RADIO_PACKET_DATA_SIZE) == true)
  {
    audio_buffers__add_new_data_to_left_buffer((uint8_t *)pcm16_buffer);
  }

  copy_data_into_buffer(buffer, false, RADIO_PACKET_DATA_SIZE);
}

static void audio_processing__process_right_audio_data(uint8_t *buffer)
{
  int16_t pcm16_buffer[RADIO_PACKET_DATA_SIZE_PER_CHANNEL / sizeof(int16_t)];

  if (audio_processing__convert_raw_audio_to_pcm16(buffer,
                                                   pcm16_buffer,
                                                   RADIO_PACKET_DATA_SIZE) == true)
  {
    audio_buffers__add_new_data_to_right_buffer((uint8_t *)pcm16_buffer);
  }

  copy_data_into_buffer(buffer, true, RADIO_PACKET_DATA_SIZE);
}

void process_new_left_audio_data(uint8_t *buffer) {
  audio_processing__process_left_audio_data(buffer);
}

void process_new_right_audio_data(uint8_t *buffer) {
  audio_processing__process_right_audio_data(buffer);
}

// Strong implementations of weak functions. Do not rename these

// This implements the weak functions defined in ADC.c to process incoming audio
// data Passes the audio data to the audio_buffers module for buffering and
// processing
void adc__process_left_buffer(uint8_t *buffer) {
  process_new_left_audio_data(buffer);
}

// This implements the weak functions defined in ADC.c to process incoming audio
// data Passes the audio data to the audio_buffers module for buffering and
// processing
void adc__process_right_buffer(uint8_t *buffer) {
  process_new_right_audio_data(buffer);
}

// This implemented the weak function defined in audio_buffers.c to process
// audio packets ready for transmission
bool audio_buffers__process_packet(uint8_t *left_or_first_buffer,
                                   uint8_t *right_or_second_buffer,
                                   bool is_stereo) {
  return radio_transmit__create_new_packet_buffer(
      left_or_first_buffer, right_or_second_buffer, is_stereo);
}

/*

static void audio_processing__print_adpcm_diff(const char *channel_name,
                                              const adpcm_context_t *encode_ctx,
                                              const uint8_t *input_buffer,
                                              const uint8_t *encoded_buffer)
{
  adpcm_t decode_state[1];
  adpcm_context_t decode_ctx = {.ch_count = encode_ctx->ch_count,
                                .adpcm_list = decode_state};
  int16_t expected_samples[N_FRAMES] = {0};
  int16_t decoded_samples[N_FRAMES] = {0};

  decode_state[0] = encode_ctx->adpcm_list[0];
  decode_state[0].comprStateCurrent = encode_ctx->adpcm_list[0].comprStateBegin;

  if (audio_processing__convert_raw_audio_to_pcm16(input_buffer,
                                                   expected_samples,
                                                   RADIO_PACKET_DATA_SIZE) == false) {
    adc__printf(false, "%s ADPCM compare skipped: expected conversion failed\n",
                channel_name);
    return;
  }

  ADPCM_decode(&decode_ctx, encoded_buffer, decoded_samples, N_FRAMES);

  uint32_t mismatch_count = 0;
  int32_t max_abs_diff = 0;

  adc__printf(false, "%s ADPCM round-trip diff:\n", channel_name);
  for (uint32_t i = 0; i < N_FRAMES; i++) {
    int32_t diff = (int32_t)decoded_samples[i] - (int32_t)expected_samples[i];
    int32_t abs_diff = (diff < 0) ? -diff : diff;

    if (abs_diff > max_abs_diff) {
      max_abs_diff = abs_diff;
    }

    if (abs_diff != 0) {
      mismatch_count++;
      adc__printf(false,
                  "  idx %u expected %d decoded %d diff %ld\n",
                  (unsigned int)i,
                  (int)expected_samples[i],
                  (int)decoded_samples[i],
                  (long)diff);
    }
  }

  adc__printf(false,
              "  mismatches: %u  max_abs_diff: %ld\n",
              (unsigned int)mismatch_count,
              (long)max_abs_diff);
}

void audio_processing__test_adpcm_left(uint8_t *buffer)
{
  int16_t pcm16_buffer[RADIO_PACKET_DATA_SIZE_PER_CHANNEL / sizeof(int16_t)];

  if (audio_processing__convert_raw_audio_to_pcm16(buffer,
                                                   pcm16_buffer,
                                                   RADIO_PACKET_DATA_SIZE) == true) {
    audio_buffers__add_new_data_to_left_buffer((uint8_t *)pcm16_buffer);
  }
  copy_data_into_buffer(buffer, false, RADIO_PACKET_DATA_SIZE);

  ADPCM_encode_s32(&left_ctx, (int32_t *)buffer, left_adpcm, N_FRAMES,
                   audio_processing__get_adpcm_input_shift(), 32768);

  audio_processing__print_adpcm_diff("LEFT", &left_ctx, buffer, left_adpcm);
}

void audio_processing__test_adpcm_right(uint8_t *buffer)
{
  int16_t pcm16_buffer[RADIO_PACKET_DATA_SIZE_PER_CHANNEL / sizeof(int16_t)];

  if (audio_processing__convert_raw_audio_to_pcm16(buffer,
                                                   pcm16_buffer,
                                      RADIO_PACKET_DATA_SIZE) == true) {
    audio_buffers__add_new_data_to_right_buffer((uint8_t *)pcm16_buffer);
  }
  copy_data_into_buffer(buffer, true, RADIO_PACKET_DATA_SIZE);

  ADPCM_encode_s32(&right_ctx, (int32_t *)buffer, right_adpcm, N_FRAMES,
                   audio_processing__get_adpcm_input_shift(), 32768);

  audio_processing__print_adpcm_diff("RIGHT", &right_ctx, buffer, right_adpcm);
}
  */