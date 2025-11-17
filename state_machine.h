#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

//#include "em_cmu.h"
//#include "em_gpio.h"
#include "em_core.h"
#include "em_chip.h"
#include "hardware_config.h"
#include "RGB.h"
#include "debug.h"
#include "sl_rail_util_init.h"
#include "app_process.h"
#include "vdac.h"
#include "button.h"
//#include "radio.h"
#include "vdac.h"
#include "scheduler.h"
#include "ADC.h"
#include "button.h"
#include "wdog.h"
#include "microseconds.h"
#include "radio_base.h"

typedef enum
{
  NONE,
  INIT,
  RUNNING,
  ERROR,
} states_t;

static const char *STATE_NAMES[] __attribute__ ((used)) = {
    "NONE",
    "INIT",
    "RUNNING",
    "ERROR",
};

states_t state_machine__get_state(void);

void state_machine__run_state_machine(void);
//void processStateNode(void);
bool state_machine__process_state_init(void);
bool state_machine__process_state_idle(void);
bool state_machine__process_state_error(void);
bool state_machine__process_state_running(void);

void state_machine__force_state_machine_error(void);

bool state_machine__init_peripherals(void);
bool state_machine__de_init_peripherals(void);

void state_machine__set_first_packet_received_flag(void);

#endif /* STATE_MACHINE_H_ */
