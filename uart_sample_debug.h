#ifndef UART_SAMPLE_DEBUG_H
#define UART_SAMPLE_DEBUG_H

#include <stdint.h>
#include <stdbool.h>

uint32_t uart_sample_debug__get_microsecond_ticks(void);
void uart_sample_debug__write_to_buffer(uint8_t *data, uint32_t length);

void uart_sample_debug__flag_transmit_complete(void);
void uart_sample_debug__init(void);
void uart_sample_debug__add_to_buffer(uint8_t *data, uint32_t length);
void uart_sample_debug__transmit_buffer(uint32_t index);
void uart_sample_debug__process_transmit_complete(void);
void uart_sample_debug__reset_buffer(uint32_t index);
void uart_sample_debug__reset_all_buffers(void);
void uart_sample_debug__check_for_new_data_and_transmit(void);

#endif // UART_SAMPLE_DEBUG_H