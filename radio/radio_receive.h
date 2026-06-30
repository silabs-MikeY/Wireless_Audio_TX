#ifndef RADIO_RECEIVE_H_
#define RADIO_RECEIVE_H_

#include "rail_types.h"
#include "stddef.h"
#include <stdbool.h>

void radio_receive__init(void);
bool radio__process_event_rx(RAIL_Handle_t rail_handle, RAIL_Events_t events);
#endif