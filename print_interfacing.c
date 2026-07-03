#include "print_interfacing.h"
#include "print.h"
#include "em_core.h"
#include "button.h"
#include "adc.h"
// #include "audio_buffers.h"
#include "microseconds.h"
#include "radio_packet_buffers.h"
#include "radio_base.h"
#include "counters_new.h"
#include "scheduler.h"
#include "sl_iostream.h"
#include "sl_iostream_handles.h"

#include <string.h>
#include <stdbool.h>

void print_interfacing__vprintf(bool add_timestamp, const char *format, va_list args)
{
  char formatted_text[100];
  bool in_irq_context;

  (void)add_timestamp;
  vsnprintf(formatted_text, sizeof(formatted_text), format, args);
  in_irq_context = CORE_InIrqContext();

#if (DEBUG_PRINT_TO_BUFFERS == 1)
  if (in_irq_context == true)
  {
    debug__printf_to_buf_append_time(0, "%s", formatted_text);
  }
  else
  {
    if (add_timestamp)
    {
      debug__printf_to_buf_append_time(0, "%s", formatted_text);
    }
    else
    {
      printf_to_buf(0, "%s", formatted_text);
    }
  }
#else
  if (in_irq_context == true)
  {
    printf("%s", formatted_text);
  }
  else
  {
    printf("%s", formatted_text);
  }
#endif
}

#define DEFINE_PRINT_INTERFACING_WRAPPER(function_name) \
void function_name(bool add_timestamp, const char *format, ...) \
{ \
  va_list args; \
  \
  va_start(args, format); \
  print_interfacing__vprintf(add_timestamp, format, args); \
  va_end(args); \
}

void print_interfacing__printf(bool add_timestamp, const char *format, ...)
{
  va_list args;

  va_start(args, format);
  print_interfacing__vprintf(add_timestamp, format, args);
  va_end(args);
}

DEFINE_PRINT_INTERFACING_WRAPPER(button__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(adc__printf)
// DEFINE_PRINT_INTERFACING_WRAPPER(audio_buffers__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(radio_packet_buffers__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(radio__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(counters__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(scheduler__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(microseconds__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(generic__printf)

#undef DEFINE_PRINT_INTERFACING_WRAPPER

void wdog__printf(bool add_timestamp, const char *format, ...)
{
  char formatted_text[128];
  va_list args;

  (void)add_timestamp;

  va_start(args, format);
  vsnprintf(formatted_text, sizeof(formatted_text), format, args);
  va_end(args);

  sl_iostream_t *console = sl_iostream_get_handle("inst");
  if (console == NULL)
  {
    return;
  }

  sl_iostream_write(console, (uint8_t *)formatted_text, strlen(formatted_text));
}