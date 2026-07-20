#include "counter_interface.h"

// #include "audio_buffers.h"
#include "counters_new.h"
#include "ADC.h"
#include "audio_intensity.h"
#include "app_process.h"
#include "radio_transmit.h"
#include "microseconds.h"
#include "scheduler.h"
#include "audio_pipeline.h"
#include "audio_ring_buffer.h"
#include "radio_statistics.h"
#include "tx_retry.h"

typedef struct counter_provider_s {
  uint32_t (*get_number_of_counters)(void);
  const char *(*get_counter_name)(uint32_t counter_index);
  volatile uint32_t *(*get_counter_value)(uint32_t counter_index);
} counter_provider_t;

typedef struct counter_reset_s {
  void (*reset_counters)(void);
} counter_reset_t;

static void counter_interface__register_provider(const counter_provider_t *provider)
{
  for (uint32_t counter_index = 0; counter_index < provider->get_number_of_counters(); counter_index++)
  {
    const char *counter_name = provider->get_counter_name(counter_index);
    volatile uint32_t *counter_value = provider->get_counter_value(counter_index);

    if ((counter_name != NULL) && (counter_value != NULL))
    {
      counters__register_counter(counter_name, (uint32_t *)counter_value);
    }
  }
}

static void counter_interface__reset_provider(const counter_reset_t *provider)
{
  provider->reset_counters();
}

static const counter_provider_t counter_providers[] = {
  { scheduler__get_number_of_counters, scheduler__get_counter_name, scheduler__get_counter_address },
  { microseconds__get_number_of_counters, microseconds__get_counter_name, microseconds__get_counter_address },
  { app_process__get_number_of_counters, app_process__get_counter_name, app_process__get_counter_address },
  { adc__get_number_of_counters, adc__get_counter_name, adc__get_counter_address },
  { audio_intensity__get_number_of_counters, audio_intensity__get_counter_name, audio_intensity__get_counter_address },
  { audio_pipeline__get_number_of_counters, audio_pipeline__get_counter_name, audio_pipeline__get_counter_address },
  // { audio_buffers__get_number_of_counters, audio_pipeline__get_counter_address`, audio_pipeline__get_counter_address },
  { ring_buffer__get_number_of_counters, ring_buffer__get_counter_name, ring_buffer__get_counter_address },
  { radio_transmit__get_number_of_counters, radio_transmit__get_counter_name, radio_transmit__get_counter_address },
  { radio_statistics__get_number_of_counters, radio_statistics__get_counter_name, radio_statistics__get_counter_address },
  //{ tx_retry__get_number_of_counters, tx_retry__get_counter_name, tx_retry__get_counter_address },
};

static const counter_reset_t counter_resets[] = {
  { app_process__reset_counters },
  { radio_transmit__reset_counters },
  { scheduler__reset_counters },
  { adc__reset_counters },
  { audio_intensity__reset_counters },
  { audio_pipeline__reset_counters },
  // { audio_buffers__reset_counters },
  { ring_buffer__reset_counters },
  { radio_statistics__reset_counters },
  //{ tx_retry__reset_counters },
};


void counter_interface__init(void)
{
    counter_interface__register_counters();
}

void counter_interface__register_counters(void)
{
  for (uint32_t provider_index = 0; provider_index < (sizeof(counter_providers) / sizeof(counter_providers[0])); provider_index++)
  {
    counter_interface__register_provider(&counter_providers[provider_index]);
  }
}

// -----------------------------------------------------------------------------
//                     Weak function implementations, do not rename.
// -----------------------------------------------------------------------------

void counters__process_counter_reset(void)
{
  for (uint32_t reset_index = 0; reset_index < (sizeof(counter_resets) / sizeof(counter_resets[0])); reset_index++)
  {
    counter_interface__reset_provider(&counter_resets[reset_index]);
  }
}

void counters__pre_save_hook(void)
{
  microseconds__trigger_counter_update();
  radio_statistics__prepare_counters_for_print();
}

void counters__post_print_hook(void)
{
  radio_statistics__reset_counters();
  counters__printf(true, "Details:\n");
}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------