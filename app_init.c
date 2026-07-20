/***************************************************************************//**
 * @file
 * @brief app_init.c
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

#include "app_init.h"
#include "sl_iostream.h"
#include "sl_iostream_handles.h"
#include "sl_rail_util_init.h"
#include "state_machine.h"
#include "generic.h"
#include "print_interfacing.h"
#include "rail.h"

volatile uint32_t msTicks = 0;
uint32_t get_msTicks(void)
{
  return msTicks;
}
volatile uint32_t one_second_tick = 0;
//extern sl_iostream_t *sl_iostream_vcom_handle;
extern sl_iostream_t *sl_iostream_inst_handle;

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

//#define BSP_BATTERYMON_TX_ACTIVE_CHANNEL  6
//#define BSP_BATTERYMON_TX_ACTIVE_PORT     gpioPortB
//#define BSP_BATTERYMON_TX_ACTIVE_PIN      1
//
//#define PRS_SOURCE                        PRS_ASYNC_CH_CTRL_SOURCESEL_RACL
//#define PRS_SIGNAL                        PRS_ASYNC_CH_CTRL_SIGSEL_RACLTX

#define BSP_BATTERYMON_TX_ACTIVE_CHANNEL  6

#define PRS_SOURCE                        PRS_ASYNC_CH_CTRL_SOURCESEL_RACL
#define PRS_SIGNAL                        PRS_ASYNC_CH_CTRL_SIGSEL_RACLTX

#define PRS_EDGE                         prsEdgeOff
#define PRS_PIN_SHIFT \
    (8 * (BSP_BATTERYMON_TX_ACTIVE_CHANNEL % 4))
#define PRS_PIN_MASK                     (0x1F << PRS_PIN_SHIFT)


// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------


//uint8_t tx_buffer[RADIO_FIFO_SIZE] __attribute__((aligned(4)));
//uint8_t data_buffer[RADIO_PACKET_DATA_SIZE];
//uint8_t test_buffer[RADIO_PACKET_DATA_SIZE];
//uint8_t payload[RADIO_PACKET_SIZE];

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

/******************************************************************************
 * The function is used for some basic initialization related to the app.
 *****************************************************************************/
RAIL_Handle_t app_init(void)
{
  // Get RAIL handle, used later by the application
  RAIL_Handle_t rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);

  /////////////////////////////////////////////////////////////////////////////
  // Put your application init code here!                                    //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  process_device_reset();

  sl_iostream_set_default(sl_iostream_get_handle("inst"));
  print_interfacing__init();

  // printf("\n\n\nTransmit Application Start\n\n\n");
  print_interfacing__printf_static_string(0, true, "\n\n\nTransmit Application Start\n\n\n");

  state_machine__run_state_machine();

  return rail_handle;
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------
