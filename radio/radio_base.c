#include "radio_base.h"

static void radio__start_radio_rx_prs(void);

volatile uint32_t next_sequence_number = 0;
uint32_t channel = 10;
bool channel_changed_flag = false;
bool searching_for_channel_flag = false;

/**
 * @brief Gets the current TX sequence number.
 * Returns the next sequence number to be assigned to outgoing packets.
 * 
 * @param None
 * @return Current sequence number (0-65535 range)
 */
uint32_t radio__get_sequence_number(void)
{
  return next_sequence_number;
}

/**
 * @brief Increments the TX sequence number for the next packet.
 * Wraps around to 0 after 0xFFFF. Updates debug counter and can force
 * skip specific sequence numbers for testing purposes (see DEBUG_TEST_MISSING_SEQUENCE_NUMBER).
 * 
 * @param None
 * @return void
 */
void radio__increment_sequence_number(void)
{
  if (next_sequence_number == 0xFFFF)
  {
    next_sequence_number = 0;
  }
  else
  {
    next_sequence_number++;
  }

  debug__set_counter(sequence_number_at_end, next_sequence_number);

#define DEBUG_TEST_MISSING_SEQUENCE_NUMBER 1 // forces a sequence number to be skipped
#if (DEBUG_TEST_MISSING_SEQUENCE_NUMBER == 1)
  if (next_sequence_number == 1000)
  {
    next_sequence_number++;
  }
#endif
}

/**
 * @brief Increments the RF channel (hopping pattern).
 * Cycles through channels 0-19, wrapping at 20. Sets flag and restarts RX on new channel.
 * Used for frequency hopping / channel switching.
 * 
 * @param None
 * @return void
 */
void radio__increment_channel(void)
{
  channel++;
  if (channel == 20)
  {
    channel = 0;
  }
  channel_changed_flag = true;

  RAIL_Handle_t rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
  RAIL_StartRx(rail_handle, channel, NULL);
}

/**
 * @brief Gets the current RF channel.
 * 
 * @param None
 * @return Current channel number (0-19)
 */
uint32_t radio__get_channel(void)
{
  return channel;
}

/**
 * @brief Gets the channel changed flag state.
 * Flag indicates whether channel was switched since last check.
 * 
 * @param None
 * @return true if channel was changed, false otherwise
 */
bool radio__get_channel_changed_flag(void)
{
  return channel_changed_flag;
}

/**
 * @brief Resets the channel changed flag.
 * Clears the flag after it has been checked/handled by higher-level code.
 * 
 * @param None
 * @return void
 */
void radio__reset_channel_chanegd_flag(void)
{
  channel_changed_flag = false;
}

/**
 * @brief De-initializes radio subsystem (placeholder).
 * Currently empty but reserved for cleanup operations.
 * 
 * @param None
 * @return void
 */
void radio__deinit(void)
{
}

/**
 * @brief Initializes the complete radio subsystem.
 * Configures RAIL events, initializes TX FIFO and RX FIFO,
 * and sets up transmit and receive subsystems.
 * Must be called once at startup.
 * 
 * @param None
 * @return void
 */
void radio__init(void)
{
  RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);

  RAIL_ConfigEvents(rail_handle, RAIL_EVENTS_ALL, RAIL_EVENTS_ALL);

#define ENABLE_PRS 0
#if (ENABLE_PRS == 1)
  radio__start_radio_rx_prs()
