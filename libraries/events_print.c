#include "events_prints.h"
#include "rail_types.h"

// -----------------------------------------------------------------------------
//                     RAIL Event String Helpers
// -----------------------------------------------------------------------------

const char * getString(long long unsigned int inputValue)
{
  switch (inputValue)
  {
    case RAIL_EVENT_RSSI_AVERAGE_DONE:
      return RAIL_EVENT_RSSI_AVERAGE_DONE_TEXT;
      break;
    case RAIL_EVENT_RX_ACK_TIMEOUT:
      return RAIL_EVENT_RX_ACK_TIMEOUT_TEXT;
      break;
    case RAIL_EVENT_RX_FIFO_ALMOST_FULL:
      return RAIL_EVENT_RX_FIFO_ALMOST_FULL_TEXT;
      break;
    case RAIL_EVENT_RX_PACKET_RECEIVED:
      return RAIL_EVENT_RX_PACKET_RECEIVED_TEXT;
      break;
    case RAIL_EVENT_RX_PREAMBLE_LOST:
      return RAIL_EVENT_RX_PREAMBLE_LOST_TEXT;
      break;
    case RAIL_EVENT_RX_PREAMBLE_DETECT:
      return RAIL_EVENT_RX_PREAMBLE_DETECT_TEXT;
      break;
    case RAIL_EVENT_RX_SYNC1_DETECT:
      return RAIL_EVENT_RX_SYNC1_DETECT_TEXT;
      break;
    case RAIL_EVENT_RX_SYNC2_DETECT:
      return RAIL_EVENT_RX_SYNC2_DETECT_TEXT;
      break;
    case RAIL_EVENT_RX_FRAME_ERROR:
      return RAIL_EVENT_RX_FRAME_ERROR_TEXT;
      break;
    case RAIL_EVENT_RX_FIFO_FULL:
      return RAIL_EVENT_RX_FIFO_FULL_TEXT;
      break;
    case RAIL_EVENT_RX_FIFO_OVERFLOW:
      return RAIL_EVENT_RX_FIFO_OVERFLOW_TEXT;
      break;
    case RAIL_EVENT_RX_ADDRESS_FILTERED:
      return RAIL_EVENT_RX_ADDRESS_FILTERED_TEXT;
      break;
    case RAIL_EVENT_RX_TIMEOUT:
      return RAIL_EVENT_RX_TIMEOUT_TEXT;
      break;
    case RAIL_EVENT_SCHEDULED_RX_STARTED:
      return RAIL_EVENT_SCHEDULED_RX_STARTED_TEXT;
      break;
      //    case RAIL_EVENT_SCHEDULED_TX_STARTED:
      //      return RAIL_EVENT_SCHEDULED_TX_STARTED_TEXT;
      //      break;
    case RAIL_EVENT_RX_SCHEDULED_RX_END:
      return RAIL_EVENT_RX_SCHEDULED_RX_END_TEXT;
      break;
    case RAIL_EVENT_RX_SCHEDULED_RX_MISSED:
      return RAIL_EVENT_RX_SCHEDULED_RX_MISSED_TEXT;
      break;
    case RAIL_EVENT_RX_PACKET_ABORTED:
      return RAIL_EVENT_RX_PACKET_ABORTED_TEXT;
      break;
    case RAIL_EVENT_RX_FILTER_PASSED:
      return RAIL_EVENT_RX_FILTER_PASSED_TEXT;
      break;
    case RAIL_EVENT_RX_TIMING_LOST:
      return RAIL_EVENT_RX_TIMING_LOST_TEXT;
      break;
    case RAIL_EVENT_RX_TIMING_DETECT:
      return RAIL_EVENT_RX_TIMING_DETECT_TEXT;
      break;
    case RAIL_EVENT_RX_CHANNEL_HOPPING_COMPLETE:
      return RAIL_EVENT_RX_CHANNEL_HOPPING_COMPLETE_TEXT;
      break;
      //    case RAIL_EVENT_RX_DUTY_CYCLE_RX_END:
      //      return RAIL_EVENT_RX_DUTY_CYCLE_RX_END_TEXT;
      //      break;
    case RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND:
      return RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND_TEXT;
      break;
      //    case RAIL_EVENT_MFM_TX_BUFFER_DONE:
      //      return RAIL_EVENT_MFM_TX_BUFFER_DONE_TEXT;
      //      break;
      //    case RAIL_EVENT_ZWAVE_LR_ACK_REQUEST_COMMAND:
      //      return RAIL_EVENT_ZWAVE_LR_ACK_REQUEST_COMMAND_TEXT;
      //      break;
    case RAIL_EVENTS_RX_COMPLETION:
      return RAIL_EVENTS_RX_COMPLETION_TEXT;
      break;
    case RAIL_EVENT_TX_FIFO_ALMOST_EMPTY:
      return RAIL_EVENT_TX_FIFO_ALMOST_EMPTY_TEXT;
      break;
    case RAIL_EVENT_TX_PACKET_SENT:
      return RAIL_EVENT_TX_PACKET_SENT_TEXT;
      break;
    case RAIL_EVENT_TXACK_PACKET_SENT:
      return RAIL_EVENT_TXACK_PACKET_SENT_TEXT;
      break;
    case RAIL_EVENT_TX_ABORTED:
      return RAIL_EVENT_TX_ABORTED_TEXT;
      break;
    case RAIL_EVENT_TXACK_ABORTED:
      return RAIL_EVENT_TXACK_ABORTED_TEXT;
      break;
    case RAIL_EVENT_TX_BLOCKED:
      return RAIL_EVENT_TX_BLOCKED_TEXT;
      break;
    case RAIL_EVENT_TXACK_BLOCKED:
      return RAIL_EVENT_TXACK_BLOCKED_TEXT;
      break;
    case RAIL_EVENT_TX_UNDERFLOW:
      return RAIL_EVENT_TX_UNDERFLOW_TEXT;
      break;
    case RAIL_EVENT_TXACK_UNDERFLOW:
      return RAIL_EVENT_TXACK_UNDERFLOW_TEXT;
      break;
    case RAIL_EVENT_TX_CHANNEL_CLEAR:
      return RAIL_EVENT_TX_CHANNEL_CLEAR_TEXT;
      break;
    default:
      return "rail status not found";
      break;
      return "rail status not found";
      //    case RAIL_EVENT_TX_CHANNEL_BUSY:
      //      return RAIL_EVENT_TX_CHANNEL_BUSY_TEXT;
      //      break;
//    case RAIL_EVENT_TX_CCA_RETRY:
//      return RAIL_EVENT_TX_CCA_RETRY_TEXT;
//      break;
//    case RAIL_EVENT_TX_START_CCA:
//      return RAIL_EVENT_TX_START_CCA_TEXT;
//      break;
//    case RAIL_EVENT_TX_STARTED:
//      return RAIL_EVENT_TX_STARTED_TEXT;
//      break;
//    case RAIL_EVENT_TX_SCHEDULED_TX_MISSED:
//      return RAIL_EVENT_TX_SCHEDULED_TX_MISSED_TEXT;
//      break;
//    case RAIL_EVENT_CONFIG_UNSCHEDULED:
//      return RAIL_EVENT_CONFIG_UNSCHEDULED_TEXT;
//      break;
//    case RAIL_EVENT_CONFIG_SCHEDULED:
//      return RAIL_EVENT_CONFIG_SCHEDULED_TEXT;
//      break;
//    case RAIL_EVENT_SCHEDULER_STATUS:
//      return RAIL_EVENT_SCHEDULER_STATUS_TEXT;
//      break;
//    case RAIL_EVENT_CAL_NEEDED:
//      return RAIL_EVENT_CAL_NEEDED_TEXT;
//      break;
//    case RAIL_EVENT_RF_SENSED:
//      return RAIL_EVENT_RF_SENSED_TEXT;
//      break;
//    case RAIL_EVENT_PA_PROTECTION:
//      return RAIL_EVENT_PA_PROTECTION_TEXT;
//      break;
//    case RAIL_EVENT_SIGNAL_DETECTED:
//      return RAIL_EVENT_SIGNAL_DETECTED_TEXT;
//      break;
//    case RAIL_EVENT_IEEE802154_MODESWITCH_START:
//      return RAIL_EVENT_IEEE802154_MODESWITCH_START_TEXT;
//      break;
//    case RAIL_EVENT_IEEE802154_MODESWITCH_END:
//      return RAIL_EVENT_IEEE802154_MODESWITCH_END_TEXT;
//      break;
//    case RAIL_EVENT_DETECT_RSSI_THRESHOLD:
//      return RAIL_EVENT_DETECT_RSSI_THRESHOLD_TEXT;
//      break;
//    case RAIL_EVENT_THERMISTOR_DONE:
//      return RAIL_EVENT_THERMISTOR_DONE_TEXT;
//      break;
//    case RAIL_EVENT_TX_BLOCKED_TOO_HOT:
//      return RAIL_EVENT_TX_BLOCKED_TOO_HOT_TEXT;
//      break;
//    case RAIL_EVENT_TEMPERATURE_TOO_HOT:
//      return RAIL_EVENT_TEMPERATURE_TOO_HOT_TEXT;
//      break;
//    case RAIL_EVENT_TEMPERATURE_COOL_DOWN:
//      return RAIL_EVENT_TEMPERATURE_COOL_DOWN_TEXT;
//      break;
  }
}

