#include <stdint.h>
#include <string.h>

#include "adpcm.h"

static const int8_t adpcmIndexAdjust[16] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
};

static const int16_t adpcmStepTable[89] = {
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
  19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
  50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
  130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
  337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
  876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
  2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
  5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static uint8_t imaEncodeSample(int16_t sample, adpcm_state_t *adpcmState);
static int16_t imaDecodeSample(uint8_t compressedCode, adpcm_state_t *adpcmState);

static int16_t saturate_i16(int32_t x)
{
  if (x > 32767) {
    return 32767;
  }

  if (x < -32768) {
    return -32768;
  }

  return (int16_t)x;
}

/*
 * Rounded arithmetic right shift for signed int32_t values.
 *
 * shift = 0 returns x unchanged.
 * shift > 0 divides by 2^shift with rounding to nearest.
 */
static int32_t rounded_shift_right_i32(int32_t x, uint8_t shift)
{
  if (shift == 0) {
    return x;
  }

  if (shift >= 31) {
    return (x < 0) ? -1 : 0;
  }

  const int32_t round = (int32_t)1 << (shift - 1);

  if (x >= 0) {
    return (x + round) >> shift;
  } else {
    return -(((-x) + round) >> shift);
  }
}

/*
 * Convert signed 32-bit container sample to signed 16-bit PCM.
 *
 * input_shift selects the source alignment:
 *
 *   24-bit right-justified:
 *     input_shift = 8
 *
 *   24-bit left-justified:
 *     input_shift = 16
 *
 * gain_q15 is applied after the alignment shift:
 *
 *   32768 = 1.0x
 *   65536 = 2.0x
 *   16384 = 0.5x
 */
static int16_t pcm32_to_pcm16(int32_t sample,
                              uint8_t input_shift,
                              int32_t gain_q15)
{
  int32_t x = rounded_shift_right_i32(sample, input_shift);

  /*
   * Apply Q15 gain with rounding.
   *
   * x = round((x * gain_q15) / 32768)
   */
  int64_t y = (int64_t)x * (int64_t)gain_q15;

  if (y >= 0) {
    y += 16384;
  } else {
    y -= 16384;
  }

  y >>= 15;

  if (y > 32767) {
    return 32767;
  }

  if (y < -32768) {
    return -32768;
  }

  return (int16_t)y;
}

void ADPCM_init(adpcm_context_t *ctx)
{
  for (uint8_t ch = 0; ch < ctx->ch_count; ch++) {
    adpcm_t *adpcm = &ctx->adpcm_list[ch];

    adpcm->comprStateBegin.predictedSample = 0;
    adpcm->comprStateBegin.step = 0;

    adpcm->comprStateCurrent.predictedSample = 0;
    adpcm->comprStateCurrent.step = 0;
  }
}

void ADPCM_encode(adpcm_context_t *ctx,
                  const int16_t *in,
                  uint8_t *out,
                  uint32_t n_frames)
{
  for (uint8_t ch = 0; ch < ctx->ch_count; ch++) {
    adpcm_t *adpcm = &ctx->adpcm_list[ch];

    uint8_t compressedNibble = 0;
    uint8_t compressedNibbleCount = 0;
    uint32_t idx_in;
    uint32_t idx_out = ch;

    memcpy(&adpcm->comprStateBegin,
           &adpcm->comprStateCurrent,
           sizeof(adpcm_state_t));

    for (uint32_t i = 0; i < n_frames; i++) {
      idx_in = (i * ctx->ch_count) + ch;

      compressedNibble <<= 4;
      compressedNibble |= imaEncodeSample(in[idx_in],
                                           &adpcm->comprStateCurrent);
      compressedNibbleCount++;

      if (compressedNibbleCount < 2) {
        continue;
      }

      compressedNibbleCount = 0;

      out[idx_out] = compressedNibble;
      idx_out += ctx->ch_count;
    }
  }
}

void ADPCM_encode_s32(adpcm_context_t *ctx,
                      const int32_t *in,
                      uint8_t *out,
                      uint32_t n_frames,
                      uint8_t input_shift,
                      int32_t gain_q15)
{
  if (gain_q15 == 0) {
    gain_q15 = 32768;
  }

  for (uint8_t ch = 0; ch < ctx->ch_count; ch++) {
    adpcm_t *adpcm = &ctx->adpcm_list[ch];

    uint8_t compressedNibble = 0;
    uint8_t compressedNibbleCount = 0;
    uint32_t idx_in;
    uint32_t idx_out = ch;

    memcpy(&adpcm->comprStateBegin,
           &adpcm->comprStateCurrent,
           sizeof(adpcm_state_t));

    for (uint32_t i = 0; i < n_frames; i++) {
      int16_t sample16;

      idx_in = (i * ctx->ch_count) + ch;

      sample16 = pcm32_to_pcm16(in[idx_in], input_shift, gain_q15);

      compressedNibble <<= 4;
      compressedNibble |= imaEncodeSample(sample16,
                                           &adpcm->comprStateCurrent);
      compressedNibbleCount++;

      if (compressedNibbleCount < 2) {
        continue;
      }

      compressedNibbleCount = 0;

      out[idx_out] = compressedNibble;
      idx_out += ctx->ch_count;
    }
  }
}

void ADPCM_decode(adpcm_context_t *ctx,
                  const uint8_t *in,
                  int16_t *out,
                  uint32_t n_frames)
{
  for (uint8_t ch = 0; ch < ctx->ch_count; ch++) {
    adpcm_t *adpcm = &ctx->adpcm_list[ch];

    uint32_t idx_out = ch;

    memcpy(&adpcm->comprStateBegin,
           &adpcm->comprStateCurrent,
           sizeof(adpcm_state_t));

    for (uint32_t i = 0; i < n_frames; i++) {
      uint32_t idx_in = ((i >> 1) * ctx->ch_count) + ch;
      uint8_t compressedByte = in[idx_in];
      uint8_t compressedCode = (i & 0x1) ? (compressedByte & 0x0F)
                                         : (compressedByte >> 4);

      out[idx_out] = imaDecodeSample(compressedCode,
                                     &adpcm->comprStateCurrent);
      idx_out += ctx->ch_count;
    }
  }
}

static uint8_t imaEncodeSample(int16_t sample, adpcm_state_t *adpcmState)
{
  uint8_t compressedCode = 0;

  int32_t currentStep = (int32_t)adpcmStepTable[adpcmState->step];
  int32_t predictedSample = (int32_t)adpcmState->predictedSample;
  int32_t diff;
  int32_t predictedDiff;

  diff = (int32_t)sample - predictedSample;

  if (diff < 0) {
    compressedCode = 8;
    diff = -diff;
  }

  predictedDiff = currentStep >> 3;

  if (diff >= currentStep) {
    compressedCode |= 4;
    diff -= currentStep;
    predictedDiff += currentStep;
  }

  currentStep >>= 1;

  if (diff >= currentStep) {
    compressedCode |= 2;
    diff -= currentStep;
    predictedDiff += currentStep;
  }

  currentStep >>= 1;

  if (diff >= currentStep) {
    compressedCode |= 1;
    predictedDiff += currentStep;
  }

  if (compressedCode & 8) {
    predictedSample -= predictedDiff;
  } else {
    predictedSample += predictedDiff;
  }

  adpcmState->predictedSample = saturate_i16(predictedSample);

  adpcmState->step += adpcmIndexAdjust[compressedCode];

  if (adpcmState->step < 0) {
    adpcmState->step = 0;
  } else if (adpcmState->step > 88) {
    adpcmState->step = 88;
  }

  return compressedCode;
}

static int16_t imaDecodeSample(uint8_t compressedCode, adpcm_state_t *adpcmState)
{
  int32_t currentStep = (int32_t)adpcmStepTable[adpcmState->step];
  int32_t predictedSample = (int32_t)adpcmState->predictedSample;
  int32_t predictedDiff = currentStep >> 3;

  if (compressedCode & 4) {
    predictedDiff += currentStep;
  }

  if (compressedCode & 2) {
    predictedDiff += (currentStep >> 1);
  }

  if (compressedCode & 1) {
    predictedDiff += (currentStep >> 2);
  }

  if (compressedCode & 8) {
    predictedSample -= predictedDiff;
  } else {
    predictedSample += predictedDiff;
  }

  adpcmState->predictedSample = saturate_i16(predictedSample);

  adpcmState->step += adpcmIndexAdjust[compressedCode & 0x0F];

  if (adpcmState->step < 0) {
    adpcmState->step = 0;
  } else if (adpcmState->step > 88) {
    adpcmState->step = 88;
  }

  return adpcmState->predictedSample;
}