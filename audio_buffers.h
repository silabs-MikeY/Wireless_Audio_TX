#include "hardware_config.h"
#include "stddef.h"
#include "scheduler.h"
#include "radio_transmit.h"
#include "scheduler.h"

#define NUMBER_OF_AUDIO_BUFFERS 20

typedef struct audio_buffer_s {
  uint8_t audio_buffer[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
  bool used;
  uint32_t micros_timestamp;
} audio_buffer_t;

void audio_buffers__add_new_data_to_left_buffer(uint8_t *data_pointer);
void audio_buffers__add_new_data_to_right_buffer(uint8_t *data_pointer);
void audio_buffers__init(void);
bool audio_buffers__run_process(void);

