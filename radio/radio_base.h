#ifndef RADIO_CONFIG_H_
#define RADIO_CONFIG_H_

#include <stdbool.h>
#include "stddef.h"
#include "rail_types.h"


#define DEBUG_TEST_MISSING_SEQUENCE_NUMBER 0 // forces a sequence number to be skipped

#define NUMBER_OF_PACKET_BUFFERS 20

uint8_t* radio__get_next_buffer(bool left_or_right_data);
void radio__process_event(RAIL_Handle_t rail_handle, RAIL_Events_t events);
void radio__init(void);
void radio__deinit(void);
uint32_t radio__get_next_sequence_number(void);
void radio__increment_sequence_number(void);
void radio__request_increment_channel(void);
uint32_t radio__get_channel(void);
bool radio__get_channel_changed_flag(void);
void radio__reset_channel_chanegd_flag(void);
void radio__run_process(void);
bool radio__is_radio_busy(void);
void radio__printf(bool add_timestamp, const char *format, ...);

#endif