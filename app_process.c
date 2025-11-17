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
#include "sl_component_catalog.h"
#include "rail.h"
#include "events_prints.h"
#include "rail_types.h"
#include "app_init.h"
#include "sl_rail_util_init.h"
#include "dmadrv.h"
#include "ADC.h"
#include "scheduler.h"
#include <stdio.h>
#include "sl_iostream.h"
#include "app_process.h"
#include "hardware_config.h"
// #include "radio.h"
#include "state_machine.h"
#include "generic.h"

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

const RAIL_CsmaConfig_t CSMA_config = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;

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

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

// uint32_t radio__get_channel(void)
// {
//   return channel;
// }

// void set_channel(uint32_t new_channel)
// {
//   channel = new_channel;
// }

// static void process_data(uint8_t* new_data_pointer)
// {
//   uint32_t j = 0;
//   for (uint16_t i=0 ; i<RADIO_PACKET_DATA_SIZE;)
//     {
//       two_byte_buffer[j+1] = new_data_pointer[i];
//       i++; j++;
//       two_byte_buffer[j-1] = new_data_pointer[i];
//       i++; j++;
//       i++;
//       i++;
//     }
// }

// static void start_tx(uint32_t* new_data_pointer)
// {
//   static uint32_t delay = 0;

//   delay++;

//   if (delay == 1)
//     {
//       delay = 0;
//       RAIL_Handle_t* temp = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
//       //uint32_t return_count = RAIL_WriteTxFifo(temp, (uint8_t*)new_data_pointer, RADIO_PACKET_SIZE, true);

//       header_t header_temp;
//       header_temp.control_bits = 0xCC;
//       header_temp.sequence_number = sequence_number++;
//       uint32_t return_count = RAIL_WriteTxFifo(temp, (uint8_t*)(&header_temp), RADIO_PACKET_HEADER_SIZE, true);
//       if (return_count != RADIO_PACKET_HEADER_SIZE)
//         {
//           printf("Write Header To TX FIFO Failed: 0x%u:\n",(unsigned int)return_count);
//           while(1);
//         }

//       process_data(new_data_pointer);
//       // two_byte_buffer'
//       return_count = RAIL_WriteTxFifo(temp, (uint8_t*)two_byte_buffer, RADIO_PACKET_DATA_SIZE, false);
//       //return_count = RAIL_WriteTxFifo(temp, (uint8_t*)TEST_BUFFER, RADIO_PACKET_DATA_SIZE, false);

//       if (return_count != RADIO_PACKET_DATA_SIZE)
//         {
//           printf("Write Data To TX FIFO Failed: 0x%u:\n",(unsigned int)return_count);
//           while(1);
//         }


