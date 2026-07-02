#include "print.h"

#include "rail.h"

#include <stdarg.h>

// The goal of this entire library is to buffer printf statements so that they can be sent over serial at a more convenient time
// This lets printf be used better in blocking functions without significantly affecting real-time operations
// Useful for very slow baud rates
// At fast baud rates, I don't think it's even helping, printf() is so slow that it's probably negligible and easier to just do the printf() and not waste time with the buffering system
// Also this is a terrible use of memory...

// Static buffer to hold print statements
// Should probably implement a circular buffer or memory pool. For now just a simple array. Don't have the time to make it fancy
#if (DEBUG_PRINT_TO_BUFFERS == 1)
volatile debug__print_buffer_t debug__print_buffers[NUMBER_OF_PRINT_BUFFERS];
#endif

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

#if (DEBUG_PRINT_TO_BUFFERS == 1)
void debug__check_print_buffers_and_print(void)
{
    uint32_t lowest_timestamp_index = NUMBER_OF_PRINT_BUFFERS;
    long long unsigned int lowest_timestamp = -1;

    for (uint32_t i = 0; i < NUMBER_OF_PRINT_BUFFERS; i++)
    {
        if (debug__print_buffers[i].used == true)
        {
            if (debug__print_buffers[i].timestamp_ticks < lowest_timestamp)
            {
                lowest_timestamp = debug__print_buffers[i].timestamp_ticks;
                lowest_timestamp_index = i;
            }
        }
    }

    if (lowest_timestamp_index < NUMBER_OF_PRINT_BUFFERS)
    {
        printf("%s", (char *)debug__print_buffers[lowest_timestamp_index].print_text);
        debug__print_buffers[lowest_timestamp_index].used = false;
    }
}
#else
// Do nothing if buffered printing is disabled
void debug__check_print_buffers_and_print(void)
{
    (void)debug__check_print_buffers_and_print();
    return;
}
#endif

#if (DEBUG_PRINT_TO_BUFFERS == 1)
void debug__print_all_buffers(void)
{
    for (uint32_t i = 0; i < NUMBER_OF_PRINT_BUFFERS; i++)
    {
        debug__check_print_buffers_and_print();
    }
}
#else
void debug__print_all_buffers(void)
{
    (void)debug__print_all_buffers();
    return;
}
#endif

#if (DEBUG_PRINT_TO_BUFFERS == 1)
// Iterates through and array and turns it into a string then prints
// If the array is large, it gets broken down into multiple chunks that can fit into the buffers
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
            temp[output_index++] = ' ';
    }

    if (output_index > 0)
    {
        if(line_break_at_end)
        {
            temp[output_index++] = '\n';
        }
        temp[output_index] = '\0';
        printf_to_buf(0,"%s", temp);
    }
    else
    {
        if(line_break_at_end)
        {
            printf_to_buf(0,"\n\0");
        }
    }
}
#else
// Do nothing if buffered printing is disabled
void printf_to_buf_array(const uint8_t *data, uint32_t data_length, bool line_break_at_end)
{
    (void)printf_to_buf_array(data, data_length, line_break_at_end);
    return;
}
#endif

#if (DEBUG_PRINT_TO_BUFFERS == 1)
// Add print statement to buffer
void printf_to_buf(uint32_t time, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    uint8_t available_buffer_index = NUMBER_OF_PRINT_BUFFERS;

    for (uint32_t i = 0; i < NUMBER_OF_PRINT_BUFFERS; i++)
    {
        if (debug__print_buffers[i].used == false)
        {
            available_buffer_index = i;
            break;
        }
    }

    if(available_buffer_index == NUMBER_OF_PRINT_BUFFERS)
    {
        // No available buffer, drop the print
        CORE_EXIT_CRITICAL();
        va_end(args);
        return;
    }
    debug__print_buffers[available_buffer_index].used = true;

    CORE_EXIT_CRITICAL();

            vsnprintf((char *)&debug__print_buffers[available_buffer_index].print_text[0], sizeof(debug__print_buffers[available_buffer_index].print_text), format, args);
            
            if (time == 0)
            {
                // Storing CYCCNT for chronological order of prints.
                // In theory 2 prints could happen on the same microsecond
                debug__print_buffers[available_buffer_index].timestamp_ticks = DWT->CYCCNT;
            }
            else
            {
                debug__print_buffers[available_buffer_index].timestamp_ticks = time;
            }

    va_end(args);
}
#else
void printf_to_buf(uint32_t time, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
}
#endif


#if (DEBUG_PRINT_TO_BUFFERS == 1)
// Add print statement to buffer and add timestamp
void debug__printf_to_buf_append_time(uint32_t time, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    uint8_t available_buffer_index = NUMBER_OF_PRINT_BUFFERS;

    for (uint32_t i = 0; i < NUMBER_OF_PRINT_BUFFERS; i++)
    {
        if (debug__print_buffers[i].used == false)
        {
            available_buffer_index = i;
            break;
        }
    }

    if(available_buffer_index == NUMBER_OF_PRINT_BUFFERS)
    {
        // No available buffer, drop the print
        CORE_EXIT_CRITICAL();
        va_end(args);
        return;
    }
    debug__print_buffers[available_buffer_index].used = true;

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
    debug__format_timestamp_with_commas(timestamp, timestmap_char_buffer, timestmap_char_buffer_SIZE);
    offset_for_timestamp = snprintf((char *)debug__print_buffers[available_buffer_index].print_text, sizeof(debug__print_buffers[available_buffer_index].print_text),"(t:%s) ", timestmap_char_buffer); // Append time to start
#else
    offset_for_timestamp = snprintf(debug_print_buffers[available_buffer_index].print_text, sizeof(debug_print_buffers[available_buffer_index].print_text), "(t:%u)", timestamp); // Append time to start
#endif

    size_t max_len = sizeof(debug__print_buffers[available_buffer_index].print_text) - offset_for_timestamp;
    vsnprintf((char *)&debug__print_buffers[available_buffer_index].print_text[offset_for_timestamp],
              max_len,
              format,
              args);

    // Storing CYCCNT for chronological order of prints.
    // In theory 2 prints could happen on the same microsecond
    if (time == 0)
    {
        debug__print_buffers[available_buffer_index].timestamp_ticks = DWT->CYCCNT;
    }
    else
    {
        debug__print_buffers[available_buffer_index].timestamp_ticks = time;
    }   

    va_end(args);
}
#else
void debug__printf_to_buf_append_time(uint32_t time, const char *format, ...)
{
    va_list args;
    va_start(args, format);

#if (FORMAT_TIMESTAMPS_WITH_COMMAS == 1)
#define timestmap_char_buffer_SIZE 32
    char timestmap_char_buffer[timestmap_char_buffer_SIZE];

    uint32_t timestamp = time;;
    if (timestamp == 0)
    {
        timestamp = RAIL_GetTime();
    }  

    debug__format_timestamp_with_commas(timestamp, timestmap_char_buffer, timestmap_char_buffer_SIZE);
    uint32_t length = printf("(t:%s) ", timestmap_char_buffer); // Append time to start
#else
    uint32_t length = printf("(t:%u) ", timestamp); // Append time to start
#endif

    vprintf(format, args);

    va_end(args);
}
#endif
