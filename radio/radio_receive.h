#ifndef RADIO_RECEIVE_H_
#define RADIO_RECEIVE_H_

#include "em_cmu.h"
#include "em_core.h"
#include "stddef.h"
#include "hardware_config.h"
#include "radio_transmit.h"

void radio_receive__init(void);
bool radio__process_event_rx(RAIL_Handle_t rail_handle, RAIL_Events_t events);
#endif