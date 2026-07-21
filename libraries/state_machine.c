#include "state_machine.h"
#include <stdbool.h>
#include <stdint.h>
#include "sl_core.h"

static volatile states_t state = INIT;
static volatile states_t last_state = NONE;
static volatile states_t next_state = NONE;

volatile bool first_packet_received_flag = false;

volatile bool force_error_flag = false;

// -----------------------------------------------------------------------------
//                     Weak function implementations, do not rename.
// -----------------------------------------------------------------------------

__attribute__((weak)) void state_machine__printf(bool add_timestamp, const char *format, ...)
{
    (void)add_timestamp;
    (void)format;
}

__attribute__((weak)) bool state_machine__process_state_running(states_t last_state)
{
  (void)last_state;
  return false;
}

__attribute__((weak)) bool state_machine__process_state_init(states_t last_state)
{
  (void)last_state;
  return false;
}

__attribute__((weak)) bool state_machine__process_state_error(states_t last_state)
{
  (void)last_state;
  return false;
}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     State Machine General
// -----------------------------------------------------------------------------

bool state_machine__run_state_machine(void)
{
  bool restart_app_process_loop = false;

  if (next_state != NONE)
  {
    state_machine__printf(0, false, "Next State Requested: %s , Current: %s\n", STATE_NAMES[next_state], STATE_NAMES[state]);
    state = next_state;
    next_state = NONE;
  }

  if (state != last_state)
  {
    state_machine__printf(0, false, "New State : %s , Previous : %s\n", STATE_NAMES[state], STATE_NAMES[last_state]);
  }

  if (force_error_flag == true)
  {
    force_error_flag = false;
    state = ERROR;
  }

  switch (state)
  {
  case INIT:
    restart_app_process_loop = state_machine__process_state_init(last_state);
    break;
  case RUNNING:
    restart_app_process_loop = state_machine__process_state_running(last_state);
    break;
  case ERROR:
    restart_app_process_loop = state_machine__process_state_error(last_state);
    break;
  case NONE:
  default:
    state_machine__printf(0, false,   "State = None");
    // next_state_request = ERROR;
    break;
  }

  last_state = state;

  if (restart_app_process_loop)
  {
    return true;
  }
  return false;
}

states_t state_machine__get_state(void)
{
  return state;
}

void state_machine__set_next_state(states_t new_state)
{
  next_state = new_state;
}

void state_machine__force_state_machine_error(void)
{
  // CORE_DECLARE_IRQ_STATE;
  // CORE_ENTER_CRITICAL();

  force_error_flag = true;

  state_machine__printf(0, false, "Forcing Error\n");

  // CORE_EXIT_CRITICAL();
}

// -----------------------------------------------------------------------------
//                     State Machine General End
// -----------------------------------------------------------------------------
