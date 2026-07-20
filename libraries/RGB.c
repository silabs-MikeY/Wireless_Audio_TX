#include <assert.h>

#include "RGB.h"
#include "hardware_config.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_timer.h"

#include "print.h"
#include "radio_base.h"
#include "scheduler.h"
#include "state_machine.h"
#include "timer_helper.h"

#define OUT_FREQ 0x0FFF

typedef struct {
  uint32_t timestamp;
  uint32_t state;
} radio_blink_state_change_t;

static bool radio_status_blink_running = false;
static radio_blink_state_change_t on_off_timestamps[20];
static uint32_t next_timestamp_index = 0;
static bool rgb__init_done = false;

void rgb__start_radio_status_blink_force(void)
{
  rgb__start_radio_status_blink(scheduler__get_millisecond_ticks());
  scheduler__reset_5s_countdown();
}

void rgb__start_radio_status_blink(uint32_t current_tick)
{
  if (rgb__init_done == false)
  {
    return;
  }

  radio_status_blink_running = true;

  uint32_t channel = radio__get_channel();
  uint32_t timestamps_logged = 0;
  uint32_t state_change_timestamp = current_tick;

  while (channel > 0)
  {
    if (channel > 4)
    {
      channel -= 5;

      on_off_timestamps[timestamps_logged].timestamp = state_change_timestamp;
      on_off_timestamps[timestamps_logged++].state = 1;
      state_change_timestamp += 400;

      on_off_timestamps[timestamps_logged].timestamp = state_change_timestamp;
      on_off_timestamps[timestamps_logged++].state = 0;
      state_change_timestamp += 400;
    }
    else
    {
      channel--;

      on_off_timestamps[timestamps_logged].timestamp = state_change_timestamp;
      on_off_timestamps[timestamps_logged++].state = 1;
      state_change_timestamp += 200;

      on_off_timestamps[timestamps_logged].timestamp = state_change_timestamp;
      on_off_timestamps[timestamps_logged++].state = 0;
      state_change_timestamp += 200;
    }
  }

  for (uint8_t i = (uint8_t)timestamps_logged; i < 20; i++)
  {
    on_off_timestamps[i].timestamp = 0xFFFFFFFF;
    on_off_timestamps[i].state = 0;
  }

  rgb__radio_blink_turn_on();
  next_timestamp_index = 1;
}

void rgb__run_radio_status_blink(uint32_t current_tick)
{
  if (radio_status_blink_running)
  {
    if (current_tick > on_off_timestamps[next_timestamp_index].timestamp)
    {
      if (on_off_timestamps[next_timestamp_index].state == 0)
      {
        rgb__set_rgb_out(1, black, true, 0);
      }
      else
      {
        rgb__set_rgb_out(1, green, true, 0x7F);
      }

      next_timestamp_index++;

      if (on_off_timestamps[next_timestamp_index].timestamp == 0xFFFFFFFF)
      {
        radio_status_blink_running = false;
      }
    }
  }
}

void rgb__radio_blink_turn_on(void)
{
  if (state_machine__get_state() == RUNNING)
  {
    rgb__set_rgb_out(1, green, true, 0x7F);
  }
  else
  {
    rgb__set_rgb_out(1, red, true, 0x7F);
  }
}

