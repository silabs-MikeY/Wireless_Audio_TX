#include "counters.h"
#include <sl_core.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct new_counters_s {
  volatile uint32_t *counter_address;
  uint32_t saved_counter_value;
  uint32_t last_printed_counter_value;
  const char *counter_name;
  bool has_printed_counter_value;
#if (USE_STATIC_STRING_BUFFERS_FOR_COUNTER_PRINTING == 1)
  char counter_string_buffer[2][COUNTER_STRING_BUFFER_LENGTH];
  uint32_t counter_string_buffer_value_start_index;
  uint8_t counter_string_buffer_index;
#endif
} new_counters_t;

new_counters_t new_counters[COUNTER_BUFFER_MAX_COUNT];
static uint32_t counters_used_count = 0;

static bool printing_flag = false;
static bool printing_full_snapshot = true;
static uint32_t snapshots_since_full_print = COUNTER_FULL_PRINT_INTERVAL_SNAPSHOTS;
static uint32_t printing_counter_index = 0;

static void counters__save_counters(void);
static bool counters__counter_should_print(const new_counters_t *counter);
static bool counters__print_counter(new_counters_t *counter,
                                    uint32_t counter_index);

// -----------------------------------------------------------------------------
//                     Weak function implementations, do not rename.
// -----------------------------------------------------------------------------

__attribute__((weak)) void counters__printf(bool add_timestamp,
                                            const char *format, ...) {
  (void)add_timestamp;
  (void)format;
}

__attribute__((weak)) void
counters__printf_static_string(bool add_timestamp, const char *counter_string) {
  (void)add_timestamp;
  (void)counter_string;
}

__attribute__((weak)) void counters__process_counter_reset(void) {}

__attribute__((weak)) void counters__pre_save_hook(void) {}

__attribute__((weak)) void counters__post_print_hook(void) {}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Counter Registration
// -----------------------------------------------------------------------------

// Register a new counter with a name and get its index. Returns 0 on success,
// negative value on error.
int32_t counters__register_counter(const char *input_counter_name,
                                   uint32_t *input_counter_address) {
  if (counters_used_count >= COUNTER_BUFFER_MAX_COUNT) {
    // Handle error: maximum number of counters reached
    return -1;
  }
  if (input_counter_name == NULL || input_counter_address == NULL) {
    // Handle error: invalid input
    return -2;
  }
  if (input_counter_name[0] == '\0') {
    // Handle error: empty counter name
    return -3;
  }
  if (strlen(input_counter_name) >= COUNTER_NAME_MAX_LENGTH) {
    // Handle error: counter name too long
    return -4;
  }

  new_counters[counters_used_count].counter_name = input_counter_name;
  new_counters[counters_used_count].counter_address = input_counter_address;
  new_counters[counters_used_count].saved_counter_value =
      0; // Initialize the saved counter value to 0
  new_counters[counters_used_count].last_printed_counter_value = 0;
  new_counters[counters_used_count].has_printed_counter_value = false;

#if (USE_STATIC_STRING_BUFFERS_FOR_COUNTER_PRINTING == 1)
  int prefix_length = snprintf(
      new_counters[counters_used_count].counter_string_buffer[0],
      sizeof(new_counters[counters_used_count].counter_string_buffer[0]),
      " %u : %s,", (unsigned int)counters_used_count, input_counter_name);
  if ((prefix_length < 0) ||
      (prefix_length >=
       (int)sizeof(new_counters[counters_used_count].counter_string_buffer[0]))) {
    memset(&new_counters[counters_used_count], 0,
           sizeof(new_counters[counters_used_count]));
    return -5;
  }

  memcpy(new_counters[counters_used_count].counter_string_buffer[1],
         new_counters[counters_used_count].counter_string_buffer[0],
         (size_t)prefix_length + 1U);

  new_counters[counters_used_count].counter_string_buffer_value_start_index =
      (uint32_t)prefix_length;
  new_counters[counters_used_count].counter_string_buffer_index = 0;
#endif

  counters__printf(true, "Registered counter: %s at index %u\n",
                   input_counter_name, counters_used_count);
  counters_used_count++;
  return 0;
}

