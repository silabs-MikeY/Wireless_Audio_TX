#ifndef COUNTERS_NEW_H
#define COUNTERS_NEW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

void counters_new__printf(bool add_timestamp, const char *format, ...);
void counters_new__process_counter_reset(void);
void counters_new__pre_save_hook(void);
void counters_new__run_print_state_machine(void);

uint32_t counters_new__get_counter_value(uint32_t counter_index);
int32_t counters_new__register_counter(const char* input_counter_name, uint32_t* input_counter_address);
void counters_new__init();
bool counters_new__get_non_volatile_counters_array(uint32_t* counters_array, uint32_t* array_size);
void counters_new__save_and_print_counters(uint32_t current_timestamp);

#define COUNTER_BUFFER_MAX_COUNT 128
#define COUNTER_NAME_MAX_LENGTH 64


#endif // COUNTERS_NEW_H