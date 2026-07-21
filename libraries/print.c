#include "print.h"

#include "rail.h"

#include <stdarg.h>

// -----------------------------------------------------------------------------
//                     Weak function implementations, do not rename.
// -----------------------------------------------------------------------------

void __attribute__((weak)) debug__write_console_text(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    //printf("%s", text);
}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Print Formatting Helpers
// -----------------------------------------------------------------------------

void debug__format_timestamp_with_commas(uint64_t value, char *out, size_t outSize)
{
    char tmp[32];
    int len = snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)value);

    if (len < 0 || len >= (int)sizeof(tmp))
    {
        out[0] = '\0';
        return;
    }

    int commaCount = (len - 1) / 3;
    int outLen = len + commaCount;

    if (outLen + 1 > (int)outSize)
    {
        out[0] = '\0';
        return;
    }

    out[outLen] = '\0';

    int i = len - 1;
    int o = outLen - 1;
    int digitCount = 0;

    while (i >= 0)
    {
        out[o--] = tmp[i--];
        digitCount++;

        if (digitCount == 3 && i >= 0)
        {
            out[o--] = ',';
            digitCount = 0;
        }
    }
}

// -----------------------------------------------------------------------------
//                     Print Formatting Helpers End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Print Buffer Queue
// -----------------------------------------------------------------------------

#if (DEBUG_PRINT_TO_BUFFERS == 1)
volatile debug__print_buffer_t debug__print_buffers[NUMBER_OF_PRINT_BUFFERS];
static uint64_t debug__print_enqueue_sequence = 0;
static uint32_t debug__next_print_buffer_to_try = 0;
static uint32_t debug__print_buffer_used_count = 0;
static uint8_t debug__ready_fifo[NUMBER_OF_PRINT_BUFFERS];
static uint32_t debug__ready_fifo_head = 0;
static uint32_t debug__ready_fifo_tail = 0;
static uint32_t debug__ready_fifo_count = 0;

static bool debug__ready_fifo_push(uint8_t buffer_index)
{
    if ((buffer_index >= NUMBER_OF_PRINT_BUFFERS) ||
        (debug__ready_fifo_count >= NUMBER_OF_PRINT_BUFFERS))
    {
        return false;
    }

    debug__ready_fifo[debug__ready_fifo_tail] = buffer_index;
    debug__ready_fifo_tail =
        (debug__ready_fifo_tail + 1U) % NUMBER_OF_PRINT_BUFFERS;
    debug__ready_fifo_count++;
    return true;
}

static bool debug__ready_fifo_pop(uint8_t *buffer_index)
{
    if ((buffer_index == NULL) || (debug__ready_fifo_count == 0U))
    {
        return false;
    }

    *buffer_index = debug__ready_fifo[debug__ready_fifo_head];
    debug__ready_fifo_head =
        (debug__ready_fifo_head + 1U) % NUMBER_OF_PRINT_BUFFERS;
    debug__ready_fifo_count--;
    return true;
}

static void debug__clear_print_buffer_payload(uint8_t available_buffer_index)
{
    debug__print_buffers[available_buffer_index].print_text[0] = '\0';
    debug__print_buffers[available_buffer_index].print_text_pointer = NULL;
    debug__print_buffers[available_buffer_index].ready = false;
    debug__print_buffers[available_buffer_index].print_in_progress = false;
    debug__print_buffers[available_buffer_index].use_print_text_pointer = false;
}

static bool debug__reserve_print_buffer(uint8_t *available_buffer_index)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    *available_buffer_index = NUMBER_OF_PRINT_BUFFERS;

    for (uint32_t i = 0; i < NUMBER_OF_PRINT_BUFFERS; i++)
    {
        uint32_t candidate_index =
            (debug__next_print_buffer_to_try + i) % NUMBER_OF_PRINT_BUFFERS;

        if (debug__print_buffers[candidate_index].used == false)
        {
            *available_buffer_index = (uint8_t)candidate_index;
            debug__print_buffers[candidate_index].used = true;
            debug__print_buffer_used_count++;
            debug__clear_print_buffer_payload((uint8_t)candidate_index);
            debug__next_print_buffer_to_try =
                (candidate_index + 1U) % NUMBER_OF_PRINT_BUFFERS;
            break;
        }
    }

    CORE_EXIT_CRITICAL(); 

    return (*available_buffer_index < NUMBER_OF_PRINT_BUFFERS);
}

