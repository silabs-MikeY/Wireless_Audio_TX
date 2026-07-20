#include "radio_base.h"
#include "rail.h"
#include "sl_rail_util_init.h"
#include "radio_statistics.h"
#include "radio_transmit.h"
#include "radio_receive.h"

volatile uint32_t next_sequence_number = 0;
uint32_t channel = 10;
bool channel_changed_flag = false;
bool searching_for_channel_flag = false;
volatile bool request_channel_increment_flag = false;
static volatile bool radio_calibration_pending = false;
static RAIL_CalValues_t radio_calibration_values = RAIL_CALVALUES_UNINIT;

#define ENABLE_PRS 0

#if (ENABLE_PRS == 1)
static void radio__start_radio_rx_prs(void)
#endif

static bool radio__run_pending_calibration(void);

__attribute__((weak)) void radio__printf(bool add_timestamp, const char *format, ...)
{
  (void)add_timestamp;
  (void)format;
}

// -----------------------------------------------------------------------------
//                     Sequence Number
// -----------------------------------------------------------------------------

/**
 * @brief Gets the current TX sequence number.
 * Returns the next sequence number to be assigned to outgoing packets.
 *
 * @param None
 * @return Current sequence number (0-65535 range)
 */
uint32_t radio__get_next_sequence_number(void)
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

#if (DEBUG_TEST_MISSING_SEQUENCE_NUMBER == 1)
  if (next_sequence_number == 1000)
  {
    next_sequence_number++;
  }
#endif
}

// -----------------------------------------------------------------------------
//                     Sequence Number End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Channel Management
// -----------------------------------------------------------------------------

/**
 * @brief Increments the RF channel (hopping pattern).
 * Cycles through channels 0-19, wrapping at 20. Sets flag and restarts RX on new channel.
 * Used for frequency hopping / channel switching.
 *
 * @param None
 * @return void
 */
static void radio__check_if_update_channel_needed_and_update(void)
{
  if (request_channel_increment_flag == true)
  {
    request_channel_increment_flag = false;
    channel++;
  if (channel == NUMBER_OF_CHANNELS)
  {
    channel = 0;
  }
  channel_changed_flag = true;
  }
}

void radio__request_increment_channel(void)
{
  request_channel_increment_flag = true;
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
void radio__reset_channel_changed_flag(void)
{
  channel_changed_flag = false;
}

// -----------------------------------------------------------------------------
//                     Channel Management End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Radio General
// -----------------------------------------------------------------------------

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
  #if (ENABLE_PRS == 1)
  radio__start_radio_rx_prs();
  #endif
  radio_statistics__init();
  radio_transmit__init();
  radio_receive__init();
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
 * @brief Starts PRS (Peripheral Reflex System) output for radio RX signals.
 * Configures PRS to output RAIL RX state on GPIO for debugging/monitoring.
 *
 * @param None
 * @return void
 */
#if (ENABLE_PRS == 1)
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
#endif

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

  if ((events & RAIL_EVENT_CAL_NEEDED) != 0)
  {
    radio_calibration_pending = true;
  }

  radio__process_event_tx(rail_handle, events);
  // radio__process_event_rx(rail_handle, events);

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
  
  if (radio__is_radio_busy() == false)
  {
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    radio__check_if_update_channel_needed_and_update();
    radio__run_pending_calibration();
    CORE_EXIT_CRITICAL();
  }
}

static bool radio__run_pending_calibration(void)
{
  bool run_calibration = false;

  if (radio_calibration_pending)
  {
    radio_calibration_pending = false;
    run_calibration = true;
  }

  if (!run_calibration)
  {
    return false;
  }

  RAIL_Handle_t rail_handle =
      sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
  RAIL_Status_t status = RAIL_Calibrate(
      rail_handle, &radio_calibration_values, RAIL_CAL_ALL_PENDING);

  if (status != RAIL_STATUS_NO_ERROR)
  {
    // RAIL can reject calibration during a transient protocol/radio state.
    // Keep the request pending and retry it from a later main-loop pass.
    radio_calibration_pending = true;
  }

  return true;
}

/**
 * @brief Checks if radio is currently busy (transmitting or in other active state).

    radio_transmit__run_process();
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

// -----------------------------------------------------------------------------
//                     Radio General End
// -----------------------------------------------------------------------------
