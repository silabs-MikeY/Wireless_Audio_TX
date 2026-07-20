
#ifndef RADIO_PACKET_FORMAT_H
#define RADIO_PACKET_FORMAT_H

#include "hardware_config.h"
#include "radio/radio_base.h"
#include "rail_types.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct header_s {
	uint8_t size;
	uint8_t control_bits;
	uint16_t sequence_number;
} __attribute__((aligned(4))) header_t;

typedef struct payload_s {
	header_t header;
	uint8_t data_left[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
	uint8_t data_right[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
} __attribute__((aligned(4))) payload_t;

typedef struct packet_buffer_s {
	payload_t payload;
	bool used;
	bool waiting_to_be_sent;
	bool send_attempted;
	uint32_t micros_timestamp_added_to_buffer;
	uint32_t micros_timestamp_packet_sent;
	uint32_t micros_timestamp_retry_after;
	bool tx_processed;
} __attribute__((aligned(4))) packet_buffer_t;

#define RADIO_DATA_BYTES_PER_CHANNEL RADIO_PACKET_DATA_SIZE_PER_CHANNEL
#define RADIO_DATA_BYTES_TOTAL (RADIO_DATA_BYTES_PER_CHANNEL*2)

#define CONTROL_BITS__STEREO (1u << 0)
#define CONTROL_BITS__RETRY (1u << 1)
#define CONTROL_BITS__COMMAND_PACKET (1u << 2)
#define CONTROL_BITS__ENCODED (1u << 3)
#define CONTROL_BITS__BIT4_UNUSED (1u << 4)
#define CONTROL_BITS__BIT5_UNUSED (1u << 5)
#define CONTROL_BITS__BIT6_UNUSED (1u << 6)
#define CONTROL_BITS__BIT7_UNUSED (1u << 7)

#endif // RADIO_PACKET_FORMAT_H

