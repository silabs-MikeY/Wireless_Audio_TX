#ifndef RADIO_CONFIG_H_
#define RADIO_CONFIG_H_

#include <stdbool.h>
#include "em_cmu.h"
#include "em_core.h"
#include "em_prs.h"
#include "em_gpio.h"
#include "generic.h"
#include "stddef.h"
#include "hardware_config.h"
#include "rail_types.h"
#include "sl_rail_util_init.h"
#include "radio_transmit.h"
#include "radio_statistics.h"
#include "radio_receive.h"
#include "radio_transmit.h"


#define ENABLE_PRS 0

#define NUMBER_OF_PACKET_BUFFERS 20

#define CONTROL_BITS__STEREO BIT(0)
#define CONTROL_BITS__RETRY BIT(1)
#define CONTROL_BITS__COMMAND_PACKET BIT(2)
#define CONTROL_BITS__BIT3_UNUSED BIT(3)
#define CONTROL_BITS__BIT4_UNUSED BIT(4)
#define CONTROL_BITS__BIT5_UNUSED BIT(5)
#define CONTROL_BITS__BIT6_UNUSED BIT(6)
#define CONTROL_BITS__BIT7_UNUSED BIT(7)

uint8_t* radio__get_next_buffer(bool left_or_right_data);
void radio__process_event(RAIL_Handle_t rail_handle, RAIL_Events_t events);
void radio__init(void);
void radio__deinit(void);
uint32_t radio__get_sequence_number(void);
void radio__increment_sequence_number(void);
void radio__request_increment_channel(void);
uint32_t radio__get_channel(void);
bool radio__get_channel_changed_flag(void);
void radio__reset_channel_chanegd_flag(void);
void radio__run_process(void);
bool radio__is_radio_busy(void);
void radio__printf(bool add_timestamp, const char *format, ...);

#endif