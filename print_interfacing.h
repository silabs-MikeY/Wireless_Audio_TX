#ifndef PRINT_INTERFACING_H
#define PRINT_INTERFACING_H

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

void print_interfacing__vprintf(bool add_timestamp, const char *format, va_list args);
void print_interfacing__printf(bool add_timestamp, const char *format, ...);

#endif // PRINT_INTERFACING_H