/*
 * const char * getString(uint32_t inputValue)
{
  switch (inputValue)
  {
    case RAIL_EVENT_RSSI_AVERAGE_DONE:
      //return RAIL_EVENT_RSSI_AVERAGE_DONE_TEXT;
      break;
    case RAIL_EVENT_RX_ACK_TIMEOUT:
      //return RAIL_EVENT_RX_ACK_TIMEOUT_TEXT;
      break;
    case RAIL_EVENT_RX_FIFO_ALMOST_FULL:
      //return RAIL_EVENT_RX_FIFO_ALMOST_FULL_TEXT;
      break;
    case RAIL_EVENT_RX_PACKET_RECEIVED:
      //return RAIL_EVENT_RX_PACKET_RECEIVED_TEXT;
      break;
    case RAIL_EVENT_RX_PREAMBLE_LOST:
      //return RAIL_EVENT_RX_PREAMBLE_LOST_TEXT;
      break;
    case RAIL_EVENT_RX_PREAMBLE_DETECT:
      //return RAIL_EVENT_RX_PREAMBLE_DETECT_TEXT;
      break;
    case RAIL_EVENT_RX_SYNC1_DETECT:
      //return RAIL_EVENT_RX_SYNC1_DETECT_TEXT;
      break;
    case RAIL_EVENT_RX_SYNC2_DETECT:
      //return RAIL_EVENT_RX_SYNC2_DETECT_TEXT;
      break;
    case RAIL_EVENT_RX_FRAME_ERROR:
      //return RAIL_EVENT_RX_FRAME_ERROR_TEXT;
      break;
    case RAIL_EVENT_RX_FIFO_FULL:
      //return RAIL_EVENT_RX_FIFO_FULL_TEXT;
      break;
    case RAIL_EVENT_RX_FIFO_OVERFLOW:
      //return RAIL_EVENT_RX_FIFO_OVERFLOW_TEXT;
      break;
    case RAIL_EVENT_RX_ADDRESS_FILTERED:
      //return RAIL_EVENT_RX_ADDRESS_FILTERED_TEXT;
      break;
    case RAIL_EVENT_RX_TIMEOUT:
      //return RAIL_EVENT_RX_TIMEOUT_TEXT;
      break;
    case RAIL_EVENT_SCHEDULED_RX_STARTED:
      //return RAIL_EVENT_SCHEDULED_RX_STARTED_TEXT;
      break;
    case RAIL_EVENT_SCHEDULED_TX_STARTED:
      //return RAIL_EVENT_SCHEDULED_TX_STARTED_TEXT;
      break;
    case RAIL_EVENT_RX_SCHEDULED_RX_END:
      //return RAIL_EVENT_RX_SCHEDULED_RX_END_TEXT;
      break;
    case RAIL_EVENT_RX_SCHEDULED_RX_MISSED:
      //return RAIL_EVENT_RX_SCHEDULED_RX_MISSED_TEXT;
      break;
    case RAIL_EVENT_RX_PACKET_ABORTED:
      //return RAIL_EVENT_RX_PACKET_ABORTED_TEXT;
      break;
    case RAIL_EVENT_RX_FILTER_PASSED:
      //return RAIL_EVENT_RX_FILTER_PASSED_TEXT;
      break;
    case RAIL_EVENT_RX_TIMING_LOST:
      //return RAIL_EVENT_RX_TIMING_LOST_TEXT;
      break;
    case RAIL_EVENT_RX_TIMING_DETECT:
      //return RAIL_EVENT_RX_TIMING_DETECT_TEXT;
      break;
    case RAIL_EVENT_RX_CHANNEL_HOPPING_COMPLETE:
      //return RAIL_EVENT_RX_CHANNEL_HOPPING_COMPLETE_TEXT;
      break;
    case RAIL_EVENT_RX_DUTY_CYCLE_RX_END:
      //return RAIL_EVENT_RX_DUTY_CYCLE_RX_END_TEXT;
      break;
    case RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND:
      //return RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND_TEXT;
      break;
    case RAIL_EVENT_MFM_TX_BUFFER_DONE:
      //return RAIL_EVENT_MFM_TX_BUFFER_DONE_TEXT;
      break;
    case RAIL_EVENT_ZWAVE_LR_ACK_REQUEST_COMMAND:
      //return RAIL_EVENT_ZWAVE_LR_ACK_REQUEST_COMMAND_TEXT;
      break;
    case RAIL_EVENTS_RX_COMPLETION:
      //return RAIL_EVENTS_RX_COMPLETION_TEXT;
      break;
    case RAIL_EVENT_TX_FIFO_ALMOST_EMPTY:
      //return RAIL_EVENT_TX_FIFO_ALMOST_EMPTY_TEXT;
      break;
    case RAIL_EVENT_TX_PACKET_SENT:
      //return RAIL_EVENT_TX_PACKET_SENT_TEXT;
      break;
    case RAIL_EVENT_TXACK_PACKET_SENT:
      //return RAIL_EVENT_TXACK_PACKET_SENT_TEXT;
      break;
    case RAIL_EVENT_TX_ABORTED:
      //return RAIL_EVENT_TX_ABORTED_TEXT;
      break;
    case RAIL_EVENT_TXACK_ABORTED:
      //return RAIL_EVENT_TXACK_ABORTED_TEXT;
      break;
    case RAIL_EVENT_TX_BLOCKED:
      //return RAIL_EVENT_TX_BLOCKED_TEXT;
      break;
    case RAIL_EVENT_TXACK_BLOCKED:
      //return RAIL_EVENT_TXACK_BLOCKED_TEXT;
      break;
    case RAIL_EVENT_TX_UNDERFLOW:
      //return RAIL_EVENT_TX_UNDERFLOW_TEXT;
      break;
    case RAIL_EVENT_TXACK_UNDERFLOW:
      //return RAIL_EVENT_TXACK_UNDERFLOW_TEXT;
      break;
    case RAIL_EVENT_TX_CHANNEL_CLEAR:
      //return RAIL_EVENT_TX_CHANNEL_CLEAR_TEXT;
      break;
    case RAIL_EVENT_TX_CHANNEL_BUSY:
      //return RAIL_EVENT_TX_CHANNEL_BUSY_TEXT;
      break;
    case RAIL_EVENT_TX_CCA_RETRY:
      //return RAIL_EVENT_TX_CCA_RETRY_TEXT;
      break;
    case RAIL_EVENT_TX_START_CCA:
      //return RAIL_EVENT_TX_START_CCA_TEXT;
      break;
    case RAIL_EVENT_TX_STARTED:
      //return RAIL_EVENT_TX_STARTED_TEXT;
      break;
    case RAIL_EVENT_TX_SCHEDULED_TX_MISSED:
      //return RAIL_EVENT_TX_SCHEDULED_TX_MISSED_TEXT;
      break;
    case RAIL_EVENT_CONFIG_UNSCHEDULED:
      //return RAIL_EVENT_CONFIG_UNSCHEDULED_TEXT;
      break;
    case RAIL_EVENT_CONFIG_SCHEDULED:
      //return RAIL_EVENT_CONFIG_SCHEDULED_TEXT;
      break;
    case RAIL_EVENT_SCHEDULER_STATUS:
      //return RAIL_EVENT_SCHEDULER_STATUS_TEXT;
      break;
    case RAIL_EVENT_CAL_NEEDED:
      //return RAIL_EVENT_CAL_NEEDED_TEXT;
      break;
    case RAIL_EVENT_RF_SENSED:
      //return RAIL_EVENT_RF_SENSED_TEXT;
      break;
    case RAIL_EVENT_PA_PROTECTION:
      //return RAIL_EVENT_PA_PROTECTION_TEXT;
      break;
    case RAIL_EVENT_SIGNAL_DETECTED:
      //return RAIL_EVENT_SIGNAL_DETECTED_TEXT;
      break;
    case RAIL_EVENT_IEEE802154_MODESWITCH_START:
      //return RAIL_EVENT_IEEE802154_MODESWITCH_START_TEXT;
      break;
    case RAIL_EVENT_IEEE802154_MODESWITCH_END:
      //return RAIL_EVENT_IEEE802154_MODESWITCH_END_TEXT;
      break;
    case RAIL_EVENT_DETECT_RSSI_THRESHOLD:
      //return RAIL_EVENT_DETECT_RSSI_THRESHOLD_TEXT;
      break;
    case RAIL_EVENT_THERMISTOR_DONE:
      //return RAIL_EVENT_THERMISTOR_DONE_TEXT;
      break;
    case RAIL_EVENT_TX_BLOCKED_TOO_HOT:
      //return RAIL_EVENT_TX_BLOCKED_TOO_HOT_TEXT;
      break;
    case RAIL_EVENT_TEMPERATURE_TOO_HOT:
      //return RAIL_EVENT_TEMPERATURE_TOO_HOT_TEXT;
      break;
    case RAIL_EVENT_TEMPERATURE_COOL_DOWN:
      //return RAIL_EVENT_TEMPERATURE_COOL_DOWN_TEXT;
      break;
  }
}
*/

// -----------------------------------------------------------------------------
//                     RAIL Event String Helpers End
// -----------------------------------------------------------------------------
