#include "audio_buffers.h"

#include "scheduler.h"

#include <assert.h>
#include <string.h>

// TODO
// Really the perfect use case for C++...
// Should get rid of duplication
// Can probably use structs and function pointers but will be a mess to debug

audio_buffer_t audio_buffers_right[NUMBER_OF_AUDIO_BUFFERS];
audio_buffer_t audio_buffers_left[NUMBER_OF_AUDIO_BUFFERS];

typedef enum audio_buffers_counter_index_s {
  AUDIO_BUFFERS_COUNTER_LEFT_WRITTEN = 0,
  AUDIO_BUFFERS_COUNTER_RIGHT_WRITTEN,
  AUDIO_BUFFERS_COUNTER_LEFT_CONSUMED,
  AUDIO_BUFFERS_COUNTER_RIGHT_CONSUMED,
  AUDIO_BUFFERS_COUNTER_PACKET_OVERFLOWS,
  AUDIO_BUFFERS_NUMBER_OF_COUNTERS
} audio_buffers_counter_index_t;

static volatile uint32_t audio_buffer_counter_values[AUDIO_BUFFERS_NUMBER_OF_COUNTERS] = {0};
static const char *audio_buffer_counter_names[AUDIO_BUFFERS_NUMBER_OF_COUNTERS] = {
    "left_buffers_written",
    "right_buffers_written",
    "left_buffers_consumed",
    "right_buffers_consumed",
    "packet_buffer_overflows",
};

uint32_t audio_buffer_head_left;
uint32_t audio_buffer_head_right;

bool left_data_waiting = false;
bool right_data_waiting = false;

bool stereo_mode = false;

__attribute__((weak)) bool audio_buffers__process_packet(uint8_t *left_or_first_buffer, uint8_t *right_or_second_buffer, bool is_stereo)
{
    (void)left_or_first_buffer;
    (void)right_or_second_buffer;
    (void)is_stereo;
    assert(0);
    return false;
}

__attribute__((weak)) void audio_buffers__printf(bool add_timestamp, const char *format, ...)
{
    (void)add_timestamp;
    (void)format;
}

// bool is_stereo_mode(void)
// {
//     return stereo_mode;
// }

void audio_buffers__set_stereo_mode(bool enable)
{
    stereo_mode = enable;
}

uint32_t audio_buffers__get_number_of_counters(void)
{
    return AUDIO_BUFFERS_NUMBER_OF_COUNTERS;
}

const char *audio_buffers__get_counter_name(uint32_t counter_index)
{
    if (counter_index >= AUDIO_BUFFERS_NUMBER_OF_COUNTERS)
    {
        return NULL;
    }

    return audio_buffer_counter_names[counter_index];
}

volatile uint32_t *audio_buffers__get_counter_value(uint32_t counter_index)
{
    if (counter_index >= AUDIO_BUFFERS_NUMBER_OF_COUNTERS)
    {
        return NULL;
    }

    return &audio_buffer_counter_values[counter_index];
}

void audio_buffers__reset_counters(void)
{
    for (uint32_t i = 0; i < AUDIO_BUFFERS_NUMBER_OF_COUNTERS; i++)
    {
        audio_buffer_counter_values[i] = 0;
    }
}

static void radio_buffers__advance_left_audio_buffer_head(void)
{
    audio_buffer_head_left++;
    if (audio_buffer_head_left >= NUMBER_OF_AUDIO_BUFFERS)
    {
        audio_buffer_head_left = 0;
    }

    if (audio_buffers_left[audio_buffer_head_left].used == true)
    {
        // Overflow
        audio_buffer_counter_values[AUDIO_BUFFERS_COUNTER_PACKET_OVERFLOWS]++;
    }
}

static void radio_buffers__advance_right_audio_buffer_head(void)
{
    audio_buffer_head_right++;
    if (audio_buffer_head_right >= NUMBER_OF_AUDIO_BUFFERS)
    {
        audio_buffer_head_right = 0;
    }

    if (audio_buffers_right[audio_buffer_head_right].used == true)
    {
        // Overflow
        audio_buffer_counter_values[AUDIO_BUFFERS_COUNTER_PACKET_OVERFLOWS]++;
    }
}

/**
 * @brief Initializes the audio buffer system.
 * Clears all left and right audio buffers and resets head pointers to 0.
 * Called once at startup before audio data is received.
 * 
 * @param None
 * @return void
 */
void audio_buffers__init(bool is_stereo)
{
    stereo_mode = is_stereo;
    memset(audio_buffers_right, 0, sizeof(audio_buffers_right));
    memset(audio_buffers_left, 0, sizeof(audio_buffers_left));
    audio_buffer_head_right = 0;
    audio_buffer_head_left = 0;
    left_data_waiting = false;
    right_data_waiting = false;
    audio_buffers__reset_counters();
}

/**
 * @brief Adds new left channel audio data to the buffer queue.
 * Copies data into current left buffer, marks as used, timestamps it,
 * advances head pointer, and increments debug counter.
 * 
 * @param data_pointer - Pointer to left channel audio samples (RADIO_PACKET_DATA_SIZE_PER_CHANNEL bytes)
 * @return void
 */