static void debug__finish_print_buffer(uint8_t available_buffer_index, uint32_t time)
{
    (void)time;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    debug__print_buffers[available_buffer_index].timestamp_ticks =
        ++debug__print_enqueue_sequence;
    debug__print_buffers[available_buffer_index].ready = true;
    (void)debug__ready_fifo_push(available_buffer_index);
    CORE_EXIT_CRITICAL();
}

static void debug__copy_text_to_print_buffer(uint8_t available_buffer_index, const char *text)
{
    if (text == NULL)
    {
        debug__print_buffers[available_buffer_index].print_text[0] = '\0';
        return;
    }

    strncpy((char *)&debug__print_buffers[available_buffer_index].print_text[0],
            text,
            sizeof(debug__print_buffers[available_buffer_index].print_text) - 1U);
    debug__print_buffers[available_buffer_index].print_text[
        sizeof(debug__print_buffers[available_buffer_index].print_text) - 1U] = '\0';
}

static void debug__set_print_buffer_pointer(uint8_t available_buffer_index, const char *text)
{
    debug__print_buffers[available_buffer_index].print_text_pointer = text;
    debug__print_buffers[available_buffer_index].use_print_text_pointer = true;
    debug__print_buffers[available_buffer_index].print_text[0] = '\0';
}

bool debug__get_oldest_print_buffer(const char **text, size_t *length, uint8_t *buffer_index)
{
    uint8_t selected_buffer_index;
    const char *selected_text = NULL;

    if ((text == NULL) || (length == NULL) || (buffer_index == NULL))
    {
        return false;
    }

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    if (debug__ready_fifo_pop(&selected_buffer_index) == false)
    {
        CORE_EXIT_CRITICAL();
        *text = NULL;
        *length = 0U;
        *buffer_index = NUMBER_OF_PRINT_BUFFERS;
        return false;
    }

    if ((selected_buffer_index >= NUMBER_OF_PRINT_BUFFERS) ||
        (debug__print_buffers[selected_buffer_index].used == false) ||
        (debug__print_buffers[selected_buffer_index].ready == false) ||
        (debug__print_buffers[selected_buffer_index].print_in_progress == true))
    {
        CORE_EXIT_CRITICAL();
        *text = NULL;
        *length = 0U;
        *buffer_index = NUMBER_OF_PRINT_BUFFERS;
        return false;
    }

    debug__print_buffers[selected_buffer_index].ready = false;
    debug__print_buffers[selected_buffer_index].print_in_progress = true;

    if (debug__print_buffers[selected_buffer_index].use_print_text_pointer == true)
    {
        selected_text = debug__print_buffers[selected_buffer_index].print_text_pointer;
    }
    else
    {
        selected_text = (const char *)debug__print_buffers[selected_buffer_index].print_text;
    }

    if (selected_text == NULL)
    {
        debug__print_buffers[selected_buffer_index].used = false;
        if (debug__print_buffer_used_count > 0U)
        {
            debug__print_buffer_used_count--;
        }
        debug__clear_print_buffer_payload(selected_buffer_index);
        CORE_EXIT_CRITICAL();
        return false;
    }

    *text = selected_text;
    *buffer_index = selected_buffer_index;
    CORE_EXIT_CRITICAL();
    *length = strlen(selected_text);
    return true;
}

bool debug__print_buffers_empty(void)
{
    bool empty = true;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    empty = (debug__print_buffer_used_count == 0U);
    CORE_EXIT_CRITICAL();

    return empty;
}

void debug__mark_print_buffer_reusable(uint8_t buffer_index)
{
    if (buffer_index >= NUMBER_OF_PRINT_BUFFERS)
    {
        return;
    }

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    if (debug__print_buffers[buffer_index].used == true)
    {
        debug__print_buffers[buffer_index].used = false;
        if (debug__print_buffer_used_count > 0U)
        {
            debug__print_buffer_used_count--;
        }
        debug__clear_print_buffer_payload(buffer_index);
    }
    CORE_EXIT_CRITICAL();
}

