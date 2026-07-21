#include "print_interfacing.h"
#include "print.h"
#include "button.h"
#include "adc.h"
// #include "audio_buffers.h"
#include "microseconds.h"
#include "radio_packet_buffers.h"
#include "radio_base.h"
#include "counters_new.h"
#include "scheduler.h"
#include "libraries/console_tx.h"
#include "uart_sample_debug.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static void print_interfacing__console_tx_complete(void *user_context)
{
  debug__mark_print_buffer_reusable((uint8_t)(uintptr_t)user_context);
}

bool print_interfacing__init(unsigned int ldma_channel)
{
  return console_tx__init(ldma_channel);
}

void print_interfacing__emit_text(const char *text)
{
  if (text == NULL)
  {
    return;
  }

  printf_to_buf_string(0, text);
}

bool print_interfacing__process(void)
{
  const char *text = NULL;
  size_t length = 0U;
  uint8_t buffer_index = NUMBER_OF_PRINT_BUFFERS;

  if (console_tx__busy())
  {
    return false;
  }

  if (!debug__get_oldest_print_buffer(&text, &length, &buffer_index))
  {
    return false;
  }

  if (length == 0U)
  {
    debug__mark_print_buffer_reusable(buffer_index);
    return true;
  }

  if (!console_tx__print(text,
                         length,
                         print_interfacing__console_tx_complete,
                         (void *)(uintptr_t)buffer_index))
  {
    debug__mark_print_buffer_pending(buffer_index);
    return false;
  }

  return true;
}

void print_interfacing__flush_blocking(void)
{
  while (console_tx__busy() || !debug__print_buffers_empty())
  {
    (void)print_interfacing__process();
  }
}

static void print_interfacing__vprintf(bool add_timestamp, bool print_immediately, const char *format, va_list args)
{
  char formatted_text[100];

  (void)add_timestamp;
  (void)print_immediately;

  if (format == NULL)
  {
    return;
  }

  vsnprintf(formatted_text, sizeof(formatted_text), format, args);

  printf_to_buf_string(0, formatted_text);
}

void print_interfacing__printf(bool add_timestamp, bool print_immediately, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  print_interfacing__vprintf(add_timestamp, print_immediately, format, args);
  va_end(args);
}

void print_interfacing__printf_static_string(bool add_timestamp, bool print_immediately, const char *counter_string)
{
  (void)add_timestamp;
  (void)print_immediately;
  if (counter_string == NULL)
  {
    assert(0);
  }

  printf_to_buf_static_string(0, counter_string);
}

void print_interfacing__printf_volatile_string(bool add_timestamp, bool print_immediately, const char *counter_string)
{
  (void)add_timestamp;
  (void)print_immediately;
  if (counter_string == NULL)
  {
    assert(0);
  }

  printf_to_buf_string(0, counter_string);
}


// -----------------------------------------------------------------------------
//                     Printf Wrappers for Various Modules
// -----------------------------------------------------------------------------

#define DEFINE_PRINT_INTERFACING_WRAPPER(function_name) \
void function_name(bool add_timestamp, const char *format, ...) \
{ \
  va_list args; \
  \
  va_start(args, format); \
  print_interfacing__vprintf(add_timestamp, false, format, args); \
  va_end(args); \
}

DEFINE_PRINT_INTERFACING_WRAPPER(button__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(adc__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(radio_packet_buffers__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(radio__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(radio_statistics__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(tx_retry__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(counters__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(scheduler__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(microseconds__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(state_machine__printf)
 

#undef DEFINE_PRINT_INTERFACING_WRAPPER

void wdog__printf(bool add_timestamp, const char *format, ...)
{
  char formatted_text[128];
  va_list args;

  (void)add_timestamp;

  va_start(args, format);
  vsnprintf(formatted_text, sizeof(formatted_text), format, args);
  va_end(args);
  printf_to_buf_string(0, formatted_text);
}

void counters__printf_static_string(bool add_timestamp, const char *counter_string)
{
  print_interfacing__printf_static_string(add_timestamp, false, counter_string);
}

// -----------------------------------------------------------------------------
//                     Printf Wrappers End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Weak function implementations, do not rename.
// -----------------------------------------------------------------------------

void debug__write_console_text(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    printf_to_buf_string(0, text);
}

void uart_sample_debug__write_to_buffer(uint8_t *data, uint32_t length)
{
  if ((data == NULL) || (length == 0U))
  {
    return;
  }

  printf_to_buf_array(data, length, false);
}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------
