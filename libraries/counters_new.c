#include "counters_new.h"
#include <stdint.h>
#include <string.h>
#include <sl_core.h>
#include "print_interfacing.h"

typedef struct new_counters_s {
    volatile uint32_t* counter_address;
    uint32_t saved_counter_value;
    const char* counter_name;
} new_counters_t;

new_counters_t new_counters[COUNTER_BUFFER_MAX_COUNT];
static uint32_t counters_used_count = 0;

// static uint32_t new_saved_counters[COUNTER_BUFFER_MAX_COUNT] = {0};
// static char* new_counters_names[COUNTER_BUFFER_MAX_COUNT] = {0};
// static uint32_t* new_counter_addresses[COUNTER_BUFFER_MAX_COUNT] = {0};

static bool printing_flag = false;
static uint32_t printing_counter_index = 0;

static void counters_new__save_counters(void);

// // User implementation of the weak function to get counter values. 
// // This should be overridden in the application code and return requested counter value based on the index. 
// // For now, it returns 0 for all counters.
// __attribute__((weak)) uint32_t counters_new__get_counter_value(uint32_t counter_index)
// {
//     return 0;
// }

__attribute__((weak)) void counters_new__printf(bool add_timestamp, const char *format, ...)
{
    (void)add_timestamp;
    (void)format;
}

__attribute__((weak)) void counters_new__process_counter_reset(void)
{
}

__attribute__((weak)) void counters_new__pre_save_hook(void)
{
}

// Register a new counter with a name and get its index. Returns 0 on success, negative value on error.
int32_t counters_new__register_counter(const char* input_counter_name, uint32_t* input_counter_address)
{
    if (counters_used_count >= COUNTER_BUFFER_MAX_COUNT)
    {
        // Handle error: maximum number of counters reached
        return -1;
    }
    if (input_counter_name == NULL || input_counter_address == NULL)
    {
        // Handle error: invalid input
        return -2;
    }
    
    *input_counter_address = counters_used_count;
    new_counters[counters_used_count].counter_name = input_counter_name;
    new_counters[counters_used_count].counter_address = input_counter_address;
    new_counters[counters_used_count].saved_counter_value = 0; // Initialize the saved counter value to 0
    counters_new__printf(true, "Registered counter: %s at index %u\n", input_counter_name, counters_used_count);
    counters_used_count++;
    return 0;
}

// Saves the current counter values to non-volatile storage.
static void counters_new__save_counters(void)
{
    for (uint32_t i = 0; i < counters_used_count; i++)
    {
        // Save each counter value to non-volatile storage
        if (new_counters[i].counter_address == NULL)
        {
            assert(0); // This should not happen, as we check for NULL during registration
        }
        new_counters[i].saved_counter_value = *(new_counters[i].counter_address);
    }
}

void counters_new__save_and_print_counters(uint32_t current_timestamp)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    counters_new__pre_save_hook();
    counters_new__save_counters();
    counters_new__process_counter_reset();
    CORE_EXIT_CRITICAL();

    printing_flag = true;
    printing_counter_index = 0;
    counters_new__printf(false, "\n");
    counters_new__printf(true, "Counters saved at timestamp: %u", (unsigned int)current_timestamp);

}

void counters_new__run_print_state_machine(void)
{
    if (printing_flag)
    {
        if (printing_counter_index < counters_used_count)
        {
            // Print the counter name and value
            counters_new__printf(false, " %u : %s, %u\n", printing_counter_index, new_counters[printing_counter_index].counter_name, new_counters[printing_counter_index].saved_counter_value);
            printing_counter_index++;
        }
        else
        {
            counters_new__printf(false, "\n");
            printing_flag = false; // Finished printing all counters
        }
    }
}

void counters_new__init()
{
    counters_used_count = 0;
    memset(new_counters, 0, sizeof(new_counters));

    printing_flag = false;
    printing_counter_index = 0;
}