void debug__mark_print_buffer_pending(uint8_t buffer_index)
{
    if (buffer_index >= NUMBER_OF_PRINT_BUFFERS)
    {
        return;
    }

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    if (debug__print_buffers[buffer_index].used == true)
    {
        debug__print_buffers[buffer_index].ready = true;
        debug__print_buffers[buffer_index].print_in_progress = false;
        (void)debug__ready_fifo_push(buffer_index);
    }
    CORE_EXIT_CRITICAL();
}

static void debug__print_buffer_entry(uint32_t buffer_index)
{
    const char *text = NULL;
    size_t length = 0U;
    uint8_t selected_buffer_index = NUMBER_OF_PRINT_BUFFERS;

    (void)buffer_index;

    if (debug__get_oldest_print_buffer(&text, &length, &selected_buffer_index))
    {
        (void)length;
        debug__write_console_text(text);
        debug__mark_print_buffer_reusable(selected_buffer_index);
    }
}

void debug__check_print_buffers_and_print(void)
{
    debug__print_buffer_entry(0U);
}

void debug__print_all_buffers(void)
{
    for (uint32_t i = 0; i < NUMBER_OF_PRINT_BUFFERS; i++)
    {
        debug__check_print_buffers_and_print();
    }
}

void printf_to_buf_array(const uint8_t *data, uint32_t data_length, bool line_break_at_end)
{
    static const char hex_digits[] = "0123456789ABCDEF";
    char temp[100];
    uint32_t output_index = 0;

    for (uint32_t i = 0; i < data_length; i++)
    {
        if (output_index > sizeof(temp) - 4)
        {
            temp[output_index] = '\0';
            printf_to_buf(0, "%s", temp);
            output_index = 0;
        }

        temp[output_index++] = hex_digits[(data[i] >> 4) & 0x0F];
        temp[output_index++] = hex_digits[data[i] & 0x0F];

        if (i != data_length - 1)
        {
            temp[output_index++] = ' ';
        }
    }

    if (output_index > 0)
    {
        if (line_break_at_end)
        {
            temp[output_index++] = '\n';
        }

        temp[output_index] = '\0';
        printf_to_buf(0, "%s", temp);
    }
    else if (line_break_at_end)
    {
        printf_to_buf(0, "\n");
    }
}

void printf_to_buf(uint32_t time, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    uint8_t available_buffer_index;

    if (debug__reserve_print_buffer(&available_buffer_index) == false)
    {
        va_end(args);
        return;
    }

    vsnprintf((char *)&debug__print_buffers[available_buffer_index].print_text[0],
              sizeof(debug__print_buffers[available_buffer_index].print_text),
              format,
              args);

    debug__print_buffers[available_buffer_index].use_print_text_pointer = false;
    debug__print_buffers[available_buffer_index].print_text_pointer = NULL;
    debug__finish_print_buffer(available_buffer_index, time);

    va_end(args);
}

void printf_to_buf_string(uint32_t time, const char *text)
{
    uint8_t available_buffer_index;

    if (debug__reserve_print_buffer(&available_buffer_index) == false)
    {
        return;
    }

    debug__copy_text_to_print_buffer(available_buffer_index, text);
    debug__print_buffers[available_buffer_index].use_print_text_pointer = false;
    debug__print_buffers[available_buffer_index].print_text_pointer = NULL;
    debug__finish_print_buffer(available_buffer_index, time);
}

void printf_to_buf_static_string(uint32_t time, const char *text)
{
    uint8_t available_buffer_index;

    if (debug__reserve_print_buffer(&available_buffer_index) == false)
    {
        return;
    }

    debug__set_print_buffer_pointer(available_buffer_index, text);
    debug__finish_print_buffer(available_buffer_index, time);
}