void audio_buffers__add_new_data_to_left_buffer(uint8_t *data_pointer)
{
    memcpy(audio_buffers_left[audio_buffer_head_left].audio_buffer, data_pointer, RADIO_PACKET_DATA_SIZE_PER_CHANNEL);
    audio_buffers_left[audio_buffer_head_left].used = true;
    audio_buffers_left[audio_buffer_head_left].micros_timestamp = scheduler__get_microsecond_ticks();
    radio_buffers__advance_left_audio_buffer_head();
    audio_buffer_counter_values[AUDIO_BUFFERS_COUNTER_LEFT_WRITTEN]++;
}

/**
 * @brief Adds new right channel audio data to the buffer queue.
 * Copies data into current right buffer, marks as used, timestamps it,
 * advances head pointer, and increments debug counter.
 * Called from ADC/I2S DMA callbacks when new samples arrive.
 * 
 * @param data_pointer - Pointer to right channel audio samples (RADIO_PACKET_DATA_SIZE_PER_CHANNEL bytes)
 * @return void
 */
void audio_buffers__add_new_data_to_right_buffer(uint8_t *data_pointer)
{
    memcpy(audio_buffers_right[audio_buffer_head_right].audio_buffer, data_pointer, RADIO_PACKET_DATA_SIZE_PER_CHANNEL);
    audio_buffers_right[audio_buffer_head_right].used = true;
    audio_buffers_right[audio_buffer_head_right].micros_timestamp = scheduler__get_microsecond_ticks();
    radio_buffers__advance_right_audio_buffer_head();
    audio_buffer_counter_values[AUDIO_BUFFERS_COUNTER_RIGHT_WRITTEN]++;
    // counters__add_to_counter(samples_received_right, RADIO_PACKET_DATA_SIZE_PER_CHANNEL >> 1);
    //audio_buffers__printf("wrote the right buffer: %u\n", (unsigned int)audio_buffer_head_right);
}

static bool audio_buffers__check_if_any_left_audio_data_ready(void)
{
    for (uint8_t i = 0; i < NUMBER_OF_AUDIO_BUFFERS; i++)
    {
        if (audio_buffers_left[i].used == true)
        {
            return true;
        }
    }
    return false;
}

static bool audio_buffers__get_oldest_left_audio_data_buffer_index(uint32_t *oldest_data_buffer_index)
{
    uint32_t oldest_index_timestamp = 0xFFFFFFFF;
    bool found = false;

    for (uint8_t i = 0; i < NUMBER_OF_AUDIO_BUFFERS; i++)
    {
        if (audio_buffers_left[i].used == true)
        {
            if (audio_buffers_left[i].micros_timestamp < oldest_index_timestamp)
            {
                oldest_index_timestamp = audio_buffers_left[i].micros_timestamp;
                *oldest_data_buffer_index = i;
                found = true;
            }
        }
    }

    return found;
}

static bool audio_buffers__get_oldest_right_audio_data_buffer_index(uint32_t *oldest_data_buffer_index)
{
    uint32_t oldest_index_timestamp = 0xFFFFFFFF;
    bool found = false;

    for (uint8_t i = 0; i < NUMBER_OF_AUDIO_BUFFERS; i++)
    {
        if (audio_buffers_right[i].used == true)
        {
            if (audio_buffers_right[i].micros_timestamp < oldest_index_timestamp)
            {
                oldest_index_timestamp = audio_buffers_right[i].micros_timestamp;
                *oldest_data_buffer_index = i;
                found = true;
            }
        }
    }

    return found;
}

// count_for_valid used for mono mode where 2 packets are needed to create a full packet buffer
static bool audio_buffers__check_if_any_right_audio_data_ready(uint32_t count_for_valid)
{
    uint32_t count_found = 0;
    for (uint8_t i = 0; i < NUMBER_OF_AUDIO_BUFFERS; i++)
    {
        if (audio_buffers_right[i].used == true)
        {
            count_found++;
        }
        if (count_found >= count_for_valid)
        {
            return true;
        }
    }
    return false;
}

static bool audio_buffers__is_left_audio_data_ready(void)
{
    if (left_data_waiting == true)
    {
        return true;
    }

    left_data_waiting = audio_buffers__check_if_any_left_audio_data_ready();
    return left_data_waiting;
}

static bool audio_buffers__is_right_audio_data_ready(void)
{
    if (right_data_waiting == true)
    {
        return true;
    }

    if (stereo_mode == true)
    {
        right_data_waiting = audio_buffers__check_if_any_right_audio_data_ready(1);
    }
    else
    {
        right_data_waiting = audio_buffers__check_if_any_right_audio_data_ready(2);
    }
    return right_data_waiting;
}

