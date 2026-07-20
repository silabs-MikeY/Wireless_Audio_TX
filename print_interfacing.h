#ifndef PRINT_INTERFACING_H
#define PRINT_INTERFACING_H

#include <stdbool.h>

void print_interfacing__init(void);
void print_interfacing__emit_text(const char *text);

void print_interfacing__printf(bool add_timestamp, bool print_immediately, const char *format, ...);
void print_interfacing__printf_static_string(bool add_timestamp, bool print_immediately, const char *counter_string);
void print_interfacing__printf_volatile_string(bool add_timestamp, bool print_immediately, const char *counter_string);

#endif // PRINT_INTERFACING_H
