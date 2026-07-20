#include "uart_sample_debug.h"
#include "assert.h"
#include <string.h>

#define BUFFER_COUNT 30

uint8_t uart_sample_debug__buffer[BUFFER_COUNT][256];
bool uart_sample_debug__buffer_used[BUFFER_COUNT] = {0};
bool uart_sample_debug__buffer_ready[BUFFER_COUNT] = {0};
bool uart_sample_debug__buffer_transmitting[BUFFER_COUNT] = {0};
bool uart_sample_debug__buffer_transmit_complete[BUFFER_COUNT] = {0};
uint32_t uart_sample_debug__buffer_length[BUFFER_COUNT] = {0};
uint32_t uart_sample_debug__timestamp[BUFFER_COUNT] = {0};

uint32_t buffer_index_in_transmit = 0xFFFFFFFF;

__attribute__((weak)) uint32_t uart_sample_debug__get_microsecond_ticks(void) {
  assert(0); // This function should be implemented in the application to return the current microsecond ticks.
  return 0;
}

__attribute__((weak)) void uart_sample_debug__write_to_buffer(uint8_t *data, uint32_t length) {
  (void)data;
  (void)length;
  assert(0); // This function should be implemented in the application to handle UART transmission.
}

void uart_sample_debug__flag_transmit_complete(void) {
  if (buffer_index_in_transmit < BUFFER_COUNT) {
    uart_sample_debug__buffer_transmit_complete[buffer_index_in_transmit] = true;
    uart_sample_debug__buffer_transmitting[buffer_index_in_transmit] = false;
    uart_sample_debug__buffer_ready[buffer_index_in_transmit] = false;
    uart_sample_debug__buffer_used[buffer_index_in_transmit] = false;
    uart_sample_debug__buffer_length[buffer_index_in_transmit] = 0;
    buffer_index_in_transmit = 0xFFFFFFFF;
    uart_sample_debug__check_for_new_data_and_transmit();
  }
}

void uart_sample_debug__init(void) {
  // Initialize UART for debugging purposes
  // This function should set up the UART peripheral with the desired baud rate,
  // data bits, stop bits, and parity. The implementation will depend on the
  // specific microcontroller and its UART library.

  uart_sample_debug__reset_all_buffers();
}

void uart_sample_debug__add_to_buffer(uint8_t *data, uint32_t length) {
  // Add the provided data to the UART debug buffer.
  // This function should handle buffering the data and preparing it for
  // transmission over UART. The implementation will depend on the specific
  // requirements of the application.

  uint32_t index = 0;
  for (index = 0; index < BUFFER_COUNT; index++) {
    if (!uart_sample_debug__buffer_used[index]) {
      uart_sample_debug__buffer_used[index] = true;
      break;
    }
  }

  if (index < BUFFER_COUNT) {
    // Copy data to the buffer
    if (length > sizeof(uart_sample_debug__buffer[index])) {
      assert(0); // Data length exceeds buffer size
    }
    memcpy(uart_sample_debug__buffer[index], data, length);
    uart_sample_debug__buffer_ready[index] = true;
    uart_sample_debug__buffer_length[index] = length;
    uart_sample_debug__timestamp[index] = uart_sample_debug__get_microsecond_ticks();
  }
  else
  {
    // No available buffer, handle the error (e.g., drop data, log error, etc.)
    assert(0); // No available buffer
  }
}

void uart_sample_debug__transmit_buffer(uint32_t index) {
  // Transmit the buffered data over UART.
  // This function should handle sending the data in the buffer over the UART
  // interface. The implementation will depend on the specific requirements of
  // the application.
    uart_sample_debug__write_to_buffer(uart_sample_debug__buffer[index], uart_sample_debug__buffer_length[index]);
  
}

void uart_sample_debug__process_transmit_complete(void) {
  // Handle the completion of a UART transmission.
  // This function should be called when a UART transmission is complete, and it
  // should update the buffer state accordingly. The implementation will depend
  // on the specific requirements of the application.

  if (buffer_index_in_transmit < BUFFER_COUNT) {
    uart_sample_debug__buffer_transmit_complete[buffer_index_in_transmit] = true;
    uart_sample_debug__buffer_transmitting[buffer_index_in_transmit] = false;
    uart_sample_debug__buffer_ready[buffer_index_in_transmit] = false;
    uart_sample_debug__buffer_used[buffer_index_in_transmit] = false;
    uart_sample_debug__buffer_length[buffer_index_in_transmit] = 0;
    buffer_index_in_transmit = 0xFFFFFFFF;
  }
}

void uart_sample_debug__reset_buffer(uint32_t index) {
  if (index < BUFFER_COUNT) {
    uart_sample_debug__buffer_used[index] = false;
    uart_sample_debug__buffer_ready[index] = false;
    uart_sample_debug__buffer_transmitting[index] = false;
    uart_sample_debug__buffer_transmit_complete[index] = false;
    uart_sample_debug__buffer_length[index] = 0;
  }
}

void uart_sample_debug__reset_all_buffers(void) {
  for (uint32_t i = 0; i < BUFFER_COUNT; i++) {
    uart_sample_debug__reset_buffer(i);
  }
}

void uart_sample_debug__check_for_new_data_and_transmit(void) {
  uint32_t oldest_index = 0xFFFFFFFF;
  uint32_t oldest_timestamp = 0xFFFFFFFF;
  
  for (uint32_t i = 0; i < BUFFER_COUNT; i++) {
    if (uart_sample_debug__buffer_ready[i] &&
        !uart_sample_debug__buffer_transmitting[i]) {
      if (uart_sample_debug__timestamp[i] < oldest_timestamp) {
        oldest_timestamp = uart_sample_debug__timestamp[i];
        oldest_index = i;
      }
    }
  }

  if (oldest_index != 0xFFFFFFFF) {
    uart_sample_debug__buffer_transmitting[oldest_index] = true;
    uart_sample_debug__buffer_transmit_complete[oldest_index] = false;
    uart_sample_debug__transmit_buffer(oldest_index);
    buffer_index_in_transmit = oldest_index;
  }
}