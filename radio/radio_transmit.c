#include "radio_transmit.h"
#include "radio_base.h"
#include "radio_packet_buffers.h"
#include "radio_packet_format.h"

#include <string.h>

typedef enum radio_transmit_counter_index_s {
    number_of_TX_attempts = 0,
    number_of_TX_attempt_success,
    number_of_TX_attempt_failed,
    number_of_TX_success,
    number_of_TX_fail_failed_to_send,
    number_of_TX_fail_abort,
    number_of_TX_fail_block,
    number_of_TX_fail_underflow,
    number_of_TX_fail_busy,
    number_of_TX_fail_missed,
    number_of_TX_retry_attempts,
    number_of_TX_retry_attempt_success,
    number_of_TX_retry_attempt_failed,
    number_of_TX_retry_success,
    number_of_TX_retry_fail_failed_to_send,
    number_of_TX_retry_fail_abort,
    number_of_TX_retry_fail_block,
    number_of_TX_retry_fail_underflow,
    number_of_TX_retry_fail_busy,
    number_of_TX_retry_fail_missed,
    RADIO_TRANSMIT_NUMBER_OF_COUNTERS
} radio_transmit_counter_index_t;

static volatile uint32_t radio_transmit_counter_values[RADIO_TRANSMIT_NUMBER_OF_COUNTERS] = {0};
static const char *radio_transmit_counter_names[RADIO_TRANSMIT_NUMBER_OF_COUNTERS] = {
    "number_of_TX_attempts",
    "number_of_TX_attempt_success",
    "number_of_TX_attempt_failed",
    "number_of_TX_success",
    "number_of_TX_fail_failed_to_send",
    "number_of_TX_fail_abort",
    "number_of_TX_fail_block",
    "number_of_TX_fail_underflow",
    "number_of_TX_fail_busy",
    "number_of_TX_fail_missed",
    "number_of_TX_retry_attempts",
    "number_of_TX_retry_attempt_success",
    "number_of_TX_retry_attempt_failed",
    "number_of_TX_retry_success",
    "number_of_TX_retry_fail_failed_to_send",
    "number_of_TX_retry_fail_abort",
    "number_of_TX_retry_fail_block",
    "number_of_TX_retry_fail_underflow",
    "number_of_TX_retry_fail_busy",
    "number_of_TX_retry_fail_missed",
};

static bool radio_transmit__send_packet(uint32_t buffer_index, bool retry);
static void radio_transmit__increment_counter(uint32_t counter_index);

uint8_t radio_tx_fifo[RADIO_FIFO_SIZE] __attribute__((aligned(4)));
uint32_t radio_tx_fifo_Size = 0;

typedef struct tx_packet_in_flight_info_s
{
    uint32_t packet_buffer_index;
    bool retry;
    bool in_flight;
} tx_packet_in_flight_info_t;
tx_packet_in_flight_info_t tx_packet_in_flight_info;

uint32_t radio_transmit__get_number_of_counters(void)
{
    return RADIO_TRANSMIT_NUMBER_OF_COUNTERS;
}

const char *radio_transmit__get_counter_name(uint32_t counter_index)
{
    if (counter_index >= RADIO_TRANSMIT_NUMBER_OF_COUNTERS)
    {
        return NULL;
    }

    return radio_transmit_counter_names[counter_index];
}

volatile uint32_t *radio_transmit__get_counter_address(uint32_t counter_index)
{
    if (counter_index >= RADIO_TRANSMIT_NUMBER_OF_COUNTERS)
    {
        return NULL;
    }

    return &radio_transmit_counter_values[counter_index];
}

void radio_transmit__reset_counters(void)
{
    memset((void *)radio_transmit_counter_values, 0, sizeof(radio_transmit_counter_values));
}

static void radio_transmit__increment_counter(uint32_t counter_index)
{
    if (counter_index < RADIO_TRANSMIT_NUMBER_OF_COUNTERS)
    {
        radio_transmit_counter_values[counter_index]++;
    }
}

