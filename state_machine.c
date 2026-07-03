#include "state_machine.h"
#include "print.h"
#include "print_interfacing.h"
#include "wdog.h"
#include "microseconds.h"
#include "scheduler.h"
#include "app_process.h"
// #include "audio_buffers.h"
#include "RGB.h"
#include "em_usart.h"
#include "ADC.h"
#include "radio_base.h"
#include "RGB.h"
#include "button.h"
#include <stdbool.h>
#include "counters_new.h"
#include "counter_interface.h"
#include "audio_pipeline.h"
#include "audio_ring_buffer.h"
#include "radio_packet_buffers.h"

static volatile states_t state = INIT;
static volatile states_t last_state = NONE;
static volatile states_t next_state = NONE;

volatile bool first_packet_received_flag = false;

void state_machine__run_state_machine(void)
{
  bool restart_app_process_loop = false;

  if (next_state != NONE)
  {
    print_interfacing__printf(0,"Next State Requested: %s , Current: %s\n", STATE_NAMES[next_state], STATE_NAMES[state]);
    state = next_state;
    next_state = NONE;
  }

  if (state != last_state)
  {
    print_interfacing__printf(0,"New State : %s , Previous : %s\n", STATE_NAMES[state], STATE_NAMES[last_state]);
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
    print_interfacing__printf(0,"State = None");
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
  print_interfacing__printf(0,"\n");
  print_interfacing__printf(0,"\n- Start WDOG Init\n");
  wdog__init();
  print_interfacing__printf(0,"- End WDOG Init\n");

  print_interfacing__printf(0,"\n- Starting Microsecond Init\n");
  microseconds__init_microsecond();
  print_interfacing__printf(0,"- End Microsecond Init\n");

  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));
  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"\n- Start Scheduler Init\n");)
  // scheduler__init_SysTick();
  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"- End Scheduler Init\n");)
  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count()));

  print_interfacing__printf(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count());
  print_interfacing__printf(0,"\n- Start Counters Init\n");
  print_interfacing__printf(0,"\n- Start Audio Buffers Init\n");
  // audio_buffers__init(false);
  bool is_stereo = app_process__is_audio_stereo();
  bool enable_encoder = app_process__is_audio_encoder_enabled();

  ring_buffer__init(is_stereo, enable_encoder);
  audio_pipeline__init(is_stereo, enable_encoder);
  print_interfacing__printf(0,"- End Audio Buffers Init\n");

  print_interfacing__printf(0,"\n- Start Radio Init\n");
  //radio__init_packet_buffers();
  radio_packet_buffers__init();
  radio__init();
  print_interfacing__printf(0,"- End Radio Init\n");
  print_interfacing__printf(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count());

  print_interfacing__printf(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count());
  print_interfacing__printf(0,"\n- Start RGB Init\n");
  rgb__init();
  print_interfacing__printf(0,"- End RGB Init\n");
  print_interfacing__printf(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count());

  print_interfacing__printf(0,"Start New Counters Init\n");
  counters__init();
  counter_interface__init();
  print_interfacing__printf(0,"- End New Counters Init\n");

  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start I2C Init - "));
  //  i2c__init();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Test I2C"));
  //  i2c__test();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End I2C Init\n"));

  print_interfacing__printf(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count());
  print_interfacing__printf(0,"\n- Start Button Init - \n");
  button__init();
  print_interfacing__printf(0,"- End Button Init\n");
  print_interfacing__printf(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count());

  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start LDMA Init - "));
  //  adc__ldma_init();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End LDMA Init\n"));

  print_interfacing__printf(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count());
  print_interfacing__printf(0,"\n- Start ADC Init - \n");
  adc__init(is_stereo);
  print_interfacing__printf(0,"- End ADC Init\n");
  print_interfacing__printf(0,"micros : %u\n", (unsigned int)microseconds__get_micros_count());

  print_interfacing__printf(0,"\n");

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

  print_interfacing__printf(0,"\n- Starting RADIO Deinit\n");
  radio__deinit();
  print_interfacing__printf(0,"- End Radio Deinit\n");

  print_interfacing__printf(0,"\n- Starting Microsecond Deinit\n");
  microseconds__deinit_microsecond();
  print_interfacing__printf(0,"- End Microsecond Deinit\n");

  print_interfacing__printf(0,"\n- Starting SysTick Deinit\n");
  scheduler__deinit_SysTick();
  print_interfacing__printf(0,"- End SysTick Deinit\n");

  print_interfacing__printf(0,"\n- Starting Button Deinit\n");
  button__deinit();
  print_interfacing__printf(0,"- End Button Deinit\n");

  print_interfacing__printf(0,"\n- Starting RGB Deinit\n");
  rgb__deinit();
  print_interfacing__printf(0,"- End RGB Deinit\n");

  print_interfacing__printf(0,"\n- Starting ADC Deinit\n");
  adc__deinit();
  print_interfacing__printf(0,"- End ADC Deinit\n");

  CORE_EXIT_CRITICAL();
  return false;
}

bool state_machine__process_state_init(void)
{
  if (state != last_state)
  {
    state_machine__de_init_peripherals(); // TODO Implement this
    print_interfacing__printf(0,"\n**Init Peripherals Start\n");
    if (state_machine__init_peripherals() == true)
    {
      print_interfacing__printf(0,"\n**Init Peripherals Successful\n");
      rgb__start_radio_status_blink_force();
      next_state = RUNNING;
    }
    else
    {
      print_interfacing__printf(0,"\n**Init Peripherals Failed\n");
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

  print_interfacing__printf(0,"Forcing Error\n");

  // dac__stop_audio_out();
  // radio__stop_rx();
  set_reset_loop_flag();

  CORE_EXIT_CRITICAL();
}