void rgb__timer_init(void)
{
  if (RGB1_TIMER == RGB2_TIMER)
  {
    debug__printf_to_buf_append_time(0, "Can't use the same timer for RGB1 and RGB2\n");
    assert(0);
  }

  if (timers__init_timer_cmu(RGB1_TIMER) == false)
  {
    debug__printf_to_buf_append_time(0, "Bad RGB1_TIMER choice\n");
    assert(0);
  }

  if (timers__init_timer_cmu(RGB2_TIMER) == false)
  {
    debug__printf_to_buf_append_time(0, "Bad RGB2_TIMER choice\n");
    assert(0);
  }

  uint32_t RGB1_timer_index = timers__get_timer_index(RGB1_TIMER);
  if (RGB1_timer_index == 0xFFFFFFFF)
  {
    debug__printf_to_buf_append_time(0, "Invalid RGB1 index\n");
    assert(0);
  }

  uint32_t RGB2_timer_index = timers__get_timer_index(RGB2_TIMER);
  if (RGB2_timer_index == 0xFFFFFFFF)
  {
    debug__printf_to_buf_append_time(0, "Invalid RGB2 index\n");
    assert(0);
  }

  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef rgb1_timerCCInit = TIMER_INITCC_DEFAULT;
  TIMER_InitCC_TypeDef rgb2_timerCCInit = TIMER_INITCC_DEFAULT;

  timerInit.enable = false;
  rgb1_timerCCInit.mode = timerCCModePWM;
  rgb1_timerCCInit.cmoa = timerOutputActionToggle;
  rgb1_timerCCInit.edge = timerEdgeBoth;
  rgb1_timerCCInit.outInvert = false;

  rgb2_timerCCInit.mode = timerCCModePWM;
  rgb2_timerCCInit.cmoa = timerOutputActionToggle;
  rgb2_timerCCInit.edge = timerEdgeBoth;
  rgb2_timerCCInit.outInvert = false;

  TIMER_InitCC(RGB1_TIMER, 0, &rgb1_timerCCInit);
  TIMER_InitCC(RGB1_TIMER, 1, &rgb1_timerCCInit);
  TIMER_InitCC(RGB1_TIMER, 2, &rgb1_timerCCInit);
  TIMER_InitCC(RGB2_TIMER, 0, &rgb2_timerCCInit);
  TIMER_InitCC(RGB2_TIMER, 1, &rgb2_timerCCInit);
  TIMER_InitCC(RGB2_TIMER, 2, &rgb2_timerCCInit);

  GPIO->TIMERROUTE[RGB1_timer_index].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN |
                                               GPIO_TIMER_ROUTEEN_CC1PEN |
                                               GPIO_TIMER_ROUTEEN_CC2PEN;
  GPIO->TIMERROUTE[RGB1_timer_index].CC0ROUTE = (RGB1_RED_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) |
                                               (RGB1_RED_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB1_timer_index].CC1ROUTE = (RGB1_GREEN_PORT << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT) |
                                               (RGB1_GREEN_PIN << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB1_timer_index].CC2ROUTE = (RGB1_BLUE_PORT << _GPIO_TIMER_CC2ROUTE_PORT_SHIFT) |
                                               (RGB1_BLUE_PIN << _GPIO_TIMER_CC2ROUTE_PIN_SHIFT);

  GPIO->TIMERROUTE[RGB2_timer_index].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN |
                                               GPIO_TIMER_ROUTEEN_CC1PEN |
                                               GPIO_TIMER_ROUTEEN_CC2PEN;
  GPIO->TIMERROUTE[RGB2_timer_index].CC0ROUTE = (RGB2_RED_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) |
                                               (RGB2_RED_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB2_timer_index].CC1ROUTE = (RGB2_GREEN_PORT << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT) |
                                               (RGB2_GREEN_PIN << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB2_timer_index].CC2ROUTE = (RGB2_BLUE_PORT << _GPIO_TIMER_CC2ROUTE_PORT_SHIFT) |
                                               (RGB2_BLUE_PIN << _GPIO_TIMER_CC2ROUTE_PIN_SHIFT);

  TIMER_TopSet(RGB1_TIMER, OUT_FREQ);
  TIMER_TopSet(RGB2_TIMER, OUT_FREQ);

  TIMER_CompareSet(RGB1_TIMER, 0, 0);
  TIMER_CompareSet(RGB1_TIMER, 1, 0);
  TIMER_CompareSet(RGB1_TIMER, 2, 0);
  TIMER_CompareSet(RGB2_TIMER, 0, 0);
  TIMER_CompareSet(RGB2_TIMER, 1, 0);
  TIMER_CompareSet(RGB2_TIMER, 2, 0);

  TIMER_Init(RGB1_TIMER, &timerInit);
  TIMER_Init(RGB2_TIMER, &timerInit);

  TIMER_Enable(RGB1_TIMER, true);
  TIMER_Enable(RGB2_TIMER, true);

  TIMER_IntEnable(RGB1_TIMER, TIMER_IEN_CC0);
  TIMER_IntEnable(RGB1_TIMER, TIMER_IEN_CC1);
  TIMER_IntEnable(RGB1_TIMER, TIMER_IEN_CC2);
  TIMER_IntEnable(RGB2_TIMER, TIMER_IEN_CC0);
  TIMER_IntEnable(RGB2_TIMER, TIMER_IEN_CC1);
  TIMER_IntEnable(RGB2_TIMER, TIMER_IEN_CC2);

  RGB1_TIMER->CC[0].OCB = 0;
  RGB1_TIMER->CC[1].OCB = 0;
  RGB1_TIMER->CC[2].OCB = 0;
  RGB2_TIMER->CC[0].OCB = 0;
  RGB2_TIMER->CC[1].OCB = 0;
  RGB2_TIMER->CC[2].OCB = 0;
}