void radio__set_control_bit(uint8_t *control_bits, uint8_t bit)
{
    *control_bits |= bit;
}
void radio__reset_control_bit(uint8_t *control_bits, uint8_t bit)
{
    *control_bits &= bit;
}

/**
 * @brief Resets all TX packet buffers and related state to initial values.
 * Clears all buffer data, resets head pointer to 0, and clears in-flight packet info.
 * Called during initialization or to recover from error state.
 * Ensures clean slate before starting TX operations.
 *
 * @param None
 * @return void
 */
void radio_transmit__buffers_reset(void)
{
    memset(&tx_packet_in_flight_info, 0, sizeof(tx_packet_in_flight_info_t));
}

/**
 * @brief Initializes the radio TX system.
 * Resets all packet buffers, configures RAIL events, allocates TX FIFO memory,
 * and sets TX power to 19 dBm (190 in 0.1 dBm units).
 * Must be called once at startup before any TX operations.
 * Asserts on failure if FIFO allocation or power configuration fails.
 *
 * @param None
 * @return void
 */
void radio_transmit__init(void)
{
    radio_transmit__reset_counters();
    radio_transmit__buffers_reset();

    RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);

    RAIL_ConfigEvents(rail_handle, RAIL_EVENTS_ALL, RAIL_EVENTS_ALL);

    uint32_t RAIL_SetTxFifo_return = RAIL_SetTxFifo(rail_handle, (uint8_t *)radio_tx_fifo, 0, RADIO_FIFO_SIZE);
    if (RAIL_SetTxFifo_return < RADIO_PAYLOAD_SIZE)
    {
        radio__printf(true, "RAIL_SetTxFifo Failed. Requested : %u Bytes, Got %u Bytes \n", (unsigned int)RADIO_FIFO_SIZE, (unsigned int)RAIL_SetTxFifo_return);
        assert(0);
    }
    radio__printf(true, "RAIL_SetTxFifo Success, %u Bytes\n", (unsigned int)RAIL_SetTxFifo_return);
    radio_tx_fifo_Size = RAIL_SetTxFifo_return;

    RAIL_Status_t RAIL_SetTxPowerDbm_return = RAIL_SetTxPowerDbm(rail_handle, 190);
    if (RAIL_SetTxPowerDbm_return != RAIL_STATUS_NO_ERROR)
    {
        radio__printf(true, "RAIL_SetTxPowerDbm failed, Status Code: %X\n", (unsigned int)RAIL_SetTxPowerDbm_return);
        assert(0);
    }
    radio__printf(true, "RAIL_SetTxPowerDbm Success\n");
}

__attribute__((weak)) void radio_transmit__handle_successful_packet_sent(uint32_t packet_buffer_index)
{
    (void)packet_buffer_index;
}

/**
 * @brief Core function that loads a packet into the RAIL TX FIFO and initiates transmission.
 * Writes header, left channel data, right channel data to FIFO sequentially.
 * Tracks in-flight packet info and updates debug counters for TX attempts/failures.
 * Sets CONTROL_BITS__RETRY flag if this is a retry attempt.
 *
 * @param buffer_index - Index of the packet buffer to transmit
 * @param retry - true if this is a retry attempt, false for first-time send
 * @return true if RAIL_StartTx() succeeded, false otherwise
 */
