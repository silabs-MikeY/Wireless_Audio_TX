#include "audio_intensity.h"

#define VOLTAGE_RANGE_mV 3000          // 3V p-p
#define LOW_LEVEL_THRESHOLD_mV 100     // 0.1V
#define MEDIUM_LEVEL_THRESHOLD_mV 1500 // 1.5V
#define HIGH_LEVEL_THRESHOLD_mV 2900   // 2.9V

#define AUDIO_INTENSITY_NUMBER_OF_COUNTERS 3

enum {
  AUDIO_INTENSITY_COUNTER_MAX = 0,
  AUDIO_INTENSITY_COUNTER_MIN = 1,
  AUDIO_INTENSITY_COUNTER_AVERAGE = 2,
};

uint8_t high_level_threshold_upper = 0;
uint8_t medium_level_threshold_upper = 0;
uint8_t low_level_threshold_upper = 0;

uint8_t high_level_threshold_lower = 0;
uint8_t medium_level_threshold_lower = 0;
uint8_t low_level_threshold_lower = 0;

uint32_t last_timestamp_low_threshold_reached = 0;
uint32_t last_timestamp_medium_threshold_reached = 0;
uint32_t last_timestamp_high_threshold_reached = 0;

uint32_t microsecond_timeout = 0;

uint32_t sample_size_bytes = 0;
bool little_endian_flag = false;
uint32_t buffer_size_bytes = 0;

static volatile uint32_t audio_intensity_counter_values[AUDIO_INTENSITY_NUMBER_OF_COUNTERS] = {0};
static uint64_t audio_intensity_sum = 0;
static uint32_t audio_intensity_sample_count = 0;

static bool library_initialized = false;

// volatile uint32_t current_threshold_reached = 0;
// volatile uint32_t new_threshold_reached = 0;
// volatile uint32_t current_threshold_reached_timestamp = 0;
// static uint32_t latest_audio_level_min_mv = 0;
// static uint32_t latest_audio_level_max_mv = 0;

__attribute__((weak)) uint32_t audio_intensity__get_microsecond_ticks(void) {
  return 0;
}

static void audio_intensity__update_counters(uint32_t intensity)
{
  if (audio_intensity_sample_count == 0)
  {
    audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_MIN] = intensity;
    audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_MAX] = intensity;
  }
  else
  {
    if (intensity > audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_MAX])
    {
      audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_MAX] = intensity;
    }

    if (intensity < audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_MIN])
    {
      audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_MIN] = intensity;
    }
  }

  audio_intensity_sum += intensity;
  audio_intensity_sample_count++;
  audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_AVERAGE] =
      (uint32_t)(audio_intensity_sum / audio_intensity_sample_count);
}

static uint32_t audio_intensity__raw_magnitude_to_mv(uint32_t raw_magnitude)
{
  uint64_t full_scale_magnitude = 0;

  if (sample_size_bytes == 1)
  {
    full_scale_magnitude = 128ULL;
  }
  else if (sample_size_bytes == 2)
  {
    full_scale_magnitude = 32768ULL;
  }
  else if (sample_size_bytes == 4)
  {
    full_scale_magnitude = 2147483648ULL;
  }

  if (full_scale_magnitude == 0)
  {
    return 0;
  }

  return (uint32_t)(((uint64_t)raw_magnitude * VOLTAGE_RANGE_mV) /
                    full_scale_magnitude);
}

static uint32_t audio_intensity__sample_to_magnitude(uint32_t sample_value)
{
  if (sample_size_bytes == 1)
  {
    int8_t signed_sample = (int8_t)sample_value;
    return (signed_sample < 0) ? (uint32_t)(-signed_sample) : (uint32_t)signed_sample;
  }

  if (sample_size_bytes == 2)
  {
    int16_t signed_sample = (int16_t)sample_value;
    return (signed_sample < 0) ? (uint32_t)(-signed_sample) : (uint32_t)signed_sample;
  }

  if (sample_size_bytes == 4)
  {
    int32_t signed_sample = (int32_t)sample_value;
    return (signed_sample < 0) ? (uint32_t)(-(int64_t)signed_sample) : (uint32_t)signed_sample;
  }

  return 0;
}