void audio_buffers__get_first_and_second_oldest_mono_data_buffers(uint32_t *first_oldest_data_buffer_index, uint32_t *second_oldest_data_buffer_index)
{
    uint32_t first_oldest_index_timestamp = 0xFFFFFFFF;
    uint32_t second_oldest_index_timestamp = 0xFFFFFFFF;

    for (uint8_t i = 0; i < NUMBER_OF_AUDIO_BUFFERS; i++)
    {
        if (audio_buffers_right[i].used == true)
        {
            if (audio_buffers_right[i].micros_timestamp < first_oldest_index_timestamp)
            {
                second_oldest_index_timestamp = first_oldest_index_timestamp;
                *second_oldest_data_buffer_index = *first_oldest_data_buffer_index;

                first_oldest_index_timestamp = audio_buffers_right[i].micros_timestamp;
                *first_oldest_data_buffer_index = i;
            }
            else if (audio_buffers_right[i].micros_timestamp < second_oldest_index_timestamp)
            {
                second_oldest_index_timestamp = audio_buffers_right[i].micros_timestamp;
                *second_oldest_data_buffer_index = i;
            }
        }
    }
}

static void audio_buffers__right_clear_buffer(uint32_t index)
{
    if (index < NUMBER_OF_AUDIO_BUFFERS)
    {
        memset(&audio_buffers_right[index], 0, sizeof(audio_buffer_t));
    }
    // memset(audio_buffers_right[index], 0, sizeof(audio_buffer_t));
    // for (uint8_t i = 0; i < NUMBER_OF_AUDIO_BUFFERS; i++)
    // {

    // }
}

static void audio_buffers__left_clear_buffer(uint32_t index)
{
    if (index < NUMBER_OF_AUDIO_BUFFERS)
    {
        memset(&audio_buffers_left[index], 0, sizeof(audio_buffer_t));
    }
}

/**
 * @brief Main audio buffer process loop called from scheduler.
 * In stereo mode: waits for both left and right data and creates packet.
 * In mono mode: waits for two right buffers (combined into one packet).
 * Clears consumed buffers and increments debug counters.
 * Calls radio__transmit_create_new_packet_buffer() when ready.
 * 
 * @param None
 * @return true if packet was created, false otherwise
 */
bool audio_buffers__run_process(void)
{
    if (stereo_mode)
    {
        if (audio_buffers__is_left_audio_data_ready() && audio_buffers__is_right_audio_data_ready())
        {
            uint32_t left_buffer_to_send_index = 0xFFFFFFFF;
            uint32_t right_buffer_to_send_index = 0xFFFFFFFF;

            if (!audio_buffers__get_oldest_left_audio_data_buffer_index(&left_buffer_to_send_index))
            {
                return false;
            }

            if (!audio_buffers__get_oldest_right_audio_data_buffer_index(&right_buffer_to_send_index))
            {
                return false;
            }

            if (audio_buffers__process_packet(audio_buffers_left[left_buffer_to_send_index].audio_buffer,
                                              audio_buffers_right[right_buffer_to_send_index].audio_buffer,
                                              true))
            {
                audio_buffer_counter_values[AUDIO_BUFFERS_COUNTER_LEFT_CONSUMED]++;
                audio_buffer_counter_values[AUDIO_BUFFERS_COUNTER_RIGHT_CONSUMED]++;
                audio_buffers__left_clear_buffer(left_buffer_to_send_index);
                audio_buffers__right_clear_buffer(right_buffer_to_send_index);
                left_data_waiting = false;
                right_data_waiting = false;
            }
        }
    }
    else
    {
        if (audio_buffers__is_right_audio_data_ready())
        {
            uint32_t first_buffer_to_send_index = 0xFFFFFFFF;
            uint32_t second_buffer_to_send_index = 0xFFFFFFFF;
            audio_buffers__get_first_and_second_oldest_mono_data_buffers(&first_buffer_to_send_index, &second_buffer_to_send_index);

            if (!((first_buffer_to_send_index < NUMBER_OF_AUDIO_BUFFERS) && (second_buffer_to_send_index < NUMBER_OF_AUDIO_BUFFERS)))
            {
                // this shouldn't be possible
                //audio_buffers__printf("Error getting mono buffers to send %u %u\n", (unsigned int)first_buffer_to_send_index, (unsigned int)second_buffer_to_send_index);
                return false;
            }

            // add to packet buffer
            audio_buffer_counter_values[AUDIO_BUFFERS_COUNTER_RIGHT_CONSUMED] += 2;

            //audio_buffers__printf("Sent Buffers: %u , %u\n", (unsigned int)first_buffer_to_send_index, (unsigned int)second_buffer_to_send_index);
            right_data_waiting = false;
            //debug__log_TX_success(false);
            if (audio_buffers__process_packet(audio_buffers_right[first_buffer_to_send_index].audio_buffer, audio_buffers_right[second_buffer_to_send_index].audio_buffer, false))
            {
                audio_buffers__right_clear_buffer(first_buffer_to_send_index);
                audio_buffers__right_clear_buffer(second_buffer_to_send_index);
            }
        }
    }

    return true;
}