static bool radio_transmit__send_packet(uint32_t buffer_index, bool retry)
{
    if (buffer_index >= NUMBER_OF_PACKET_BUFFERS)
    {
        radio__printf(true, "Buffer Index Too High\n");
        return false;
    }

    //  printf_to_buf_append_time(0,"Trying to Send buffer %u - Sequence : %u\n", (unsigned int)buffer_index, (unsigned int)radio_tx_packet_buffer[buffer_index].payload.header.sequence_number);

    RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
    bool return_value = false;

    header_t new_header_copy;
    memcpy(&new_header_copy, &(radio_tx_packet_buffer[buffer_index].payload.header), sizeof(header_t));

    if (retry == true)
    {
        new_header_copy.control_bits |= CONTROL_BITS__RETRY;
    }
    new_header_copy.size = 255;

    uint32_t return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(new_header_copy)), RADIO_PACKET_HEADER_SIZE, true);
    if (return_count != RADIO_PACKET_HEADER_SIZE)
    {
        radio__printf(true, "Write Header To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
        // radio__error_handle();
        assert(0);
        return false;
    }
    return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(radio_tx_packet_buffer[buffer_index].payload.data_left)), RADIO_PACKET_DATA_SIZE_PER_CHANNEL, false);
    if (return_count != RADIO_PACKET_DATA_SIZE_PER_CHANNEL)
    {
        radio__printf(true, "Write Left Data To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
        // radio__error_handle();
        assert(0);
        return false;
    }
    return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(radio_tx_packet_buffer[buffer_index].payload.data_right)), RADIO_PACKET_DATA_SIZE_PER_CHANNEL, false);
    if (return_count != RADIO_PACKET_DATA_SIZE_PER_CHANNEL)
    {
        radio__printf(true, "Write Right Data To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
        // radio__error_handle();
        assert(0);
        return false;
    }

    if (radio_packet_buffers__mark_packet_buffer_send_attempted(buffer_index) == false)
    {
        return false;
    }
    // printf_to_buf_append_time(0,"Loaded to Buffer Successfully\n");

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    radio_transmit__increment_counter(number_of_TX_attempts);
    if (retry)
    {
        radio_transmit__increment_counter(number_of_TX_retry_attempts);
    }

    RAIL_Status_t RAIL_StartTx_return = RAIL_StartTx(rail_handle, radio__get_channel(), RAIL_TX_OPTIONS_DEFAULT, NULL);
    if (RAIL_StartTx_return == SL_STATUS_OK)
    {
        tx_packet_in_flight_info.packet_buffer_index = buffer_index;
        tx_packet_in_flight_info.retry = retry;
        tx_packet_in_flight_info.in_flight = true;
        // radio_transmit__mark_packet_as_successfully_sent(buffer_index);
        if (retry)
        {
            radio_transmit__increment_counter(number_of_TX_retry_attempt_success);
        }
        else
        {
            radio_transmit__increment_counter(number_of_TX_attempt_success);
        }
        //      printf_to_buf_append_time(0,"Time: %u - ", (unsigned int)get_millisecond_ticks());
        // printf_to_buf_append_time(0,"Sent buffer %u , sequence : %u\n", (unsigned int)buffer_index, radio_tx_packet_buffer[buffer_index].payload.header.sequence_number);
        return_value = true;
    }
    else
    {
        radio_transmit__increment_counter(number_of_TX_attempt_failed);
        radio_packet_buffers__mark_packet_buffer_failed(buffer_index);
        //      printf_to_buf_append_time(0,"Time: %u - ", (unsigned int)get_millisecond_ticks());
        // printf_to_buf_append_time(0,"Failed to Send buffer %u , sequence : %u, status: 0x%X\n", (unsigned int)buffer_index, radio_tx_packet_buffer[buffer_index].payload.header.sequence_number, (unsigned int)RAIL_StartTx_return);
        CORE_EXIT_CRITICAL();
        return false;
    }
    CORE_EXIT_CRITICAL();

    // printf_to_buf_append_time(0,"Bytes Loaded: %u\n", (unsigned int)bytes_loaded);

    return return_value;
}

/**
 * @brief Wrapper function to send a packet by index (non-retry).
 * Calls radio_transmit__send_packet() with retry=false to indicate first-time send.
 *
 * @param index_to_send - Index of the packet buffer to transmit
 * @return true if transmission initiated successfully, false otherwise
 */
bool radio__try_to_send_a_packet_by_index(uint32_t index_to_send)
{
    return radio_transmit__send_packet(index_to_send, false);
}

bool radio__send_packet_by_sequence_number(uint16_t sequence_number, bool retry)
{
    for (uint32_t i = 0; i < NUMBER_OF_PACKET_BUFFERS; i++)
    {
        if (radio_tx_packet_buffer[i].used == true && radio_tx_packet_buffer[i].payload.header.sequence_number == sequence_number)
        {
            return radio_transmit__send_packet(i, retry);
        }
    }

    radio__printf(true, "Sequence Number Not Found: %u\n", (unsigned int)sequence_number);
    return false;
}

