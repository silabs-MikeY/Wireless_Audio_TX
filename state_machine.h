#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

//#include "em_cmu.h"
//#include "em_gpio.h"
//#include "radio.h"

#include <stdbool.h>
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

void state_machine__printf(bool add_timestamp, const char *format, ...);

states_t state_machine__get_state(void);
void state_machine__set_next_state(states_t new_state);

bool state_machine__run_state_machine(void);

bool state_machine__process_state_init(states_t last_state);
bool state_machine__process_state_error(states_t last_state);
bool state_machine__process_state_running(states_t last_state);

void state_machine__force_state_machine_error(void);

void state_machine__set_first_packet_received_flag(void);

#endif /* STATE_MACHINE_H_ */