#endif

      radio_transmit__init();
  radio_receive__init();

  // uint16_t RAIL_TXFIFO_LENGTH_SIZE = RAIL_SetFixedLength(rail_handle, RADIO_PAYLOAD_SIZE);
  // if (RAIL_TXFIFO_LENGTH_SIZE != RADIO_PAYLOAD_SIZE)
  // {
  //   assert(0);
  // }

  //   uint32_t RAIL_SetTxFifo_return = RAIL_SetTxFifo(rail_handle, (uint8_t *)tx_buffer, 0, RADIO_FIFO_SIZE);
  //   if (RAIL_SetTxFifo_return < RADIO_PAYLOAD_SIZE)
  //   {
  //     printf("RAIL_SetTxFifo Failed. Requested : %u Bytes, Got %u Bytes \n", (unsigned int)RADIO_FIFO_SIZE, (unsigned int)RAIL_SetTxFifo_return);
  //     assert(0);
  //   }
  //   printf("RAIL_SetTxFifo Success, %u Bytes\n", (unsigned int)RAIL_SetTxFifo_return);
  //   FIFO_Size = RAIL_SetTxFifo_return;

  //   RAIL_Status_t RAIL_SetTxPowerDbm_return = RAIL_SetTxPowerDbm(rail_handle, 190);
  //   if (RAIL_SetTxPowerDbm_return != RAIL_STATUS_NO_ERROR)
  //   {
  //     printf("RAIL_SetTxPowerDbm failed, Status Code: %X\n", (unsigned int)RAIL_SetTxPowerDbm_return);
  //     assert(0);
  //   }
  //   printf("RAIL_SetTxPowerDbm Success\n");

  //   RAIL_Status_t RAIL_SetRxFifo_return = RAIL_SetRxFifo(rail_handle, (uint8_t *)RX_FIFO, &RX_FIFO_SIZE);
  //   if (RAIL_SetRxFifo_return != RAIL_STATUS_NO_ERROR)
  //   {
  //     printf("RAIL_SetRxFifo failed, Status Code: %X\n", (unsigned int)RAIL_SetRxFifo_return);
  //     assert(0);
  //   }
  //   if (RX_FIFO_SIZE != RADIO_FIFO_SIZE)
  //   {
  //     printf("RAIL_SetRxFifo Failed. Requested : %u Bytes, Got %u Bytes \n", (unsigned int)RADIO_FIFO_SIZE, (unsigned int)RX_FIFO_SIZE);
  //     assert(0);
  //   }
  //   printf("RAIL_SetRxFifo Success, %u Bytes\n", (unsigned int)RX_FIFO_SIZE);

  // #define ENABLE_PRS 0
  // #if (ENABLE_PRS == 1)

  // #define PRS_SOURCE PRS_ASYNC_CH_CTRL_SOURCESEL_RACL
  // #define PRS_SIGNAL PRS_ASYNC_CH_CTRL_SIGSEL_RACLTX

  //   CMU_ClockEnable(cmuClock_PRS, true);
  //   CMU_ClockEnable(cmuClock_GPIO, true);
  //   GPIO_PinOutSet(gpioPortB, 1);
  //   PRS_SourceAsyncSignalSet(1,
  //                            PRS_SOURCE,
  //                            PRS_SIGNAL);
  //   PRS_PinOutput(1,
  //                 prsTypeAsync,
  //                 gpioPortB,
  //                 1);
  // #endif
}

/**
 * @brief Starts PRS (Peripheral Reflex System) output for radio RX signals.
 * Configures PRS to output RAIL RX state on GPIO for debugging/monitoring.
 * 
 * @param None
 * @return void
 */
static void radio__start_radio_rx_prs(void)
{
#define PRS_SOURCE PRS_ASYNC_CH_CTRL_SOURCESEL_RACL
#define PRS_SIGNAL PRS_ASYNC_CH_CTRL_SIGSEL_RACLTX

  CMU_ClockEnable(cmuClock_PRS, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinOutSet(gpioPortB, 1);
  PRS_SourceAsyncSignalSet(1,
                           PRS_SOURCE,
                           PRS_SIGNAL);
  PRS_PinOutput(1,
                prsTypeAsync,
                gpioPortB,
                1);
}

/**
 * @brief Routes RAIL events to appropriate TX/RX handlers.
 * Dispatches events to radio__process_event_tx() or radio__process_event_rx()
 * based on event type. Called from RAIL event interrupt.
 * 
 * @param rail_handle - RAIL driver handle
 * @param events - Bitmask of RAIL events
 * @return void
 */
void radio__process_event(RAIL_Handle_t rail_handle, RAIL_Events_t events)
{
  (void)rail_handle;
  (void)events;

  if (radio__process_event_tx(rail_handle, events) == true)
  {
    return;
  }
  else if (radio__process_event_rx(rail_handle, events) == true)
  {
    return;
  }

  // logged_events[logged_events_count++] = events;
}

/**
 * @brief Main radio process loop called from scheduler.
 * Attempts to transmit packets and handle retries when radio is idle.
 * 
 * @param None
 * @return void
 */
void radio__run_process(void)
{
  radio_transmit__run_process();
  // if (radio_retry__run_process())
  // {
  //   //return;
  // }
  // radio_retry__run_process();
  // radio__try_to_send_a_packet();
}

/**
 * @brief Checks if radio is currently busy (transmitting or in other active state).
 * Returns false only if radio is in RX, IDLE, or INACTIVE states (can transmit).
 * 
 * @param None
 * @return true if radio is busy, false if idle and ready to transmit
 */
bool radio__is_radio_busy(void)
{
  RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
  RAIL_RadioState_t state = RAIL_GetRadioState(rail_handle);
  if (((state == RAIL_RF_STATE_RX) || (state == RAIL_RF_STATE_IDLE) || (state == RAIL_RF_STATE_INACTIVE)))
  {
    return false;
  }
  return true;
}