/**
 * @brief Main TX process loop called repeatedly from scheduler/state machine.
 * Checks if radio is idle, finds oldest packet waiting to send, and initiates transmission.
 * Returns true if a packet was sent, false otherwise.
 *
 * @param None
 * @return true if a packet was successfully sent this call, false if radio busy or no packets ready
 */
bool radio_transmit__run_process(void)
{
    // if (radio_retry__run_process())
    // {
    //     return true;
    // }

    if (radio__is_radio_busy())
    {
        return false;
    }

    packet_buffer_t *packet_buffer = NULL;
    uint32_t packet_buffer_index = 0xFFFFFFFF;

    radio_packet_buffers__get_oldest_packet_to_send(&packet_buffer, &packet_buffer_index);

    if (packet_buffer != NULL)
    {
        if (radio__try_to_send_a_packet_by_index(packet_buffer_index))
        {
            // printf_to_buf_append_time(0,"Sending Packet : %u\n", packet_buffer_index);

            return true;
        }
    }

    return false;
}
// uint8_t* get_next_data_buffer(bool left_or_right_data)
// {
//     if ()

//     // if ((radio_tx_packet_buffer[packet_buffer_head].used != true))
//     // {
//     //     // Creating First Buffer
//     //     DEBUG_PACKETS_LOG(printf_to_buf_append_time(0,"Creating First Buffer\n"));

//     //     uint32_t new_buffer_index = radio__create_new_packet_buffer();
//     //     DEBUG_PACKETS_LOG(debug__audio_buffer_printf_to_buf_append_time(0,"Making New Buffer %u\n", (unsigned int)new_buffer_index));
//     // }

//     // if ((radio_tx_packet_buffer[packet_buffer_head].left_data_present == false) && (left_or_right_data == LEFT))
//     // {
//     //     return (&(radio_tx_packet_buffer[packet_buffer_head].payload.data_left[0]));
//     // }
//     // else if ((radio_tx_packet_buffer[packet_buffer_head].right_data_present == false) && (left_or_right_data == RIGHT))
//     // {
//     //     return (&(radio_tx_packet_buffer[packet_buffer_head].payload.data_right[0]));
//     // }
// }

// if it gets here, the buffer isn't free
// check if the other buffer has a free slot

// if it gets here, the other channel's buffer isn't free
// make a new buffer

//   if (radio_tx_packet_buffer[packet_buffer_head].used == true)
//   {
//     if ((radio_tx_packet_buffer[packet_buffer_head].waiting_to_be_sent == true) && (radio_packet_buffer[packet_buffer_head].sent == false))
//     {
//       debug__increment_packet_buffer_overflows();
//       // TODO Handle overflow
//       DEBUG_STATES_LOG(printf_to_buf_append_time(0,"Forcing Error Due To Oerflows\n"));
//       state_machine__force_state_machine_error();
//     }
//     else if ((radio_packet_buffer[packet_buffer_head].waiting_to_be_sent == false) && (radio_packet_buffer[packet_buffer_head].sent == true))
//     {
//       // This should be impossible, somethign seriously wrong. Throw error
//     }
//     DEBUG_PACKETS_LOG(debug__audio_buffer_printf_to_buf_append_time(0,"No Empty Buffer, Overwriting for Sequence: %u\n", (unsigned int)next_sequence_number));

//     memset(&(radio_packet_buffer[packet_buffer_head]), 0, sizeof(packet_buffer_t));
//   }

//   uint32_t return_index = packet_buffer_head++;
//   radio_packet_buffer[return_index].payload.header.sequence_number = next_sequence_number;
//   debug__set_end_sequence_number(next_sequence_number);
//   increment_sequence_number();
//   radio_packet_buffer[return_index].used = true;

