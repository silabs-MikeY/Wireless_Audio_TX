#ifndef RADIO_TRANSMIT_H_
#define RADIO_TRANSMIT_H_

#include "hardware_config.h"
#include "rail_types.h"
#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

bool radio_transmit__init(void);
bool radio_transmit__run_process(void);
bool radio__send_packet_by_sequence_number(uint16_t sequence_number, bool retry);
bool radio__process_event_tx(RAIL_Handle_t rail_handle, RAIL_Events_t events);
void radio_transmit__handle_successful_packet_sent(uint32_t packet_buffer_index);

uint32_t radio_transmit__get_number_of_counters(void);
const char *radio_transmit__get_counter_name(uint32_t counter_index);
volatile uint32_t *radio_transmit__get_counter_address(uint32_t counter_index);
void radio_transmit__reset_counters(void);

#endif
