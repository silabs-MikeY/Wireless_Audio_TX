#include "radio_retry.h"

#include "radio_base.h"
#include "scheduler.h"

#include <string.h>

static void radio_retry__clean_missing_packet_list(void);
static uint32_t radio_retry__check_if_theres_a_missing_packet_to_send(void);

typedef struct missing_packet_list_entry_s
{
    uint32_t sequence_number;
    bool used;
    uint32_t timestamp_of_entry;
    uint32_t send_attempts;
    bool marked_for_delete;
} missing_packet_list_entry_t;
#define MISSING_PACKET_LIST_SIZE 100
missing_packet_list_entry_t missing_packet_list[MISSING_PACKET_LIST_SIZE];

/**
 * @brief Initializes the radio retry system.
 * Clears the missing packet list to prepare for tracking missing packets.
 * Must be called once at startup before any retry operations.
 * 
 * @param None
 * @return void
 */
bool radio_retry__init(void)
{
    memset(missing_packet_list, 0, sizeof(missing_packet_list));
    return true;
}

/**
 * @brief Adds a missing packet sequence number to the retry list.
 * Searches for the first unused entry in the list and records the sequence number,
 * timestamp, and marks it as in-use. Used when a packet gap is detected on RX side.
 * 
 * @param sequence_number - The sequence number of the missing packet to retry
 * @return void
 */
void radio_retry__add_missing_packet_entry(uint16_t sequence_number)
{
    for (uint32_t i = 0; i < MISSING_PACKET_LIST_SIZE; i++)
    {
        if (missing_packet_list[i].used == false)
        {
            missing_packet_list[i].sequence_number = sequence_number;
            missing_packet_list[i].used = true;
            missing_packet_list[i].timestamp_of_entry = scheduler__get_microsecond_ticks();
            return;
        }
    }
}

/**
 * @brief Cleans up expired or failed retry entries from the missing packet list.
 * Removes entries that have exceeded timeout (10ms), exceeded max send attempts (5),
 * or are marked for deletion. Called periodically before attempting retries.
 * 
 * @param None
 * @return void
 */
static void radio_retry__clean_missing_packet_list(void)
{
#define TIMEOUT_MILLISECONDS 10
#define TIMEOUT_MICROSECONDS (TIMEOUT_MILLISECONDS * 1000)
    uint32_t microseconds = scheduler__get_microsecond_ticks();
    // if (microseconds < TIMEOUT_MICROSECONDS)
    // {
    //     //avoid underflow in following code
    //     return;
    // }

    for (uint32_t i = 0; i < MISSING_PACKET_LIST_SIZE; i++)
    {
        if (missing_packet_list[i].used == true)
        {
            if (((microseconds - missing_packet_list[i].timestamp_of_entry) >= TIMEOUT_MICROSECONDS) ||
                (missing_packet_list[i].send_attempts >= 5) ||
                (missing_packet_list[i].marked_for_delete == true))
            {
                // printf_to_buf_append_time(0,"Erasing List Entry %u\n", (unsigned int)i);
                memset(&(missing_packet_list[i]), 0, sizeof(missing_packet_list_entry_t));
                continue;
            }
        }
    }
}

/**
 * @brief Searches for the oldest missing packet entry that is ready to be resent.
 * Scans the entire list to find the entry with the earliest timestamp,
 * implementing FIFO retry ordering. Returns 0xFFFFFFFF if no packets available.
 * 
 * @param None
 * @return Index of the oldest missing packet entry, or 0xFFFFFFFF if none found
 */
static uint32_t radio_retry__check_if_theres_a_missing_packet_to_send(void)
{
    // find oldest entry

    uint32_t oldest_missing_packet_index = 0xFFFFFFFF;
    uint32_t oldest_missing_packet_timestamp = 0xFFFFFFFF;
    for (uint32_t i = 0; i < MISSING_PACKET_LIST_SIZE; i++)
    {
        if (missing_packet_list[i].used == true)
        {
            if (missing_packet_list[i].timestamp_of_entry < oldest_missing_packet_timestamp)
            {
                oldest_missing_packet_index = i;
                oldest_missing_packet_timestamp = missing_packet_list[i].timestamp_of_entry;
            }
        }
    }
    return oldest_missing_packet_index;
}

/**
 * @brief Main retry process loop called from scheduler.
 * Cleans expired entries, finds oldest missing packet, and attempts transmission.
 * Increments send attempt counter and returns true if packet was sent.
 * 
 * @param None
 * @return true if a retry packet was sent, false if radio busy or no packets to retry
 */
bool radio_retry__run_process(void)
{
    if (!radio__is_radio_busy())
    {
        radio_retry__clean_missing_packet_list();

        uint32_t packet_to_send = radio_retry__check_if_theres_a_missing_packet_to_send();

        if (packet_to_send != 0xFFFFFFFF)
        {
            // printf_to_buf_append_time(0,"Resending Packet : %u, Index : %u,Attempts : %u\n", (unsigned int)missing_packet_list[packet_to_send].sequence_number, (unsigned int)packet_to_send, (unsigned int)missing_packet_list[packet_to_send].send_attempts);
            missing_packet_list[packet_to_send].send_attempts++;

            if (radio__send_packet_by_sequence_number(missing_packet_list[packet_to_send].sequence_number, true))
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Marks a successfully sent retry packet for removal from retry list.
 * Sets the marked_for_delete flag so the entry is cleaned up on next cleanup cycle.
 * Called after a retry packet transmission is confirmed successful.
 * 
 * @param sequence_number - Sequence number of the packet that was successfully resent
 * @return void
 */
void radio_retry__note_retry_packet_successfully_sent(uint16_t sequence_number)
{
    for (uint32_t i = 0; i < MISSING_PACKET_LIST_SIZE; i++)
    {
        if (missing_packet_list[i].sequence_number == sequence_number)
        {
            // printf_to_buf_append_time(0,"Marking Packet %u for Deletion from Retry List\n", (unsigned int)sequence_number);
            missing_packet_list[i].marked_for_delete = true;
            return;
        }
    }
}