//       RAIL_Status_t status = RAIL_StartTx(temp, channel, RAIL_TX_OPTIONS_DEFAULT, NULL);
//       //AIL_Status_t status = RAIL_StartCcaCsmaTx(temp, channel, RAIL_TX_OPTIONS_DEFAULT, &CSMA_config, NULL);
//       if (status == SL_STATUS_OK)
//         {
//           debug__increment_number_of_TX_attempts();
//         }
//       else
//         {
//           debug__increment_number_of_TX_failed();
//         }
//     }
// }

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
      printf("New events:\n");

      for (uint8_t i=0 ; i<logged_events_count_non_volatile ; i++)
        {
          printf("Event : %u\n", (unsigned int)i);
          for (long long unsigned int j=0 ; j<64 ; j++)
            {
              printf("0x%X : ", (unsigned int)(logged_events_non_volatile[i] & (0x1ULL << j)));
              printf("%s\n", getString((long long unsigned int)(logged_events_non_volatile[i] & (0x1ULL << j))));
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
  //          printf("0x%X : ",temp_events & (0x1ULL << i));
  //          printf("%s\n", getString((long long unsigned int)(temp_events & (0x1ULL << i))));
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

  state_machine__run_state_machine();
  RESET_LOOP_IF_NECESSARY()

  debug__increment_counter(app_process_action_runs);
  RESET_LOOP_IF_NECESSARY()

  radio__run_process();
  RESET_LOOP_IF_NECESSARY()

  scheduler__run_scheduler();
  RESET_LOOP_IF_NECESSARY()

  audio_buffers__run_process();
  RESET_LOOP_IF_NECESSARY()

  debug__run_debug_print_state_machine();
  RESET_LOOP_IF_NECESSARY()

  uint8_t* new_data_pointer = 0;
  if (adc__get_new_packet_ready_for_processing(&new_data_pointer) == true)
    {
      rgb__check_level(new_data_pointer);
    }
  RESET_LOOP_IF_NECESSARY()

  //     check_flags();
  //     RESET_LOOP_IF_NECESSARY()

  return;


  //printf("mp\n");

  //  sl_iostream_printf(sl_iostream_inst_handle,"init Inst\n");
  //
  //  static char buffer[10] = {'T','E','S','T'};
  //  sl_iostream_write(sl_iostream_inst_handle,buffer,4);

  //  for (uint32_t i=0 ; i<RADIO_PACKET_DATA_SIZE ; i++)
  //    {
  //      TEST_BUFFER[i]=i;
  //    }
  //
  //  if (channel_changed_flag == true)
  //    {
  //      channel_changed_flag = false;
  //      printf("Channel Changed, New Channel: %u\n",channel);
  //      start_radio_status_blink(get_millisecond_ticks());
  //    }

  //  uint32_t* non_volatile_new_data_pointer;
  //  if (get_new_data_ready_flag(&non_volatile_new_data_pointer) == true)
  //    {
  //radio__add_channel_data_to_buffer(non_volatile_new_data_pointer, LEFT);
  //**** Notify Consumers

  // Start TX
  //start_tx(non_volatile_new_data_pointer);

  // Check for Level
  //      check_level(non_volatile_new_data_pointer);
  //    }













  //print_events();
  //  if (events_saved != 0)
  //    {
  //
  //      for (long long unsigned int j=0 ; j<64 ; j++)
  //        {
  //          uint64_t get_event_bit = events_saved & (0x1ULL << j);
  //          if (get_event_bit)
  //            {
  //              printf("0x%X : ",events_saved & (0x1ULL << j));
  //              printf("%s\n", getString((long long unsigned int)(events_saved & (0x1ULL << j))));
  //            }
  //        }
  //      events_saved = 0;
  //    }


  //  for (uint8_t i = 0 ; i < 10 ; i++)
  //    {
  // if (debug_signals[1] != 0)
  //   {
  //     printf("Debug Signal Set - %i : %X\n",(unsigned int)1,(unsigned int)debug_signals[1]);
  //     printf("VDAC Status - %X\n",VDAC0->STATUS);
  //     printf("VDAC Config - %X\n",VDAC0->CFG);
  //     printf("VDAC Outctrl : %X\n",VDAC0->OUTCTRL);
  //     printf("VDAC IF : %X\n",VDAC0->IF);

  //     printf("\n");
  //     debug_signals[1] = 0;
  //   }

  // if (debug_signals[0] != 0)
  //   {
  //     printf("Debug Signal Set - %i : %X\n",0,debug_signals[0]);
  //     printf("UART Status - %X\n",USART0->STATUS);
  //     printf("UART I2S CTRL - %X\n",USART0->I2SCTRL);
  //     printf("DMA Status : %X\n",LDMA->STATUS);
  //     printf("DMA Channel Status : %X\n",LDMA->CHSTATUS);
  //     printf("DMA Channel Enable : %X\n",LDMA->CHEN);
  //     printf("DMA Channel Busy : %X\n",LDMA->CHBUSY);

  //     Ecode_t DMADRV_return_status;
  //     bool active = 0;



  //     DMADRV_return_status = DMADRV_TransferActive(LDMA_CHANNEL_LEFT,&active);
  //     if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  //       {
  //         printf("Check Active Left Error: %X\n",DMADRV_return_status);
  //         while(1);
  //       }
  //     printf("DMA Left Active : %b\n", active);

  //     DMADRV_return_status = DMADRV_TransferActive(LDMA_CHANNEL_RIGHT,&active);
  //     if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  //       {
  //         printf("Check Active Right Error: %X\n",DMADRV_return_status);
  //         while(1);
  //       }
  //     printf("DMA Right Active : %b\n", active);



  //     DMADRV_return_status = DMADRV_TransferDone(LDMA_CHANNEL_LEFT,&active);
  //     if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  //       {
  //         printf("Check Complete Left Error: %X\n",DMADRV_return_status);
  //         while(1);
  //       }
  //     printf("DMA Left Complete : %b\n", active);

  //     DMADRV_return_status = DMADRV_TransferDone(LDMA_CHANNEL_RIGHT,&active);
  //     if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  //       {
  //         printf("Check Complete Right Error: %X\n",DMADRV_return_status);
  //         while(1);
  //       }
  //     printf("DMA Right Complete : %b\n", active);

  //     printf("\n");
  //     debug_signals[0] = 0;
  //   }
  //    }

  //USART0->TXDATA = 0x55;

  //  RAIL_Handle_t* temp = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
  //  uint32_t return_status = RAIL_WriteTxFifo(temp, &(buffer[0]), PACKET_SIZE, false);
  //  RAIL_Status_t status = RAIL_StartTx(temp, 0, RAIL_TX_OPTIONS_DEFAULT, NULL);

  //  if (newEventFlag == true)
  //    {
  //      printf("  new events:\n");
  //      newEventFlag = false;
  //      long long unsigned int temp_events = events_saved;
  //
  //      for (long long unsigned int i=0 ; i<64 ; i++)
  //        {
  //          //          long long unsigned int temp_events = (events >> i) & 0x01;
  //
  //          if ((temp_events >> i) & 0x01)
  //            {
  //              printf("0x%X : ",temp_events & (0x1ULL << i));
  //              printf("%s\n", getString((long long unsigned int)(temp_events & (0x1ULL << i))));
  //            }
  //        }
  //    }
  //  if (one_second_tick == true)
  //    {
  //      static uint32_t tick_cnt = 0;
  //      one_second_tick = false;
  //      printf("Tick: %i\n", tick_cnt++);
  //
  //      //      printf("Debug Signals:");
  //      //      for (uint8_t i = 0 ; i < 10 ; i++)
  //      //        {
  //      //          printf("%i : %i\n",i,debug_signals[i]);
  //      //        }
  //      //      printf("\n\n\n");
  //    }
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
