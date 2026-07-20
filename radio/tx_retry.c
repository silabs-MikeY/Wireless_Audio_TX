#include "tx_retry.h"

#include <string.h>

__attribute__((weak)) void tx_retry__printf(bool add_timestamp, const char *format, ...)
{
  (void)add_timestamp;
  (void)format;
}

static volatile uint32_t tx_retry_counter_values[TX_RETRY_NUMBER_OF_COUNTERS] = {0};
static const char *tx_retry_counter_names[TX_RETRY_NUMBER_OF_COUNTERS] = {
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

uint32_t tx_retry__get_number_of_counters(void)
{
  return TX_RETRY_NUMBER_OF_COUNTERS;
}

const char *tx_retry__get_counter_name(uint32_t counter_index)
{
  if (counter_index >= TX_RETRY_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return tx_retry_counter_names[counter_index];
}

volatile uint32_t *tx_retry__get_counter_address(uint32_t counter_index)
{
  if (counter_index >= TX_RETRY_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return &tx_retry_counter_values[counter_index];
}

void tx_retry__reset_counters(void)
{
  memset((void *)tx_retry_counter_values, 0, sizeof(tx_retry_counter_values));
}

void tx_retry__increment_counter(tx_retry_counter_index_t counter_index)
{
  if (counter_index < TX_RETRY_NUMBER_OF_COUNTERS)
  {
    tx_retry_counter_values[counter_index]++;
  }
}