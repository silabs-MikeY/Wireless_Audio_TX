#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include <stdio.h>

#define DEBUG_ENABLE 1

#if (DEBUG_ENABLE == 1)

#define DEBUG_ALL 1

//  #define DEBUG_PERIPHERALS
// // #define DEBUG_COUNTERS
//  #define DEBUG_RADIO
//  #define DEBUG_STATES
//  #define DEBUG_RETRY

// DEBUG ALL ENABLE SWITCHES

#if (DEBUG_ALL == 1)
#ifndef DEBUG_PERIPHERALS
#define DEBUG_PERIPHERALS 1
#endif

#ifndef DEBUG_COUNTERS
#define DEBUG_COUNTERS 1
#endif

// #ifndef DEBUG_PACKETS
// #define DEBUG_PACKETS 1
// #endif

#ifndef DEBUG_RADIO
#define DEBUG_RADIO 1
#endif

#ifndef DEBUG_STATES
#define DEBUG_STATES 1
#endif

#ifndef DEBUG_RETRY
#define DEBUG_RETRY 1
#endif

#endif

// END DEBUG ALL ENABLE SWITCHES

#endif // End DEBUG ENABLE

// DEBUG ENABLE MACROS

// #ifdef DEBUG_PACKETS
// #define DEBUG_PACKETS_LOG(x) x
// #else
// #define DEBUG_PACKETS_LOG(x)
// #endif

#ifdef DEBUG_PERIPHERALS
#define DEBUG_PERIPHERALS_LOG(x) x
#else
#define DEBUG_PERIPHERALS_LOG(x)
#endif

#ifdef DEBUG_COUNTERS
#define DEBUG_COUNTERS_LOG(x) x
#else
#define DEBUG_COUNTERS_LOG(x)
#endif

#ifdef DEBUG_RADIO
#define DEBUG_RADIO_LOG(x) x
#else
#define DEBUG_RADIO_LOG(x)
#endif

#ifdef DEBUG_STATES
#define DEBUG_STATES_LOG(x) x
#else
#define DEBUG_STATES_LOG(x)
#endif

#ifdef DEBUG_RETRY
#define DEBUG_RETRY_LOG(x) x
#else
#define DEBUG_RETRY_LOG(x)
#endif

// END DEBUG ENABLE MACROS



// If 1 prints get stored in buffers and printed out later
// Ensure NUMBER_OF_PRINT_BUFFERS is large enough to hold all prints between debug_print() calls
// If false, prints go directly to stdout
#define DEBUG_PRINT_TO_BUFFERS 1

// Formats timestamps so they're easier to read. Adds extra processing though
#define FORMAT_TIMESTAMPS_WITH_COMMAS 1

typedef struct{
  char print_text[100];
  const char *print_text_pointer;
  bool used;
  bool ready;
  bool print_in_progress;
  bool use_print_text_pointer;
  uint64_t timestamp_ticks;
} debug__print_buffer_t;
#define NUMBER_OF_PRINT_BUFFERS 100

void debug__check_print_buffers_and_print(void);
void debug__print_all_buffers(void);
bool debug__get_oldest_print_buffer(const char **text, size_t *length, uint8_t *buffer_index);
bool debug__print_buffers_empty(void);
void debug__mark_print_buffer_pending(uint8_t buffer_index);
void debug__mark_print_buffer_reusable(uint8_t buffer_index);
void printf_to_buf(uint32_t time, const char *format, ...);
void printf_to_buf_string(uint32_t time, const char *text);
void printf_to_buf_static_string(uint32_t time, const char *text);
void debug__printf_to_buf_append_time(uint32_t time, const char *format, ...);
void printf_to_buf_array(const uint8_t *data, uint32_t data_length, bool line_break_at_end);
void debug__format_timestamp_with_commas(uint64_t value, char *out, size_t outSize);
void debug__write_console_text(const char *text);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H_INCLUDED */
