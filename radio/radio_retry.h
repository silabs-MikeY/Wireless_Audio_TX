#ifndef RADIO_RETRY_H_
#define RADIO_RETRY_H_

#include "stddef.h"
#include <stdbool.h>
#include <stdint.h>

bool radio_retry__run_process(void);
void radio_retry__init(void);
void radio_retry__add_missing_packet_entry(uint16_t sequence_number);
void radio_retry__note_retry_packet_successfully_sent(uint16_t sequence_number);
bool radio__send_packet_by_sequence_number(uint16_t sequence_number, bool retry);

#endif /* ADC_H_ */