#include "string_printer.h"
#include <stdio.h>

bool library_initialized = false;
volatile uint32_t print_head = 0;
volatile uint32_t print_tail = 0;

typedef struct {
  char print_text[100];
  bool used;
  bool ready;
} debug__print_buffer_t;

volatile debug__print_buffer_t debug__print_buffers[NUMBER_OF_PRINT_BUFFERS_NEW];

#if (STREAM_LOCKING_ENABLED == 1)
volatile bool stream_busy = false;
void unlock_stream(void) { stream_busy = false; }
#else
void unlock_stream(void) {
  // Do nothing if stream locking is disabled
}
#endif

// Initialize mutex
void stream_busy_t_init(void) {
  stream_busy = false;
  library_initialized = true;
}

// static void lock_stream(void) {
//     ENTER_CRITICAL();
//     stream_busy = true;
//     EXIT_CRITICAL();
// }

__attribute__((weak)) void print__print_string(char *array_pointer) {
  printf("%s", array_pointer);
}

void try_to_print_a_string(void) {
  if (!library_initialized) {
    return;
  }

#if (STREAM_LOCKING_ENABLED == 1)
  {

    // Test stream_busy before entering critical section to avoid overhead of critical section if stream is busy 
    if (stream_busy) {
        return;
    }       

    ENTER_CRITICAL();
    // Need to test stream_busy again after entering critical section to avoid race condition
    if (!stream_busy) {
      stream_busy = true;
      print__print_string((char *)debug__print_buffers[print_tail].print_text);
      stream_busy = false;
    }
    EXIT_CRITICAL();
  }
  #else
    {
        print__printf_string(array_pointer);
    }
    #endif
}

static int32_t get_available_print_buffer(void)
{
  if (!library_initialized) {
    return -1;
  }

  ENTER_CRITICAL();
  if (debug__print_buffers[print_head].used) {
    // No available buffer, drop the print
    EXIT_CRITICAL();
    return -2;
  }

  uint32_t allocated_index = print_head;
  debug__print_buffers[allocated_index].used = true;
  print_head = (print_head + 1);
  if (print_head >= NUMBER_OF_PRINT_BUFFERS_NEW) {
    print_head = 0;
  }
  EXIT_CRITICAL();
  return allocated_index;
}

void add_existing_string_to_print_queue(const char *array_pointer, uint32_t length) {
  if (!library_initialized) {
    return;
  }

  int32_t index = get_available_print_buffer();
    if (index < 0) {
        // No available buffer, drop the print
        return;
    }

    uint32_t sanitized_length = length;
    if (sanitized_length >= sizeof(debug__print_buffers[index].print_text)) {
        sanitized_length = sizeof(debug__print_buffers[index].print_text) - 1;
    }

  memcpy(debug__print_buffers[index].print_text, array_pointer, sanitized_length);

// Try to start a print. To avoid CRITICAL SECTION OVERHEAD, not bothering to even call try_to_print_a_string if the stream might be busy. Will just be printed later
#if (STREAM_LOCKING_ENABLED == 1)
  if (!stream_busy) {
    try_to_print_a_string();
  }
#else
  {
    try_to_print_a_string();
  }
#endif
}