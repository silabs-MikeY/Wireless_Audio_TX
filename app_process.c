/***************************************************************************//**
 * @file
 * @brief app_process.c
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/


// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "RGB.h"
#include "button.h"
#include "events_prints.h"
#include "print.h"
#include "radio_base.h"
#include "rail_types.h"
#include "sl_rail_util_init.h"
#include "ADC.h"
#include "microseconds.h"
#include "scheduler.h"
#include "sl_iostream.h"
#include "app_process.h"
#include "audio_pipeline.h"
#include "hardware_config.h"
// #include "radio.h"
#include "radio_packet_buffers.h"
#include "radio_statistics.h"
#include "state_machine.h"
#include "generic.h"
#include "counters_new.h"
#include "wdog.h"

#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "app_task_init.h"
#endif

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

volatile uint8_t newEventFlag = false;
volatile long long unsigned int events_saved = 0;

extern uint8_t tx_buffer[RADIO_PAYLOAD_SIZE];
extern uint8_t data_buffer[RADIO_PACKET_DATA_SIZE];
extern uint8_t test_buffer[RADIO_PACKET_DATA_SIZE];
uint8_t two_byte_buffer[RADIO_PACKET_DATA_SIZE];

extern volatile uint32_t one_second_tick;

extern volatile uint32_t debug_signals[10];
extern unsigned int LDMA_CHANNEL_LEFT;
extern unsigned int LDMA_CHANNEL_RIGHT;

volatile uint32_t debug_signals[10];
#define LOGGED_EVENTS_SIZE 100
volatile RAIL_Events_t logged_events[LOGGED_EVENTS_SIZE];
volatile uint32_t logged_events_count = 0;

// uint32_t channel = 10;
// extern uint32_t channel_changed_flag;
extern sl_iostream_t *sl_iostream_inst_handle;

uint8_t TEST_BUFFER[RADIO_PACKET_DATA_SIZE];

uint32_t sequence_number = 0;

static volatile bool app_process_audio_stereo_enabled = false;
static volatile bool app_process_audio_encoder_enabled = false;

const RAIL_CsmaConfig_t CSMA_config = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;

uint32_t app_process_action_run_delta_micros = 0;

typedef enum app_process_counter_index_s {
  APP_PROCESS_ACTION_RUNS = 0,
  APP_PROCESS_ACTION_RUNS_MAX_MICROS_BETWEEN_RUNS,
  APP_PROCESS_NUMBER_OF_COUNTERS
} app_process_counter_index_t;

static volatile uint32_t app_process_counter_values[APP_PROCESS_NUMBER_OF_COUNTERS] = {0};
static const char *app_process_counter_names[APP_PROCESS_NUMBER_OF_COUNTERS] = {
  "app_process_action_runs",
  "app_process_action_runs_max_micros_between_runs",
};

static uint32_t app_process_action_last_run_micros = 0;

uint32_t app_process__get_number_of_counters(void)
{
  return APP_PROCESS_NUMBER_OF_COUNTERS;
}

const char *app_process__get_counter_name(uint32_t counter_index)
{
  if (counter_index >= APP_PROCESS_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return app_process_counter_names[counter_index];
}

volatile uint32_t *app_process__get_counter_address(uint32_t counter_index)
{
  if (counter_index >= APP_PROCESS_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return &app_process_counter_values[counter_index];
}

void app_process__reset_counters(void)
{
  memset((void *)app_process_counter_values, 0, sizeof(app_process_counter_values));
  app_process_action_run_delta_micros = 0;
  app_process_action_last_run_micros = microseconds__get_micros_count();
}

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------
void check_flags(void)
{

}
// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------

uint16_t sequenceNumber;

void radio_transmit__handle_successful_packet_sent(uint32_t packet_buffer_index)
{
  const packet_buffer_t *packet_buffer = radio_packet_buffers__get_packet_buffer(packet_buffer_index);

  if (radio_packet_buffers__mark_packet_buffer_completed(packet_buffer_index) == false)
  {
    return;
  }

  if (packet_buffer != NULL)
  {
    radio_statistics__note_successful_tx(packet_buffer->payload.header.sequence_number,
                                         packet_buffer->micros_timestamp_packet_sent);
  }
}

void reset_app_process_action_run_delta_micros(void)
{
  app_process_action_run_delta_micros = 0;
}

void app_process__set_audio_mode(bool is_stereo, bool enable_encoder)
{
  app_process_audio_stereo_enabled = is_stereo;
  app_process_audio_encoder_enabled = enable_encoder;
}

bool app_process__is_audio_stereo(void)
{
  return app_process_audio_stereo_enabled;
}

bool app_process__is_audio_encoder_enabled(void)
{
  return app_process_audio_encoder_enabled;
}

void print_events(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  static RAIL_Events_t logged_events_non_volatile[LOGGED_EVENTS_SIZE];
  uint32_t logged_events_count_non_volatile = logged_events_count;
  memcpy_from_volatile((uint8_t*)&logged_events_non_volatile,(volatile uint8_t*)&logged_events,sizeof(logged_events));
  memset_volatile((volatile uint8_t*)&logged_events,0,sizeof(logged_events));
  logged_events_count = 0;

  CORE_EXIT_CRITICAL();

  if (logged_events_count_non_volatile > 0)
    {
      debug__printf_to_buf_append_time(0,"New events:\n");

      for (uint8_t i=0 ; i<logged_events_count_non_volatile ; i++)
        {
          debug__printf_to_buf_append_time(0,"Event : %u\n", (unsigned int)i);
          for (long long unsigned int j=0 ; j<64 ; j++)
            {
              debug__printf_to_buf_append_time(0,"0x%X : ", (unsigned int)(logged_events_non_volatile[i] & (0x1ULL << j)));
              debug__printf_to_buf_append_time(0,"%s\n", getString((long long unsigned int)(logged_events_non_volatile[i] & (0x1ULL << j))));
            }
        }
    }

  //  long long unsigned int temp_events = events_saved;
  //
  //  for (long long unsigned int i=0 ; i<64 ; i++)
  //    {
  //      //          long long unsigned int temp_events = (events >> i) & 0x01;
  //
  //      if ((temp_events >> i) & 0x01)
  //        {
  //          printf_to_buf_append_time(0,"0x%X : ",temp_events & (0x1ULL << i));
  //          printf_to_buf_append_time(0,"%s\n", getString((long long unsigned int)(temp_events & (0x1ULL << i))));
  //        }
  //    }
}

volatile bool reset_loop_flag = false;
void set_reset_loop_flag(void)
{
  reset_loop_flag = true;
}

#define RESET_LOOP_IF_NECESSARY() \
    if(reset_loop_flag == true) \
    { \
        reset_loop_flag=false; \
        return; \
    }

/******************************************************************************
 * Application state machine, called infinitely
 *****************************************************************************/
