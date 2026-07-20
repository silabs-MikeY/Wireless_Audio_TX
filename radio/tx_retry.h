#ifndef TX_RETRY_H_
#define TX_RETRY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

typedef enum tx_retry_counter_index_s {
    TX_RETRY_COUNTER_ATTEMPTS = 0,
    TX_RETRY_COUNTER_ATTEMPT_SUCCESS,
    TX_RETRY_COUNTER_ATTEMPT_FAILED,
    TX_RETRY_COUNTER_SUCCESS,
    TX_RETRY_COUNTER_FAIL_FAILED_TO_SEND,
    TX_RETRY_COUNTER_FAIL_ABORT,
    TX_RETRY_COUNTER_FAIL_BLOCK,
    TX_RETRY_COUNTER_FAIL_UNDERFLOW,
    TX_RETRY_COUNTER_FAIL_BUSY,
    TX_RETRY_COUNTER_FAIL_MISSED,
    TX_RETRY_NUMBER_OF_COUNTERS
} tx_retry_counter_index_t;

void tx_retry__printf(bool add_timestamp, const char *format, ...);

uint32_t tx_retry__get_number_of_counters(void);
const char *tx_retry__get_counter_name(uint32_t counter_index);
volatile uint32_t *tx_retry__get_counter_address(uint32_t counter_index);
void tx_retry__reset_counters(void);

void tx_retry__increment_counter(tx_retry_counter_index_t counter_index);

#endif /* TX_RETRY_H_ */