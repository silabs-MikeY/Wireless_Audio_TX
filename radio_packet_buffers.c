#include "radio_packet_buffers.h"

#include "radio/radio_base.h"
#include "scheduler.h"

#include <assert.h>
#include <string.h>

bool radio__try_to_send_a_packet_by_index(uint32_t index_to_send);

packet_buffer_t radio_tx_packet_buffer[NUMBER_OF_PACKET_BUFFERS];
uint32_t packet_buffer_head;

__attribute__((weak)) void radio_packet_buffers__printf(bool add_timestamp, const char *format, ...)
{
  (void)add_timestamp;
  (void)format;
}

void radio_packet_buffers__init(void)
{
  memset(radio_tx_packet_buffer, 0, sizeof(radio_tx_packet_buffer));
  packet_buffer_head = 0;
}

const packet_buffer_t *radio_packet_buffers__get_packet_buffer(uint32_t packet_buffer_index)
{
  if (packet_buffer_index >= NUMBER_OF_PACKET_BUFFERS)
  {
    return NULL;
  }

  return &radio_tx_packet_buffer[packet_buffer_index];
}

void radio_packet_buffers__get_oldest_packet_to_send(packet_buffer_t **packet_buffer, uint32_t *packet_buffer_index)
{
  uint32_t oldest_timestamp = 0xFFFFFFFF;
  uint32_t oldest_timestamp_index = 0xFFFFFFFF;
  for (uint32_t i = 0; i < NUMBER_OF_PACKET_BUFFERS; i++)
  {
    if ((radio_tx_packet_buffer[i].waiting_to_be_sent == true) && (radio_tx_packet_buffer[i].send_attempted == false))
    {
      if (radio_tx_packet_buffer[i].micros_timestamp_added_to_buffer < oldest_timestamp)
      {
        oldest_timestamp_index = i;
        oldest_timestamp = radio_tx_packet_buffer[i].micros_timestamp_added_to_buffer;
      }
    }
  }

  if (oldest_timestamp_index < NUMBER_OF_PACKET_BUFFERS)
  {
    *packet_buffer = &radio_tx_packet_buffer[oldest_timestamp_index];
    *packet_buffer_index = oldest_timestamp_index;
  }
  else
  {
    *packet_buffer = NULL;
    *packet_buffer_index = 0xFFFFFFFF;
  }
}

static bool radio_packet_buffers__is_buffer_available(const packet_buffer_t *packet_buffer)
{
  return (packet_buffer->used == false);
}

bool radio_packet_buffers__request_available_packet_buffer(packet_buffer_t **packet_buffer, uint32_t *packet_buffer_index)
{
  if ((packet_buffer == NULL) || (packet_buffer_index == NULL))
  {
    return false;
  }

  for (uint32_t buffer_index = 0; buffer_index < NUMBER_OF_PACKET_BUFFERS; buffer_index++)
  {
    packet_buffer_t *candidate_packet_buffer = &radio_tx_packet_buffer[buffer_index];

    if (radio_packet_buffers__is_buffer_available(candidate_packet_buffer) == false)
    {
      continue;
    }

    memset(candidate_packet_buffer, 0, sizeof(packet_buffer_t));

    *packet_buffer = candidate_packet_buffer;
    *packet_buffer_index = buffer_index;
    return true;
  }

  *packet_buffer = NULL;
  *packet_buffer_index = 0xFFFFFFFF;
  return false;
}

bool radio_packet_buffers__mark_packet_buffer_used(uint32_t packet_buffer_index)
{
  if (packet_buffer_index >= NUMBER_OF_PACKET_BUFFERS)
  {
    return false;
  }

  if (radio_tx_packet_buffer[packet_buffer_index].used == true)
  {
    radio_packet_buffers__printf(true, "Error: Attempting to mark a packet buffer as used that is already marked as used. Index: %u\n", (unsigned int)packet_buffer_index);
    assert(0);
    return false;
  }

  radio_tx_packet_buffer[packet_buffer_index].payload.header.sequence_number = radio__get_sequence_number();
  radio__increment_sequence_number();

  radio_tx_packet_buffer[packet_buffer_index].micros_timestamp_added_to_buffer = scheduler__get_microsecond_ticks();
  radio_tx_packet_buffer[packet_buffer_index].used = true;
  radio_tx_packet_buffer[packet_buffer_index].waiting_to_be_sent = true;
  radio_tx_packet_buffer[packet_buffer_index].send_attempted = false;
  radio_tx_packet_buffer[packet_buffer_index].tx_processed = false;

  return true;
}

bool radio_packet_buffers__mark_packet_buffer_send_attempted(uint32_t packet_buffer_index)
{
  if (packet_buffer_index >= NUMBER_OF_PACKET_BUFFERS)
  {
    return false;
  }

  if (radio_tx_packet_buffer[packet_buffer_index].used == false)
  {
    return false;
  }

  radio_tx_packet_buffer[packet_buffer_index].waiting_to_be_sent = false;
  radio_tx_packet_buffer[packet_buffer_index].send_attempted = true;

  return true;
}

bool radio_packet_buffers__mark_packet_buffer_completed(uint32_t packet_buffer_index)
{
  if (packet_buffer_index >= NUMBER_OF_PACKET_BUFFERS)
  {
    return false;
  }

  if (radio_tx_packet_buffer[packet_buffer_index].used == false)
  {
    return false;
  }

  radio_tx_packet_buffer[packet_buffer_index].tx_processed = true;
  radio_tx_packet_buffer[packet_buffer_index].micros_timestamp_packet_sent = scheduler__get_microsecond_ticks();
  radio_tx_packet_buffer[packet_buffer_index].used = false;

  return true;
}

bool radio_packet_buffers__mark_packet_buffer_failed(uint32_t packet_buffer_index)
{
  if (packet_buffer_index >= NUMBER_OF_PACKET_BUFFERS)
  {
    return false;
  }

  if (radio_tx_packet_buffer[packet_buffer_index].used == false)
  {
    return false;
  }

  radio_tx_packet_buffer[packet_buffer_index].waiting_to_be_sent = false;
  radio_tx_packet_buffer[packet_buffer_index].tx_processed = true;
  radio_tx_packet_buffer[packet_buffer_index].used = false;

  return true;
}