#include "audio_encoding.h"

#include "adc.h"
#include "adpcm.h"
// #include "audio_buffers.h"
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

static uint8_t audio_encoding__get_adpcm_input_shift(void)
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

bool audio_encoding__convert_raw_audio_to_pcm16(const uint8_t *input,
                                               int16_t *output,
                                               uint32_t input_size)
{
  if ((input == NULL) || (output == NULL))
  {
    return false;
  }

  const uint32_t sample_size_bytes = adc__get_sample_size_bytes();

  if ((sample_size_bytes != 2) && (sample_size_bytes != 4))
  {
    return false;
  }

  if ((input_size % sample_size_bytes) != 0)
  {
    return false;
  }

  if (sample_size_bytes == 2)
  {
    memcpy(output, input, input_size);
    return true;
  }

  uint8_t *output_bytes = (uint8_t *)output;
  uint32_t output_index = 0;

  for (uint32_t input_index = 0; input_index < input_size; input_index += sample_size_bytes)
  {
    output_bytes[output_index + 0] = input[input_index + 1];
    output_bytes[output_index + 1] = input[input_index + 0];
    output_index += 2;
  }

  return true;
}

static void audio_encoding__print_adpcm_diff(const char *channel_name,
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

  if (audio_encoding__convert_raw_audio_to_pcm16(input_buffer,
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

bool audio_encoding__init(void)
{
  ADPCM_init(&left_ctx);
  ADPCM_init(&right_ctx);
  return true;
}

void audio_encoding__test_adpcm_left(uint8_t *buffer)
{
  ring_buffer__copy_data_into_ring_buffer(buffer, false, RADIO_PACKET_DATA_SIZE);

  ADPCM_encode_s32(&left_ctx, (int32_t *)buffer, left_adpcm, N_FRAMES,
                   audio_encoding__get_adpcm_input_shift(), 32768);

  audio_encoding__print_adpcm_diff("LEFT", &left_ctx, buffer, left_adpcm);
}

void audio_encoding__test_adpcm_right(uint8_t *buffer)
{
  ring_buffer__copy_data_into_ring_buffer(buffer, true, RADIO_PACKET_DATA_SIZE);

  ADPCM_encode_s32(&right_ctx, (int32_t *)buffer, right_adpcm, N_FRAMES,
                   audio_encoding__get_adpcm_input_shift(), 32768);

  audio_encoding__print_adpcm_diff("RIGHT", &right_ctx, buffer, right_adpcm);
}
