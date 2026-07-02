#include "ui_interfacing.h"
#include "button.h"
#include "RGB.h"
#include "scheduler.h"
#include "radio_base.h"

static uint32_t button_press_start_timestamp = 0;

#define BUTTON_PRESS_DEBOUNCE_MS 50U
#define BUTTON_LONG_PRESS_THRESHOLD_MS 1000U

void button__rising_edge(void)
{
  button_press_start_timestamp = scheduler__get_millisecond_ticks();
}

void button__falling_edge(void)
{

  uint32_t press_duration_ms = scheduler__get_millisecond_ticks() - button_press_start_timestamp;
  if (press_duration_ms < BUTTON_PRESS_DEBOUNCE_MS)
  {
    return;
  }

  if (press_duration_ms >= BUTTON_LONG_PRESS_THRESHOLD_MS)
  {
  }
  else
  {
    radio__request_increment_channel();
    rgb__start_radio_status_blink_force();
  }
}