bool audio_intensity__init(bool little_endian_flag_param,
                           uint32_t sample_size_bytes_param,
                           uint32_t buffer_size_bytes_param) {
  little_endian_flag = little_endian_flag_param;
  sample_size_bytes = sample_size_bytes_param;
  buffer_size_bytes = buffer_size_bytes_param;

  // Check if the buffer size is a multiple of the sample size or 0. If not,
  // return error.
  if (((buffer_size_bytes % sample_size_bytes) != 0) &&
      (buffer_size_bytes != 0)) {
    return false;
  }

  if (sample_size_bytes == 1) {
    high_level_threshold_upper =
        (HIGH_LEVEL_THRESHOLD_mV * 0xFFUL / VOLTAGE_RANGE_mV);
    medium_level_threshold_upper =
        (MEDIUM_LEVEL_THRESHOLD_mV * 0xFFUL / VOLTAGE_RANGE_mV);
    low_level_threshold_upper =
        (LOW_LEVEL_THRESHOLD_mV * 0xFFUL / VOLTAGE_RANGE_mV);

    high_level_threshold_lower = (0xFFUL - high_level_threshold_upper);
    medium_level_threshold_lower = (0xFFUL - medium_level_threshold_upper);
    low_level_threshold_lower = (0xFFUL - low_level_threshold_upper);
  } else if (sample_size_bytes == 2) {
    high_level_threshold_upper =
        (HIGH_LEVEL_THRESHOLD_mV * 0xFFFFUL / VOLTAGE_RANGE_mV) >> 8;
    medium_level_threshold_upper =
        (MEDIUM_LEVEL_THRESHOLD_mV * 0xFFFFUL / VOLTAGE_RANGE_mV) >> 8;
    low_level_threshold_upper =
        (LOW_LEVEL_THRESHOLD_mV * 0xFFFFUL / VOLTAGE_RANGE_mV) >> 8;

    high_level_threshold_lower = (0xFFFFUL - high_level_threshold_upper) >> 8;
    medium_level_threshold_lower =
        (0xFFFFUL - medium_level_threshold_upper) >> 8;
    low_level_threshold_lower = (0xFFFFUL - low_level_threshold_upper) >> 8;
  } else if (sample_size_bytes == 4) {
    high_level_threshold_upper =
        (HIGH_LEVEL_THRESHOLD_mV * 0xFFFFFFFFUL / VOLTAGE_RANGE_mV) >> 24;
    medium_level_threshold_upper =
        (MEDIUM_LEVEL_THRESHOLD_mV * 0xFFFFFFFFUL / VOLTAGE_RANGE_mV) >> 24;
    low_level_threshold_upper =
        (LOW_LEVEL_THRESHOLD_mV * 0xFFFFFFFFUL / VOLTAGE_RANGE_mV) >> 24;

    high_level_threshold_lower =
        (0xFFFFFFFFUL - high_level_threshold_upper) >> 24;
    medium_level_threshold_lower =
        (0xFFFFFFFFUL - medium_level_threshold_upper) >> 24;
    low_level_threshold_lower =
        (0xFFFFFFFFUL - low_level_threshold_upper) >> 24;
  } else {
    return false; // Unsupported sample size
  }

  last_timestamp_low_threshold_reached = 0;
  last_timestamp_medium_threshold_reached = 0;
  last_timestamp_high_threshold_reached = 0;

  microsecond_timeout = 100000; // Hold threshold reached for 100ms

  library_initialized = true;
  return true;
}

uint32_t audio_intensity__get_intensity(void) {
  uint32_t current_time = audio_intensity__get_microsecond_ticks();

  if ((current_time - last_timestamp_high_threshold_reached) <
      microsecond_timeout) {
    return HIGH_THRESHOLD;
  }

  if ((current_time - last_timestamp_medium_threshold_reached) <
      microsecond_timeout) {
    return MEDIUM_THRESHOLD;
  }

  if ((current_time - last_timestamp_low_threshold_reached) <
      microsecond_timeout) {
    return LOW_THRESHOLD;
  }

  return 0;
}