void rgb__init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(RGB1_RED_PORT, RGB1_RED_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(RGB1_GREEN_PORT, RGB1_GREEN_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(RGB1_BLUE_PORT, RGB1_BLUE_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(RGB2_RED_PORT, RGB2_RED_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(RGB2_GREEN_PORT, RGB2_GREEN_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(RGB2_BLUE_PORT, RGB2_BLUE_PIN, gpioModePushPull, 0);

   rgb__timer_init();

#if (RGB2_STARTUP_TEST == 1)
  // rgb__set_rgb_out(1, green, true, 0x7F);
  // rgb__set_rgb_out(2, green, true, 0x7F);
#endif
  rgb__init_done = true;

  // CORE_DECLARE_IRQ_STATE;
  // CORE_ENTER_CRITICAL();
  rgb__set_rgb_out(1, white, true, 0x7F);
  rgb__set_rgb_out(2, white, true, 0x7F);
}

void rgb__deinit(void)
{
  if (timers__is_timer_enabled(RGB1_TIMER))
  {
    TIMER_Enable(RGB1_TIMER, false);
    TIMER_Reset(RGB1_TIMER);
    timers__deinit_timer_cmu(RGB1_TIMER);
  }

  if (timers__is_timer_enabled(RGB2_TIMER))
  {
    TIMER_Enable(RGB2_TIMER, false);
    TIMER_Reset(RGB2_TIMER);
    timers__deinit_timer_cmu(RGB2_TIMER);
  }

  if (CMU->CLKEN0 & CMU_CLKEN0_GPIO)
  {
    GPIO_PinModeSet(RGB1_RED_PORT, RGB1_RED_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(RGB1_GREEN_PORT, RGB1_GREEN_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(RGB1_BLUE_PORT, RGB1_BLUE_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(RGB2_RED_PORT, RGB2_RED_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(RGB2_GREEN_PORT, RGB2_GREEN_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(RGB2_BLUE_PORT, RGB2_BLUE_PIN, gpioModeDisabled, 0);
  }
}

void rgb__set_rgb_out(uint8_t RGB_number, color_t color, bool clear_other_colors, uint8_t intensity)
{
  (void)clear_other_colors;

  TIMER_TypeDef *TIMER_peripheral;
  if (RGB_number == 1)
  {
    TIMER_peripheral = RGB1_TIMER;
  }
  else if (RGB_number == 2)
  {
    TIMER_peripheral = RGB2_TIMER;
  }
  else
  {
    return;
  }

  uint32_t red_duty = (uint32_t)((color.red * intensity) >> 4);
  uint32_t green_duty = (uint32_t)((color.green * intensity) >> 4);
  uint32_t blue_duty = (uint32_t)((color.blue * intensity) >> 4);

  if ((color.red == 0) && (color.green == 0) && (color.blue == 0))
  {
    TIMER_CompareSet(TIMER_peripheral, 0, 0);
    TIMER_CompareSet(TIMER_peripheral, 1, 0);
    TIMER_CompareSet(TIMER_peripheral, 2, 0);
    return;
  }

  TIMER_CompareSet(TIMER_peripheral, 0, red_duty);
  TIMER_CompareSet(TIMER_peripheral, 1, green_duty);
  TIMER_CompareSet(TIMER_peripheral, 2, blue_duty);

}