//   if (packet_buffer_head >= NUMBER_OF_PACKET_BUFFERS)
//   {
//     packet_buffer_head = 0;
//   }
//   return return_index;
// }

// bool radio__send_packet_by_sequence_number(uint16_t sequence_number, bool retry)
// {
//   uint32_t buffer_index = radio__find_packet_buffer_index_by_sequence_number(sequence_number);
//   if (buffer_index > NUMBER_OF_PACKET_BUFFERS)
//   {
//     printf_to_buf_append_time(0,"Buffer Index Too High\n");
//     return false;
//   }

//   // printf_to_buf_append_time(0,"Sending Packet : %u, Index : %u\n", (unsigned int)radio_packet_buffer[buffer_index].payload.header.sequence_number, (unsigned int)buffer_index);

//   radio__send_packet_new(buffer_index,retry);
// }

// static bool radio__send_packet(uint32_t index_to_send)
// {
//   uint32_t bytes_loaded = 0;

//   if (index_to_send >= NUMBER_OF_PACKET_BUFFERS)
//   {
//     printf_to_buf_append_time(0,"Index To Send Too High\n");
//     return false;
//   }

//   if (radio_packet_buffer[index_to_send].used == false)
//   {
//     printf_to_buf_append_time(0,"Packet Buffer Not Used\n");
//     return false;
//   }

//   if (radio_packet_buffer[index_to_send].waiting_to_be_sent == false)
//   {
//     printf_to_buf_append_time(0,"Packet Buffer Not Ready To Send\n");
//     return false;
//   }

//   if (radio_packet_buffer[index_to_send].sent == true)
//   {
//     printf_to_buf_append_time(0,"Packet Buffer Already Sent\n");
//     return false;
//   }

//   // Set Stereo or Mono Control Bit
//   if (stereo_or_mono_config->stereo_mode == true)
//   {
//     radio__set_control_bit(&radio_packet_buffer[index_to_send].payload.header.control_bits, CONTROL_BITS__STEREO);
//     // printf_to_buf_append_time(0,"Sending Stereo Packet\n");
//   }
//   else
//   {
//     radio__clear_control_bit(&radio_packet_buffer[index_to_send].payload.header.control_bits, CONTROL_BITS__STEREO);
//     // printf_to_buf_append_time(0,"Sending Mono Packet\n");
//   }

/**
 * @brief Handles TX completion events from the RAIL radio driver.
 * Processes RAIL_EVENT_TX_PACKET_SENT (success) and various TX error events
 * (abort, block, underflow, missed, busy).
 * Updates debug counters based on success/failure and retry status.
 * Marks packet as successfully sent and clears in-flight flag on success.
 * On failure, logs the specific error type and updates corresponding failure counters.
 *
 * @param rail_handle - RAIL driver handle (currently unused)
 * @param events - Bitmask of RAIL TX events from the radio driver
 * @return true if event was handled, false otherwise
 */
