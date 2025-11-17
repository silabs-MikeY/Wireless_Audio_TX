#ifndef RADIO_RETRY_H_
#define RADIO_RETRY_H_

#include "em_gpio.h"
#include "em_cmu.h"
#include "stddef.h"
#include "scheduler.h"
#include "hardware_config.h"

bool radio_retry__run_process(void);
void radio_retry__init(void);
void radio_retry__add_missing_packet_entry(uint16_t sequence_number);
void radio_retry__note_retry_packet_successfully_sent(uint16_t sequence_number);

#endif /* ADC_H_ */