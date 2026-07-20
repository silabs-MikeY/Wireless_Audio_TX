#ifndef STRING_PRINTER_H
#define STRING_PRINTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Settings
#define STREAM_LOCKING_ENABLED 1
#define NUMBER_OF_PRINT_BUFFERS_NEW 100

void print__printf_string(const char *array_pointer);

#endif /* STRING_PRINTER_H */