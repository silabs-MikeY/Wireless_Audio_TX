#include "print_interfacing.h"
#include "print.h"
#include "button.h"
#include "adc.h"
#include "audio_buffers.h"
#include "radio_base.h"
#include "counters_new.h"
#include "scheduler.h"

#include <stdbool.h>

static void print_interfacing__vprintf(bool add_timestamp, const char *format, va_list args)
{
  char formatted_text[100];

  vsnprintf(formatted_text, sizeof(formatted_text), format, args);

  if (add_timestamp)
  {
    printf("%s", formatted_text);
    //debug__printf_to_buf_append_time(0, "%s", formatted_text);
  }
  else
  {
    printf("%s", formatted_text);
    //printf_to_buf(0, "%s", formatted_text);
  }
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

DEFINE_PRINT_INTERFACING_WRAPPER(button__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(adc__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(audio_buffers__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(radio__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(counters__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(scheduler__printf)
DEFINE_PRINT_INTERFACING_WRAPPER(wdog__printf)

#undef DEFINE_PRINT_INTERFACING_WRAPPER