void debug__printf_to_buf_append_time(uint32_t time, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    uint8_t available_buffer_index = NUMBER_OF_PRINT_BUFFERS;

    for (uint32_t i = 0; i < NUMBER_OF_PRINT_BUFFERS; i++)
    {
        uint32_t candidate_index =
            (debug__next_print_buffer_to_try + i) % NUMBER_OF_PRINT_BUFFERS;

        if (debug__print_buffers[candidate_index].used == false)
        {
            available_buffer_index = (uint8_t)candidate_index;
            debug__next_print_buffer_to_try =
                (candidate_index + 1U) % NUMBER_OF_PRINT_BUFFERS;
            break;
        }
    }

    if (available_buffer_index == NUMBER_OF_PRINT_BUFFERS)
    {
        CORE_EXIT_CRITICAL();
        va_end(args);
        return;
    }

    debug__print_buffers[available_buffer_index].used = true;
    debug__print_buffer_used_count++;
    debug__clear_print_buffer_payload(available_buffer_index);

    CORE_EXIT_CRITICAL();

    uint32_t offset_for_timestamp = 0;
    uint32_t timestamp = time;

    if (time == 0)
    {
        timestamp = RAIL_GetTime();
    }

#if (FORMAT_TIMESTAMPS_WITH_COMMAS == 1)
#define timestmap_char_buffer_SIZE 32
    static char timestmap_char_buffer[timestmap_char_buffer_SIZE];
    debug__format_timestamp_with_commas(timestamp, timestmap_char_buffer,
                                       timestmap_char_buffer_SIZE);
    offset_for_timestamp = snprintf(
        (char *)debug__print_buffers[available_buffer_index].print_text,
        sizeof(debug__print_buffers[available_buffer_index].print_text),
        "(t:%s) ",
        timestmap_char_buffer);
#else
    offset_for_timestamp = snprintf(
        (char *)debug__print_buffers[available_buffer_index].print_text,
        sizeof(debug__print_buffers[available_buffer_index].print_text),
        "(t:%u)",
        timestamp);
#endif

    size_t max_len = sizeof(debug__print_buffers[available_buffer_index].print_text) -
                     offset_for_timestamp;
    vsnprintf((char *)&debug__print_buffers[available_buffer_index].print_text[offset_for_timestamp],
              max_len,
              format,
              args);

    debug__print_buffers[available_buffer_index].use_print_text_pointer = false;
    debug__print_buffers[available_buffer_index].print_text_pointer = NULL;

    CORE_ENTER_CRITICAL();
    debug__print_buffers[available_buffer_index].timestamp_ticks =
        ++debug__print_enqueue_sequence;
    debug__print_buffers[available_buffer_index].ready = true;
    (void)debug__ready_fifo_push(available_buffer_index);
    CORE_EXIT_CRITICAL();

    va_end(args);
}
#else
void debug__check_print_buffers_and_print(void)
{
    return;
}

void debug__print_all_buffers(void)
{
    return;
}

bool debug__get_oldest_print_buffer(const char **text, size_t *length, uint8_t *buffer_index)
{
    if (text != NULL)
    {
        *text = NULL;
    }
    if (length != NULL)
    {
        *length = 0U;
    }
    if (buffer_index != NULL)
    {
        *buffer_index = NUMBER_OF_PRINT_BUFFERS;
    }
    return false;
}

bool debug__print_buffers_empty(void)
{
    return true;
}

void debug__mark_print_buffer_reusable(uint8_t buffer_index)
{
    (void)buffer_index;
}

void debug__mark_print_buffer_pending(uint8_t buffer_index)
{
    (void)buffer_index;
}

void printf_to_buf(uint32_t time, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
}

void printf_to_buf_string(uint32_t time, const char *text)
{
    (void)time;
    printf("%s", text);
}

void printf_to_buf_static_string(uint32_t time, const char *text)
{
    (void)time;
    debug__write_console_text(text);
}

void debug__printf_to_buf_append_time(uint32_t time, const char *format, ...)
{
    va_list args;
    va_start(args, format);

#if (FORMAT_TIMESTAMPS_WITH_COMMAS == 1)
#define timestmap_char_buffer_SIZE 32
    char timestmap_char_buffer[timestmap_char_buffer_SIZE];

    uint32_t timestamp = time;
    if (timestamp == 0)
    {
        timestamp = RAIL_GetTime();
    }

    debug__format_timestamp_with_commas(timestamp, timestmap_char_buffer,
                                       timestmap_char_buffer_SIZE);
    printf("(t:%s) ", timestmap_char_buffer);
#else
    printf("(t:%u) ", time);
#endif

    vprintf(format, args);

    va_end(args);
}

void printf_to_buf_array(const uint8_t *data, uint32_t data_length, bool line_break_at_end)
{
    (void)data;
    (void)data_length;
    (void)line_break_at_end;
}
#endif

// -----------------------------------------------------------------------------
//                     Print Buffer Queue End
// -----------------------------------------------------------------------------
