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
#include "sl_iostream.h"
#include "uart_sample_debug.h"
#include <string.h>
#include <stdbool.h>

static sl_iostream_t *print_interfacing__console = NULL;

void print_interfacing__init(void)
{
  print_interfacing__console = sl_iostream_get_default();
}

void print_interfacing__emit_text(const char *text)
{
  if (text == NULL)
  {
    return;
  }

  if (print_interfacing__console == NULL)
  {
    return;
  }

  sl_iostream_write(print_interfacing__console, (uint8_t *)text, strlen(text));
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

#if (DEBUG_PRINT_TO_BUFFERS == 1)
  if (print_immediately)
  {
    print_interfacing__emit_text(formatted_text);
  }
  else
  {
    printf_to_buf_string(0, formatted_text);
  }
#else
  print_interfacing__emit_text(formatted_text);
#endif
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

  #if (DEBUG_PRINT_TO_BUFFERS == 1)
  if (print_immediately)
  {
    print_interfacing__emit_text(counter_string);
  }
  else
  {
    printf_to_buf_static_string(0, counter_string);
  }
  #else
    print_interfacing__emit_text(counter_string);
  #endif
}

void print_interfacing__printf_volatile_string(bool add_timestamp, bool print_immediately, const char *counter_string)
{
  (void)add_timestamp;
  (void)print_immediately;
  if (counter_string == NULL)
  {
    assert(0);
  }

  #if (DEBUG_PRINT_TO_BUFFERS == 1)
  if (print_immediately)
  {
    print_interfacing__emit_text(counter_string);
  }
  else
  {
    printf_to_buf_string(0, counter_string);
  }
  #else
    print_interfacing__emit_text(counter_string);
  #endif
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

// Never buffer WDOG prints, as they are critical for debugging and should be output immediately.
void wdog__printf(bool add_timestamp, const char *format, ...)
{
  char formatted_text[128];
  va_list args;

  (void)add_timestamp;

  va_start(args, format);
  vsnprintf(formatted_text, sizeof(formatted_text), format, args);
  va_end(args);
  print_interfacing__emit_text(formatted_text);
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

    print_interfacing__emit_text(text);
    //printf("%s", text);
}

void uart_sample_debug__write_to_buffer(uint8_t *data, uint32_t length)
{
  (void)data;
  (void)length;
  sl_iostream_write(print_interfacing__console, data, length);
}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------