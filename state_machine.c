#include "state_machine.h"
#include "print.h"
#include "wdog.h"
#include "microseconds.h"
#include "scheduler.h"
#include "app_process.h"
#include "counters.h"
#include "audio_buffers.h"
#include "RGB.h"
#include "em_usart.h"
#include "ADC.h"
#include "radio_base.h"
#include "RGB.h"
#include "button.h"

static volatile states_t state = INIT;
static volatile states_t last_state = NONE;
static volatile states_t next_state = NONE;

volatile bool first_packet_received_flag = false;

void state_machine__run_state_machine(void)
{
  bool restart_app_process_loop = false;

  if (next_state != NONE)
  {
    DEBUG_STATES_LOG(debug__printf_to_buf_append_time(0,"Next State Requested: %s , Current: %s\n", STATE_NAMES[next_state], STATE_NAMES[state]));
    state = next_state;
    next_state = NONE;
  }

  if (state != last_state)
  {
    DEBUG_STATES_LOG(debug__printf_to_buf_append_time(0,"New State : %s , Previous : %s\n", STATE_NAMES[state], STATE_NAMES[last_state]));
  }

  switch (state)
  {
  case INIT:
    restart_app_process_loop = state_machine__process_state_init();
    break;
  case RUNNING:
    restart_app_process_loop = state_machine__process_state_running();
    break;
  case ERROR:
    restart_app_process_loop = state_machine__process_state_error();
    break;
  case NONE:
  default:
    DEBUG_STATES_LOG(debug__printf_to_buf_append_time(0,"State = None"));
    // next_state_request = ERROR;
    break;
  }

  last_state = state;

  if (restart_app_process_loop)
  {
    set_reset_loop_flag();
  }
}

// void processStateNode(void)
//{
//
// }

bool state_machine__init_peripherals(void)
{
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n"));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Start WDOG Init\n"));
  wdog__init();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End WDOG Init\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Starting Microsecond Init\n"));
  microseconds__init_microsecond();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End Microsecond Init\n"));

  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));
  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"\n- Start Scheduler Init\n");)
  // scheduler__init_SysTick();
  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"- End Scheduler Init\n");)
  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Start Counters Init\n"));
  counters__init_counters();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End Counters Init\n"));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Start Audio Buffers Init\n"));
  audio_buffers__init();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End Audio Buffers Init\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Start Radio Init\n"));
  //radio__init_packet_buffers();
  radio__init();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End Radio Init\n"));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Start RGB Init\n"));
  rgb__init();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End RGB Init\n"));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));

  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start I2C Init - "));
  //  i2c__init();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Test I2C"));
  //  i2c__test();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End I2C Init\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Start Button Init - \n"));
  button__init();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End Button Init\n"));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));

  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start LDMA Init - "));
  //  adc__ldma_init();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End LDMA Init\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Start ADC Init - \n"));
  adc__init();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End ADC Init\n"));
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n"));

  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start Timer Init"));
  //  vdac__init_timer();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End Timer Init\n"));
  //
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start VDAC Init"));
  //  vdac__init_vdac();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End VDAC Init\n"));

  return true;
}

bool state_machine__de_init_peripherals(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Starting RADIO Deinit\n"));
  radio__deinit();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End Radio Deinit\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Starting Microsecond Deinit\n"));
  microseconds__deinit_microsecond();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End Microsecond Deinit\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Starting SysTick Deinit\n"));
  scheduler__deinit_SysTick();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End SysTick Deinit\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Starting Button Deinit\n"));
  button__deinit();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End Button Deinit\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Starting RGB Deinit\n"));
  rgb__deinit();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End RGB Deinit\n"));

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"\n- Starting ADC Deinit\n"));
  adc__deinit();
  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"- End ADC Deinit\n"));

  CORE_EXIT_CRITICAL();
  return false;
}

bool state_machine__process_state_init(void)
{
  if (state != last_state)
  {
    state_machine__de_init_peripherals(); // TODO Implement this
    DEBUG_STATES_LOG(debug__printf_to_buf_append_time(0,"\n**Init Peripherals Start\n"));
    if (state_machine__init_peripherals() == true)
    {
      DEBUG_STATES_LOG(debug__printf_to_buf_append_time(0,"\n**Init Peripherals Successful\n"));
      rgb__start_radio_status_blink_force();
      next_state = RUNNING;
    }
    else
    {
      DEBUG_STATES_LOG(debug__printf_to_buf_append_time(0,"\n**Init Peripherals Failed\n"));
      next_state = ERROR;
    }
    return true;
  }
  return false;
}

bool state_machine__process_state_running(void)
{
  if (state != last_state)
  {
  }
  return false;
}

bool state_machine__process_state_error(void)
{
  // radio__stop_rx();
  if (last_state == RUNNING)
  {
    next_state = INIT;
    rgb__start_radio_status_blink_force();
    set_reset_loop_flag();
    return true;
  }
  return false;
}

states_t state_machine__get_state(void)
{
  return state;
}

void state_machine__force_state_machine_error(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  next_state = ERROR;

  DEBUG_STATES_LOG(debug__printf_to_buf_append_time(0,"Forcing Error\n"));

  // dac__stop_audio_out();
  // radio__stop_rx();
  set_reset_loop_flag();

  CORE_EXIT_CRITICAL();
}
