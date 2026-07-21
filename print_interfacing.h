#ifndef PRINT_INTERFACING_H
#define PRINT_INTERFACING_H

#include <stdbool.h>

bool print_interfacing__init(unsigned int ldma_channel);
void print_interfacing__emit_text(const char *text);
bool print_interfacing__process(void);
// Requires interrupts enabled so console_tx can invoke its completion callback.
void print_interfacing__flush_blocking(void);

void print_interfacing__printf(bool add_timestamp, bool print_immediately, const char *format, ...);
// Only use with string storage that remains valid until the console TX callback completes.
void print_interfacing__printf_static_string(bool add_timestamp, bool print_immediately, const char *counter_string);
void print_interfacing__printf_volatile_string(bool add_timestamp, bool print_immediately, const char *counter_string);

#endif // PRINT_INTERFACING_H
