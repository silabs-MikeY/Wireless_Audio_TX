#ifndef RADIO_PACKET_BUFFERS_H_
#define RADIO_PACKET_BUFFERS_H_

#include "radio_packet_format.h"

#include <stdbool.h>
#include <stdint.h>

extern packet_buffer_t radio_tx_packet_buffer[NUMBER_OF_PACKET_BUFFERS];
extern uint32_t packet_buffer_head;

bool radio_packet_buffers__init(void);
void radio_packet_buffers__get_oldest_packet_to_send(packet_buffer_t **packet_buffer, uint32_t *packet_buffer_index);
bool radio_packet_buffers__request_available_packet_buffer(packet_buffer_t **packet_buffer, uint32_t *packet_buffer_index);
bool radio_packet_buffers__mark_packet_buffer_used(uint32_t packet_buffer_index);
bool radio_packet_buffers__mark_packet_buffer_send_attempted(uint32_t packet_buffer_index);
bool radio_packet_buffers__mark_packet_buffer_completed(uint32_t packet_buffer_index);
bool radio_packet_buffers__mark_packet_buffer_failed(uint32_t packet_buffer_index);
const packet_buffer_t *radio_packet_buffers__get_packet_buffer(uint32_t packet_buffer_index);
void radio_packet_buffers__printf(bool add_timestamp, const char *format, ...);

#endif