int32_t audio_intensity__check_buffer(uint8_t *buffer) {
  (void)buffer;

  if (buffer == NULL) {
    return -1;
  }

  // Buffer pointer needs to be alligned to number of sample bytes. If not,
  // return error.
  if (((uintptr_t)buffer % sample_size_bytes) != 0) {
    return -2;
  }

  if (!library_initialized) {
    return -3;
  }

  uint32_t highest_signal_level = 0;

  uint8_t *buffer_pointer = (uint8_t *)buffer;

  // Decode signed PCM samples, then track absolute peak and trough magnitudes.
  for (uint32_t i = 0; i < buffer_size_bytes / sample_size_bytes; i++) {
    uint8_t *sample_pointer = buffer_pointer + (i * sample_size_bytes);
    uint32_t sample_value = 0;

    // Get the sample value based on the sample size. This assumes that the
    // sample size is either 1, 2, or 4 bytes. If it's not one of these sizes,
    // return an error.
    if (sample_size_bytes == 1) {
      sample_value = *sample_pointer;
    } else if (sample_size_bytes == 2) {
      sample_value = *(uint16_t *)sample_pointer;
    } else if (sample_size_bytes == 4) {
      sample_value = *(uint32_t *)sample_pointer;
    } else {
      return -4; // Unsupported sample size
    }

    // Pull out just the Most Significant Byte
    if (little_endian_flag) {
      // Rotate the bytes to convert from little-endian to big-endian
      if (sample_size_bytes == 2) {
        sample_value = (sample_value >> 8) | (sample_value << 8);
      } else if (sample_size_bytes == 4) {
        sample_value = ((sample_value >> 24) & 0x000000FF) |
                       ((sample_value >> 8) & 0x0000FF00) |
                       ((sample_value << 8) & 0x00FF0000) |
                       ((sample_value << 24) & 0xFF000000);
      }
    }

    uint32_t sample_magnitude = audio_intensity__sample_to_magnitude(sample_value);

    if (sample_magnitude > highest_signal_level) {
      highest_signal_level = sample_magnitude;
    }

  }

  uint32_t highest_signal_level_mV = audio_intensity__raw_magnitude_to_mv(highest_signal_level);

  if (highest_signal_level_mV >= HIGH_LEVEL_THRESHOLD_mV) {
    last_timestamp_high_threshold_reached =
        audio_intensity__get_microsecond_ticks();
  } else if (highest_signal_level_mV >= MEDIUM_LEVEL_THRESHOLD_mV) {
    last_timestamp_medium_threshold_reached =
        audio_intensity__get_microsecond_ticks();
  } else if (highest_signal_level_mV >= LOW_LEVEL_THRESHOLD_mV) {
    last_timestamp_low_threshold_reached =
        audio_intensity__get_microsecond_ticks();
  }

  audio_intensity__update_counters(highest_signal_level_mV);
  return 0;
}

uint32_t audio_intensity__get_number_of_counters(void)
{
  return AUDIO_INTENSITY_NUMBER_OF_COUNTERS;
}

const char *audio_intensity__get_counter_name(uint32_t counter_index)
{
  switch (counter_index)
  {
  case AUDIO_INTENSITY_COUNTER_MAX:
    return "audio_intensity_max";
  case AUDIO_INTENSITY_COUNTER_MIN:
    return "audio_intensity_min";
  case AUDIO_INTENSITY_COUNTER_AVERAGE:
    return "audio_intensity_average";
  default:
    return NULL;
  }
}

volatile uint32_t *audio_intensity__get_counter_address(uint32_t counter_index)
{
  if (counter_index >= AUDIO_INTENSITY_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return &audio_intensity_counter_values[counter_index];
}

void audio_intensity__reset_counters(void)
{
  audio_intensity_sum = 0;
  audio_intensity_sample_count = 0;
  audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_MAX] = 0;
  audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_MIN] = 0;
  audio_intensity_counter_values[AUDIO_INTENSITY_COUNTER_AVERAGE] = 0;
}
