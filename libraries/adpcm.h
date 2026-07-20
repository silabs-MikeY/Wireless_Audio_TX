#ifndef ADPCM_H_
#define ADPCM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int8_t step;              /* Index into adpcmStepTable */
  int16_t predictedSample;  /* Last predicted sample */
} adpcm_state_t;

typedef struct {
  adpcm_state_t comprStateBegin;
  adpcm_state_t comprStateCurrent;
} adpcm_t;

typedef struct {
  uint8_t ch_count;
  adpcm_t *adpcm_list;
} adpcm_context_t;

/*
 * Initialize ADPCM state.
 */
void ADPCM_init(adpcm_context_t *ctx);

/*
 * Original Silicon Labs-compatible encoder.
 *
 * Input:
 *   int16_t PCM, interleaved by channel.
 *
 * Output:
 *   4-bit IMA ADPCM nibbles, two samples per byte.
 *
 * Requirement:
 *   n_frames should be even.
 */
void ADPCM_encode(adpcm_context_t *ctx,
                  const int16_t *in,
                  uint8_t *out,
                  uint32_t n_frames);

/*
 * New 32-bit input encoder.
 *
 * This is still standard 16-bit IMA ADPCM internally. It converts each int32_t
 * input sample to int16_t using rounding, saturation, and optional gain.
 *
 * Input layout:
 *   Mono:
 *     s0, s1, s2...
 *
 *   Stereo:
 *     L0, R0, L1, R1...
 *
 * input_shift:
 *   Number of bits to right-shift before encoding.
 *
 *   For signed 24-bit right-justified data in int32_t:
 *     input_shift = 8
 *
 *   For signed 24-bit left-justified data in int32_t:
 *     input_shift = 16
 *
 * gain_q15:
 *   Linear gain in Q15 format.
 *
 *   32768 = 1.0x
 *   16384 = 0.5x
 *   65536 = 2.0x
 *
 *   Use 32768 for no gain.
 *
 * Output:
 *   Same as ADPCM_encode():
 *   output_bytes = ch_count * n_frames / 2
 *
 * Requirement:
 *   n_frames should be even.
 */
void ADPCM_encode_s32(adpcm_context_t *ctx,
                      const int32_t *in,
                      uint8_t *out,
                      uint32_t n_frames,
                      uint8_t input_shift,
                      int32_t gain_q15);

void ADPCM_decode(adpcm_context_t *ctx,
                  const uint8_t *in,
                  int16_t *out,
                  uint32_t n_frames);

#ifdef __cplusplus
}
#endif

#endif /* ADPCM_H_ */