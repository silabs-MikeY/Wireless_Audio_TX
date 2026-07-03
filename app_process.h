/***************************************************************************//**
 * @file
 * @brief app_process.h
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
#ifndef APP_PROCESS_H
#define APP_PROCESS_H

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "rail.h"
#include "hardware_config.h"
#include <stdbool.h>
// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Declarations
// -----------------------------------------------------------------------------

uint32_t radio__get_channel(void);
void set_channel(uint32_t new_channel);
void set_reset_loop_flag(void);
void reset_app_process_action_run_delta_micros(void);
void app_process__set_audio_mode(bool is_stereo, bool enable_encoder);
bool app_process__is_audio_stereo(void);
bool app_process__is_audio_encoder_enabled(void);

uint32_t app_process__get_number_of_counters(void);
const char *app_process__get_counter_name(uint32_t counter_index);
volatile uint32_t *app_process__get_counter_address(uint32_t counter_index);
void app_process__reset_counters(void);

/**************************************************************************//**
 * The function is used for Application logic.
 *
 * @param[in] rail_handle RAIL handle
 *
 * The function is used for Application logic.
 * It is called infinitely.
 *****************************************************************************/
void app_process_action(RAIL_Handle_t rail_handle);
#endif  // APP_PROCESS_H
