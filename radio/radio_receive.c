#include "radio_receive.h"

#include "radio_base.h"
#include "radio_retry.h"
#include "print.h"

#include <assert.h>

void radio__process_receiver_packet(RAIL_Handle_t rail_handle, RAIL_Events_t events);

volatile uint8_t radio_rx_fifo[RADIO_FIFO_SIZE] __attribute__((aligned(4)));
uint16_t radio_rx_fifo_size = RADIO_FIFO_SIZE;

/**
 * @brief Initializes the radio RX FIFO.
 * Allocates RX FIFO buffer and configures RAIL to receive into it.
 * Must be called once at startup. Asserts on failure.
 * 
 * @param None
 * @return void
 */
void radio_receive__init(void)
{
    RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);

    RAIL_Status_t RAIL_SetRxFifo_return = RAIL_SetRxFifo(rail_handle, (uint8_t *)radio_rx_fifo, &radio_rx_fifo_size);
    if (RAIL_SetRxFifo_return != RAIL_STATUS_NO_ERROR)
    {
        debug__printf_to_buf_append_time(0,"RAIL_SetRxFifo failed, Status Code: %X\n", (unsigned int)RAIL_SetRxFifo_return);
        assert(0);
    }
    if (radio_rx_fifo_size != RADIO_FIFO_SIZE)
    {
        debug__printf_to_buf_append_time(0,"RAIL_SetRxFifo Failed. Requested : %u Bytes, Got %u Bytes \n", (unsigned int)RADIO_FIFO_SIZE, (unsigned int)radio_rx_fifo_size);
        assert(0);
    }
    debug__printf_to_buf_append_time(0,"RAIL_SetRxFifo Success, %u Bytes\n", (unsigned int)radio_rx_fifo_size);
}

/**
 * @brief Handles RX completion events from the RAIL radio driver.
 * Processes RAIL_EVENT_RX_PACKET_RECEIVED (success) and various RX error events.
 * Routes packet processing and error logging. Returns true if event was handled.
 * 
 * @param rail_handle - RAIL driver handle
 * @param events - Bitmask of RAIL RX events from the radio driver
 * @return true if RX completion event was handled, false otherwise
 */
bool radio__process_event_rx(RAIL_Handle_t rail_handle, RAIL_Events_t events)
{
    (void)rail_handle;
    (void)events;
    if (events & RAIL_EVENTS_RX_COMPLETION)
    {
        // debug__audio_buffer_printf_to_buf_append_time(0,"Got RX \n");
        //      if (logged_events_count < LOGGED_EVENTS_BUFFER_SIZE)
        //        {
        //          logged_events[logged_events_count++] = events;
        //        }
        // printf_to_buf_append_time(0,"%s\n", getString(events));
        if (events & RAIL_EVENT_RX_PACKET_RECEIVED)
        {
            radio__process_receiver_packet(rail_handle, events);
            // process_packet(rail_handle);
        }
        if (events & RAIL_EVENT_RX_PACKET_ABORTED)
        {
            // debug_increment_number_of_RX_abort();
        }
        if (events & RAIL_EVENT_RX_FRAME_ERROR)
        {
            // debug_increment_number_of_RX_error();
        }
        if (events & RAIL_EVENT_RX_FIFO_OVERFLOW)
        {
            // debug_increment_number_of_RX_overflow();
        }
        if (events & RAIL_EVENT_RX_ADDRESS_FILTERED)
        {
            // debug_increment_number_of_RX_filterred();
        }
        if (events & RAIL_EVENT_RX_SCHEDULED_RX_MISSED)
        {
            // debug_increment_number_of_RX_missed();
        }
        return true;
    }
    return false;
}

/**
 * @brief Processes a received radio packet.
 * Extracts packet data from RAIL, interprets it as a request for missing packets,
 * and adds those sequence numbers to the retry list. Typically used by RX board
 * to fulfill retransmission requests from TX board.
 * 
 * @param rail_handle - RAIL driver handle
 * @param events - RAIL events (currently unused)
 * @return void
 */
void radio__process_receiver_packet(RAIL_Handle_t rail_handle, RAIL_Events_t events)
{
    (void)rail_handle;
    (void)events;

    RAIL_RxPacketHandle_t rx_packet_handle;
    RAIL_RxPacketInfo_t packet_info;
    //  uint8_t *start_of_packet = 0;

    rx_packet_handle = RAIL_GetRxPacketInfo(rail_handle, RAIL_RX_PACKET_HANDLE_OLDEST_COMPLETE, &packet_info);
    (void)rx_packet_handle;

    payload_t temp;

    RAIL_CopyRxPacket((uint8_t *)&temp, &packet_info);

    if (temp.header.size > RADIO_PACKET_DATA_SIZE_PER_CHANNEL)
    {
        debug__printf_to_buf_append_time(0,"RX request size too large: %u\n", (unsigned int)temp.header.size);
        (void) RAIL_ReleaseRxPacket(rail_handle, rx_packet_handle);
        return;
    }

    // debug__audio_buffer_printf_to_buf_append_time(0,"Packet : %u\n");
    // for (uint32_t i = 0; i < temp.header.size + 4 + 1; i++)
    // {
    //   debug__audio_buffer_printf_to_buf_append_time(0,"0x%X, ", (unsigned int)((uint8_t *)(&temp))[i]);
    // }
    uint32_t number_of_missing_packets_requested = (temp.header.size + 1) >> 1;

    // debug__audio_buffer_printf_to_buf_append_time(0,"\nNumber Sequence Numbers Requested : %u\n", number_of_missing_packets_requested);
    for (uint32_t i = 0; i < number_of_missing_packets_requested; i++)
    {
        uint16_t missing_sequence_number = (temp.data_left[(i * 2)] << 8) | (temp.data_left[(i * 2) + 1]);
        radio_retry__add_missing_packet_entry(missing_sequence_number);
        DEBUG_RETRY_LOG(debug__printf_to_buf_append_time(0,"%u - Got Request For Sequence Number : %u\n", i, missing_sequence_number));
    }

    (void) RAIL_ReleaseRxPacket(rail_handle, rx_packet_handle);
}