bool radio__process_event_tx(RAIL_Handle_t rail_handle, RAIL_Events_t events)
{
    (void)rail_handle;
    (void)events;

    bool retry = tx_packet_in_flight_info.retry;
    if (events & RAIL_EVENT_TX_PACKET_SENT)
    {
        // GPIO_PinOutToggle(gpioPortB, 3);
        // set_rgb(1, green, 0, 127);

        // radio__mark_packet_as_sent();
        // radio__note_successful_tx();

        //     RAIL_TxPacketDetails_t temp_RAIL_TxPacketDetails_t;

        //     RAIL_Status_t RAIL_GetTxPacketDetails_status = RAIL_GetTxPacketDetails(rail_handle,&temp_RAIL_TxPacketDetails_t);
        //     if (RAIL_GetTxPacketDetails_status != RAIL_STATUS_NO_ERROR)
        //     {
        //       printf_to_buf_append_time(0,"TX_ERROR??? : 0x%X\n",RAIL_GetTxPacketDetails_status);
        //     }
        // else
        // {
        //   uint32_t bytes_sent = temp_RAIL_TxPacketDetails_t.timeSent.totalPacketBytes;
        //     printf_to_buf_append_time(0,"Bytes Sent: %u\n", (unsigned int)bytes_sent);
        // }

        // uint32_t bytes_remaining = FIFO_Size - RAIL_GetTxFifoSpaceAvailable(rail_handle);
        //  printf_to_buf_append_time(0,"Fifo Size: %u , Available : %u , Bytes Remaining: %u , Bytes Sent %u\n",(unsigned int)FIFO_Size,(unsigned int)RAIL_GetTxFifoSpaceAvailable(rail_handle), (unsigned int)bytes_remaining, (unsigned int)(RADIO_PAYLOAD_SIZE - bytes_remaining));

        radio_transmit__handle_successful_packet_sent(tx_packet_in_flight_info.packet_buffer_index);

        if (retry)
        {
            radio_transmit__increment_counter(number_of_TX_retry_success);
        }
        else
        {
            radio_transmit__increment_counter(number_of_TX_success);
        }

        tx_packet_in_flight_info.in_flight = false;
        return true;
        // radio__try_to_send_a_packet();
    }
    else
    {
        if (events & RAIL_EVENTS_TX_COMPLETION)
        {
            tx_packet_in_flight_info.in_flight = false;
            radio_packet_buffers__mark_packet_buffer_failed(tx_packet_in_flight_info.packet_buffer_index);
            // debug__log_TX_fail(tx_packet_in_flight_info.retry);
            if (retry)
            {
                radio_transmit__increment_counter(number_of_TX_retry_attempt_failed);
            }
            else
            {
                radio_transmit__increment_counter(number_of_TX_attempt_failed);
            }
            if (events & RAIL_EVENT_TX_ABORTED)
            {
                // debug__log_TX_abort(tx_packet_in_flight_info.retry);
                if (retry)
                {
                    radio_transmit__increment_counter(number_of_TX_retry_fail_abort);
                }
                else
                {
                    radio_transmit__increment_counter(number_of_TX_fail_abort);
                }
            }
            else if (events & RAIL_EVENT_TX_BLOCKED)
            {
                // debug__log_TX_block(tx_packet_in_flight_info.retry);
                if (retry)
                {
                    radio_transmit__increment_counter(number_of_TX_retry_fail_block);
                }
                else
                {
                    radio_transmit__increment_counter(number_of_TX_fail_block);
                }
            }
            else if (events & RAIL_EVENT_TX_UNDERFLOW)
            {
                // debug__log_TX_underflow(tx_packet_in_flight_info.retry);
                if (retry)
                {
                    radio_transmit__increment_counter(number_of_TX_retry_fail_underflow);
                }
                else
                {
                    radio_transmit__increment_counter(number_of_TX_fail_underflow);
                }
            }
            else if (events & RAIL_EVENT_TX_SCHEDULED_TX_MISSED)
            {
                // debug__log_TX_missed(tx_packet_in_flight_info.retry);
                if (retry)
                {
                    radio_transmit__increment_counter(number_of_TX_retry_fail_missed);
                }
                else
                {
                    radio_transmit__increment_counter(number_of_TX_fail_missed);
                }
            }
            else if (events & RAIL_EVENT_TX_CHANNEL_BUSY)
            {
                // debug__log_TX_busy(tx_packet_in_flight_info.retry);
                if (retry)
                {
                    radio_transmit__increment_counter(number_of_TX_retry_fail_busy);
                }
                else
                {
                    radio_transmit__increment_counter(number_of_TX_fail_busy);
                }
            }
            else
            {
                radio__printf(true, "UNHANDLED TX ERROR: 0x%u:\n", (long long unsigned int)events);
                // if (retry)
                // {
                //     counters__increment_counter();
                // }
                // else
                // {
                //     counters__increment_counter();
                // }
            }
            return true;
        }
    }
    return false;
}

// bool radio__note_successful_tx(void)
// {
//   if (number_of_TX_packets_sent_this_second != 0)
//   {
//     // First TX this measurement period
//     uint32_t current_time = scheduler__get_microsecond_ticks();
//     uint32_t tx_delta = current_time - last_tx_timestamp_micros;