// -----------------------------------------------------------------------------
//                     Counter Registration End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Counter Snapshot
// -----------------------------------------------------------------------------

// Saves the current counter values to non-volatile storage.
static void counters__save_counters(void) {
  for (uint32_t i = 0; i < counters_used_count; i++) {
    // Save each counter value to non-volatile storage
    if (new_counters[i].counter_address == NULL) {
      assert(0); // This should not happen, as we check for NULL during
                 // registration
    }
    new_counters[i].saved_counter_value = *(new_counters[i].counter_address);
  }
}

void counters__save_and_print_counters(uint32_t current_timestamp) {
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  counters__pre_save_hook();
  counters__save_counters();
  counters__process_counter_reset();
  CORE_EXIT_CRITICAL();

  snapshots_since_full_print++;
  if (snapshots_since_full_print >= COUNTER_FULL_PRINT_INTERVAL_SNAPSHOTS) {
    printing_full_snapshot = true;
    snapshots_since_full_print = 0;
  } else {
    printing_full_snapshot = false;
  }

  printing_flag = true;
  printing_counter_index = 0;
  counters__printf(false, "\n");
  counters__printf(true, "%s counters saved at timestamp: %u\n",
                   printing_full_snapshot ? "Full" : "Changed",
                   (unsigned int)current_timestamp);
}

// -----------------------------------------------------------------------------
//                     Counter Snapshot End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Counter Printing
// -----------------------------------------------------------------------------

void counters__run_print_state_machine(void) {
  if (printing_flag) {
    if (printing_counter_index < counters_used_count) {
      new_counters_t *counter = &new_counters[printing_counter_index];

      if (counters__counter_should_print(counter)) {
        (void)counters__print_counter(counter, printing_counter_index);
      }
      printing_counter_index++;
    } else {
      counters__printf(false, "\n");
      printing_flag = false; // Finished printing all counters
      counters__post_print_hook();
    }
  }
}

static bool counters__counter_should_print(const new_counters_t *counter) {
  if (counter == NULL) {
    return false;
  }

  if (printing_full_snapshot || (counter->has_printed_counter_value == false)) {
    return true;
  }

  return (counter->saved_counter_value != counter->last_printed_counter_value);
}

static bool counters__print_counter(new_counters_t *counter,
                                    uint32_t counter_index) {
  if (counter == NULL) {
    return false;
  }

#if (USE_STATIC_STRING_BUFFERS_FOR_COUNTER_PRINTING == 1)
  (void)counter_index;

  uint32_t value_start_index = counter->counter_string_buffer_value_start_index;
  uint8_t string_buffer_index = counter->counter_string_buffer_index ^ 1U;
  char *counter_string_buffer = counter->counter_string_buffer[string_buffer_index];

  if (value_start_index >= COUNTER_STRING_BUFFER_LENGTH) {
    return false;
  }

  size_t remaining_length = COUNTER_STRING_BUFFER_LENGTH - value_start_index;
  int value_length = snprintf(counter_string_buffer + value_start_index,
                              remaining_length, " %u\n",
                              (unsigned int)counter->saved_counter_value);

  if ((value_length < 0) || (value_length >= (int)remaining_length)) {
    return false;
  }

  counter->counter_string_buffer_index = string_buffer_index;
  counters__printf_static_string(false, counter_string_buffer);
#else
  counters__printf(false, " %u : %s, %u\n", counter_index,
                   counter->counter_name, counter->saved_counter_value);
#endif

  counter->last_printed_counter_value = counter->saved_counter_value;
  counter->has_printed_counter_value = true;
  return true;
}

// -----------------------------------------------------------------------------
//                     Counter Printing End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Counter General
// -----------------------------------------------------------------------------

bool counters__init(void) {
  counters_used_count = 0;
  memset(new_counters, 0, sizeof(new_counters));

  printing_flag = false;
  printing_full_snapshot = true;
  snapshots_since_full_print = COUNTER_FULL_PRINT_INTERVAL_SNAPSHOTS;
  printing_counter_index = 0;
  return true;
}

// -----------------------------------------------------------------------------
//                     Counter General End
// -----------------------------------------------------------------------------
