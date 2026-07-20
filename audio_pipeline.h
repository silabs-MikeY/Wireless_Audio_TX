#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <stdbool.h>
#include <stdint.h>


// Counters
uint32_t audio_pipeline__get_number_of_counters(void);
const char *audio_pipeline__get_counter_name(uint32_t counter_index);
volatile uint32_t *audio_pipeline__get_counter_address(uint32_t counter_index);
void audio_pipeline__reset_counters(void);
// Counters End

void audio_pipeline__init(bool is_stereo, bool enable_encoder);
bool audio_pipeline__run_process(void);
bool audio_pipeline__is_stereo(void);
bool audio_pipeline__is_encoder_enabled(void);


#endif // AUDIO_PIPELINE_H