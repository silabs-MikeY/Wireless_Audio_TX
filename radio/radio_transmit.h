#ifndef RADIO_TRANSMIT_H_
#define RADIO_TRANSMIT_H_

#include "hardware_config.h"
#include "radio_base.h"
#include "debug.h"

typedef struct header_s {
  uint8_t size;
  uint8_t control_bits;
  uint16_t sequence_number;
} __attribute__((aligned(4))) header_t;

typedef struct payload_s {
  header_t header;
  uint8_t data_left[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
  uint8_t data_right[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
} __attribute__((aligned(4)))payload_t;

typedef struct packet_buffer_s {
  payload_t payload;
  bool used;
  bool waiting_to_be_sent;
  bool sent;
  uint32_t micros_timestamp;
} __attribute__((aligned(4)))packet_buffer_t;

bool radio_transmit__create_new_packet_buffer(uint8_t* left_or_first_data, uint8_t* right_or_second_data, bool is_stereo);
void radio_transmit__init(void);
bool radio_transmit__run_process(void);

#endif