void app_process_action(RAIL_Handle_t rail_handle)
{
  (void) rail_handle;

  ///////////////////////////////////////////////////////////////////////////
  // Put your application code here!                                       //
  // This is called infinitely.                                            //
  // Do not call blocking functions from here!                             //
  ///////////////////////////////////////////////////////////////////////////

  scheduler__update_millis();

  app_process_counter_values[APP_PROCESS_ACTION_RUNS]++;

  uint32_t timestamp_now = microseconds__get_micros_count();
  uint32_t delta = timestamp_now - app_process_action_last_run_micros;
  if (delta > app_process_action_run_delta_micros)
  {
    app_process_action_run_delta_micros = delta;
    app_process_counter_values[APP_PROCESS_ACTION_RUNS_MAX_MICROS_BETWEEN_RUNS] = delta;
  }
  app_process_action_last_run_micros = timestamp_now;

  //RESET_LOOP_IF_NECESSARY()

  state_machine__run_state_machine();
  RESET_LOOP_IF_NECESSARY()

  scheduler__run_scheduler();
  RESET_LOOP_IF_NECESSARY()

  // audio_buffers__run_process();
  // RESET_LOOP_IF_NECESSARY()

  radio__run_process();
  RESET_LOOP_IF_NECESSARY()

  debug__check_print_buffers_and_print();
  counters__run_print_state_machine();
  RESET_LOOP_IF_NECESSARY()

  audio_pipeline__run_process();
  RESET_LOOP_IF_NECESSARY()

  // uint8_t* new_data_pointer = 0;
  // if (adc__get_new_packet_ready_for_processing(&new_data_pointer) == true)
  //   {
  //     rgb__check_level(new_data_pointer);
  //   }
  RESET_LOOP_IF_NECESSARY()

  //     check_flags();
  //     RESET_LOOP_IF_NECESSARY()

  return;
}

 void run_scheduler_1_ms(void)
 {
  uint32_t current_millisecond_ticks = scheduler__get_millisecond_ticks();

  rgb__run_signal_intensity_state_machine(current_millisecond_ticks);
  rgb__run_radio_status_blink(current_millisecond_ticks);
 }

 void run_scheduler_10_ms(void)
 {
  WDOGn_Feed(WDOG0);
 }

 void run_scheduler_100_ms(void)
 {

 }

 void run_scheduler_1s(void)
 {
  static uint32_t delay_start = 3;
  if (delay_start == 0)
  {
    //counters__one_second_print();
    counters__save_and_print_counters(scheduler__get_microsecond_ticks());
  }
  else
  {
    delay_start--;
  }
 }

/******************************************************************************
 * RAIL callback, called if a RAIL event occurs
 *****************************************************************************/
void sl_rail_util_on_event(RAIL_Handle_t rail_handle, RAIL_Events_t events)
{
  (void) rail_handle;
  (void) events;

  newEventFlag = true;
  events_saved = events;

  radio__process_event(rail_handle,events);

  ///////////////////////////////////////////////////////////////////////////
  // Put your RAIL event handling here!                                    //
  // This is called from ISR context.                                      //
  // Do not call blocking functions from here!                             //
  ///////////////////////////////////////////////////////////////////////////

#if defined(SL_CATALOG_KERNEL_PRESENT)
  app_task_notify();
#endif
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------