//     if (tx_delta > stereo_or_mono_config->acceptable_tx_microsecond_delta_upper)
//     {
//       // max_tx_timestamp_delta = tx_delta;
//       //  printf_to_buf_append_time(0,"-  Max Delta Violation: %u  Max: %u\n", (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
//       bad_timestamps[bad_timestamps_count].sequence_number = next_sequence_number;
//       bad_timestamps[bad_timestamps_count].timestamp_delta_from_previous = tx_delta;
//       bad_timestamps[bad_timestamps_count].tx_timestamp = current_time;
//       bad_timestamps_count++;
//       // printf_to_buf_append_time(0,"-  New Max Delta: %u \n", (unsigned int)max_tx_timestamp_delta);
//     }
//     else if (tx_delta < stereo_or_mono_config->acceptable_tx_microsecond_delta_lower)
//     {
//       // min_tx_timestamp_delta = tx_delta;
//       //  printf_to_buf_append_time(0,"-  Min Delta Violation: %u  Min: %u\n", (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
//       bad_timestamps[bad_timestamps_count].sequence_number = next_sequence_number;
//       bad_timestamps[bad_timestamps_count].timestamp_delta_from_previous = tx_delta;
//       bad_timestamps[bad_timestamps_count].tx_timestamp = current_time;
//       bad_timestamps_count++;
//       // printf_to_buf_append_time(0,"-  New Min Delta: %u \n", (unsigned int)min_tx_timestamp_delta);
//     }

//     if (bad_timestamps_count > BAD_TIMESTAMPS_SIZE)
//     {
//       printf_to_buf_append_time(0,"-  Bad Timestamps Limit Reached: \n");
//       for (uint32_t i = 0; i < bad_timestamps_count; i++)
//       {
//         printf_to_buf_append_time(0,"  %u -- Seq: %u - Delta: %u - Timestamp: %u\n",
//                (unsigned int)i,
//                (unsigned int)bad_timestamps[i].sequence_number,
//                (unsigned int)bad_timestamps[i].timestamp_delta_from_previous,
//                (unsigned int)bad_timestamps[i].tx_timestamp);
//       }
//       state_machine__force_state_machine_error();
//       // assert(0);
//     }

//     // if (tx_delta > max_tx_timestamp_delta)
//     // {
//     //   max_tx_timestamp_delta = tx_delta;
//     //   bad_timestamps[bad_timestamps_count].sequence_number = next_sequence_number;
//     //   bad_timestamps[bad_timestamps_count].timestamp_delta_from_previous = max_tx_timestamp_delta;
//     //   bad_timestamps[bad_timestamps_count].tx_timestamp = current_time;
//     //   bad_timestamps_count++;
//     //   // printf_to_buf_append_time(0,"-  New Max Delta: %u \n", (unsigned int)max_tx_timestamp_delta);
//     // }
//     // else if (tx_delta < min_tx_timestamp_delta)
//     // {
//     //   min_tx_timestamp_delta = tx_delta;
//     //   bad_timestamps[bad_timestamps_count].sequence_number = next_sequence_number;
//     //   bad_timestamps[bad_timestamps_count].timestamp_delta_from_previous = max_tx_timestamp_delta;
//     //   bad_timestamps[bad_timestamps_count].tx_timestamp = current_time;
//     //   bad_timestamps_count++;
//     //   // printf_to_buf_append_time(0,"-  New Min Delta: %u \n", (unsigned int)min_tx_timestamp_delta);
//     // }
//   }
//   else
//   {
//     // first measurement in series. Have nothing to compare it against
//     max_tx_timestamp_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
//     min_tx_timestamp_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
//     // printf_to_buf_append_time(0,"-  New Max Delta: %u. , New Min Delta : %u\n", (unsigned int)max_tx_timestamp_delta, (unsigned int)min_tx_timestamp_delta);
//   }
//   last_tx_timestamp_micros = scheduler__get_microsecond_ticks();
//   number_of_TX_packets_sent_this_second++;
// }