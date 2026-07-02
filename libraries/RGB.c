#include "RGB.h"
#include "app_process.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "print.h"
#include "scheduler.h"
#include "state_machine.h"
#include "app_init.h"
#include "timer_helper.h"



//#define BIT_DEPTH 24
//#define VOLTAGE_RANGE_mV 30000 //3V p-p
//#define LOW_LEVEL_THRESHOLD_mV 500 //500mv
//#define MEDIUM_LEVEL_THRESHOLD_mV 2000 //2V
//#define HIGH_LEVEL_THRESHOLD_mV 29000 //2.9V
//
//#define NUMBER_OF_STEPS (0x00000001UL << (BIT_DEPTH - 1))
//#define NUMBER_OF_STEPS_PER_mV (NUMBER_OF_STEPS / VOLTAGE_RANGE_mV)
//
//#define LOW_LEVEL_THRESHOLD_BINARY (LOW_LEVEL_THRESHOLD_mV * NUMBER_OF_STEPS_PER_mV)
//#define MEDIUM_LEVEL_THRESHOLD_BINARY (MEDIUM_LEVEL_THRESHOLD_mV * NUMBER_OF_STEPS_PER_mV)
//#define HIGH_LEVEL_THRESHOLD_BINARY (HIGH_LEVEL_THRESHOLD_mV * NUMBER_OF_STEPS_PER_mV)
//
//const uint32_t LOW_LEVEL_THRESHOLD_BINARY_UPPER =  (LOW_LEVEL_THRESHOLD_BINARY>>1);
//const uint32_t MEDIUM_LEVEL_THRESHOLD_BINARY_UPPER =  (MEDIUM_LEVEL_THRESHOLD_BINARY>>1);
//const uint32_t HIGH_LEVEL_THRESHOLD_BINARY_UPPER =  (HIGH_LEVEL_THRESHOLD_BINARY>>1);
//
//const uint32_t LOW_LEVEL_THRESHOLD_BINARY_LOWER = (0x7FFFFFUL + LOW_LEVEL_THRESHOLD_BINARY_UPPER);
//const uint32_t MEDIUM_LEVEL_THRESHOLD_BINARY_LOWER = (0x7FFFFFUL + MEDIUM_LEVEL_THRESHOLD_BINARY_UPPER);
//const uint32_t HIGH_LEVEL_THRESHOLD_BINARY_LOWER = (0x7FFFFFUL + HIGH_LEVEL_THRESHOLD_BINARY_UPPER);

//#define BIT_DEPTH 24
#define VOLTAGE_RANGE_mV 3000 //3V p-p
#define LOW_LEVEL_THRESHOLD_mV 100 //500mv
#define MEDIUM_LEVEL_THRESHOLD_mV 2500 //2V
#define HIGH_LEVEL_THRESHOLD_mV 2900 //2.9V

#define HIGH_LEVEL_THRESHOLD_UPPER_8BIT ((HIGH_LEVEL_THRESHOLD_mV * 0xFFFFUL / VOLTAGE_RANGE_mV) >> 1)
#define MEDIUM_LEVEL_THRESHOLD_UPPER_8BIT ((MEDIUM_LEVEL_THRESHOLD_mV * 0xFFFFUL / VOLTAGE_RANGE_mV) >> 1)
#define LOW_LEVEL_THRESHOLD_UPPER_8BIT ((LOW_LEVEL_THRESHOLD_mV * 0xFFFFUL / VOLTAGE_RANGE_mV) >> 1)

#define HIGH_LEVEL_THRESHOLD_LOW_8BIT (0xFFFFUL - HIGH_LEVEL_THRESHOLD_UPPER_8BIT)
#define MEDIUM_LEVEL_THRESHOLD_LOW_8BIT (0xFFFFUL - MEDIUM_LEVEL_THRESHOLD_UPPER_8BIT)
#define LOW_LEVEL_THRESHOLD_LOW_8BIT (0xFFFFUL - LOW_LEVEL_THRESHOLD_UPPER_8BIT)

volatile uint32_t current_threshold_reached = 0;
volatile uint32_t new_threshold_reached = 0;
volatile uint32_t current_threshold_reached_timestamp = 0;
static uint32_t latest_audio_level_min_mv = 0;
static uint32_t latest_audio_level_max_mv = 0;

#define OUT_FREQ 0x0FFF

typedef struct{
  uint32_t timestamp;
  uint32_t state;
} radio_blink_state_change_t;

bool radio_status_blink_running = false;
uint32_t timestamp_radio_status_blink = 0;
#define LONG_BLINK 400
#define SHORT_BLINK 200
radio_blink_state_change_t on_off_timestamps[20];
uint32_t next_timestamp_index = 0;

uint32_t rgb__init_done = false;

void rgb__start_radio_status_blink_force(void)
{
  rgb__start_radio_status_blink(scheduler__get_millisecond_ticks());
  scheduler__reset_5s_countdown();
}

void rgb__start_radio_status_blink(uint32_t current_tick)
{
if (rgb__init_done == true)
{
  radio_status_blink_running = true;

  uint32_t channel = radio__get_channel();
  uint32_t timestamps_logged = 0;
  uint32_t state_change_timestamp = current_tick;

  while(channel > 0)
    {
      if (channel > 4)
        {
          channel -= 5;

          // Next turn on Time
          on_off_timestamps[timestamps_logged].timestamp = state_change_timestamp;
          on_off_timestamps[timestamps_logged++].state = 1;
          state_change_timestamp += LONG_BLINK;

          // Next turn off Time
          on_off_timestamps[timestamps_logged].timestamp = state_change_timestamp;
          on_off_timestamps[timestamps_logged++].state = 0;
          state_change_timestamp += LONG_BLINK;
        }
      else
        {
          channel--;

          // Next turn on Time
          on_off_timestamps[timestamps_logged].timestamp = state_change_timestamp;
          on_off_timestamps[timestamps_logged++].state = 1;
          state_change_timestamp += SHORT_BLINK;

          // Next turn off Time
          on_off_timestamps[timestamps_logged].timestamp = state_change_timestamp;
          on_off_timestamps[timestamps_logged++].state = 0;
          state_change_timestamp += SHORT_BLINK;
        }
    }

  for (uint8_t i=timestamps_logged ; i<20 ; i++)
    {
      on_off_timestamps[i].timestamp = 0xFFFFFFFF;
      on_off_timestamps[i].state = 0;
    }

  rgb__radio_blink_turn_on();
  next_timestamp_index = 1;
}
}

void rgb__run_radio_status_blink(uint32_t current_tick)
{
  if (radio_status_blink_running)
    {
      uint32_t current_time = current_tick;
      if (current_time  > on_off_timestamps[next_timestamp_index].timestamp)
        {
          if (on_off_timestamps[next_timestamp_index].state == 0)
            {
              rgb__set_rgb_out(1, black, 1, 0);
            }
          else
            {
              rgb__set_rgb_out(1, green, 1, 0x7F);
            }
          next_timestamp_index++;

          if (on_off_timestamps[next_timestamp_index].timestamp == 0xFFFFFFFF)
            {
              radio_status_blink_running = false;
            }
        }

      //      uint32_t current_time = get_millisecond_ticks();
      //      int32_t action_to_do = -1;
      //      for (uint8_t i=0 ; i<10 ; i++)
      //        {
      //          if (on_off_timestamps[i] < last_timestamp_run)
      //            {
      //              break;
      //            }
      //          action_to_do = i;
      //        }

    }

}

void rgb__get_audio_level_stats(uint32_t *min_level_mv, uint32_t *max_level_mv)
{
  if (min_level_mv != NULL)
  {
    *min_level_mv = latest_audio_level_min_mv;
  }

  if (max_level_mv != NULL)
  {
    *max_level_mv = latest_audio_level_max_mv;
  }
}

void rgb__radio_blink_turn_on(void)
{
  if (state_machine__get_state() == RUNNING)
    {
      rgb__set_rgb_out(1, green, 1, 0x7F);
    }
  else
    {
      rgb__set_rgb_out(1, red, 1, 0x7F);
    }

}

void rgb__timer_init(void)
{
  if (RGB1_TIMER == RGB2_TIMER)
  {
    // TODO Handle Error
    debug__printf_to_buf_append_time(0,"Can't use the same timer for RGB1 and RGB2\n");
    assert(0);
  }

  if (timers__init_timer_cmu(RGB1_TIMER) == false)
  {
    debug__printf_to_buf_append_time(0,"Bad RGB1_TIMER choice\n");
    assert(0);
  }
  if (timers__init_timer_cmu(RGB2_TIMER) == false)
  {
    debug__printf_to_buf_append_time(0,"Bad RGB2_TIMER choice\n");
    assert(0);
  }

  uint32_t RGB1_timer_index = timers__get_timer_index(RGB1_TIMER);

  if (RGB1_timer_index == 0xFFFFFFFF)
  {
    // BAD RGB1_TIMER value
    debug__printf_to_buf_append_time(0,"Invalid RGB1 index\n");
    assert(0);
  }

  uint32_t RGB2_timer_index = timers__get_timer_index(RGB2_TIMER);

  if (RGB2_timer_index == 0xFFFFFFFF)
  {
    // BAD RGB2_TIMER value
    debug__printf_to_buf_append_time(0,"Invalid RGB2 index\n");
    assert(0);
  }

  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

  // Don't start counter on initialization
  timerInit.enable = false;

  // PWM mode sets/clears the output on compare/overflow events
  timerCCInit.mode = timerCCModePWM;

  TIMER_Init(RGB1_TIMER, &timerInit);
  TIMER_Init(RGB2_TIMER, &timerInit);

  // Route CC0 output to PA6
  GPIO->TIMERROUTE[RGB1_timer_index].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN | GPIO_TIMER_ROUTEEN_CC1PEN | GPIO_TIMER_ROUTEEN_CC2PEN;
  GPIO->TIMERROUTE[RGB1_timer_index].CC0ROUTE = (RGB1_RED_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) | (RGB1_RED_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB1_timer_index].CC1ROUTE = (RGB1_GREEN_PORT << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT) | (RGB1_GREEN_PIN << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB1_timer_index].CC2ROUTE = (RGB1_BLUE_PORT << _GPIO_TIMER_CC2ROUTE_PORT_SHIFT) | (RGB1_BLUE_PIN << _GPIO_TIMER_CC2ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB2_timer_index].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN | GPIO_TIMER_ROUTEEN_CC1PEN | GPIO_TIMER_ROUTEEN_CC2PEN;
  GPIO->TIMERROUTE[RGB2_timer_index].CC0ROUTE = (RGB2_RED_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) | (RGB2_RED_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB2_timer_index].CC1ROUTE = (RGB2_GREEN_PORT << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT) | (RGB2_GREEN_PIN << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[RGB2_timer_index].CC2ROUTE = (RGB2_BLUE_PORT << _GPIO_TIMER_CC2ROUTE_PORT_SHIFT) | (RGB2_BLUE_PIN << _GPIO_TIMER_CC2ROUTE_PIN_SHIFT);

  TIMER_InitCC(RGB1_TIMER, 0, &timerCCInit);
  TIMER_InitCC(RGB1_TIMER, 1, &timerCCInit);
  TIMER_InitCC(RGB1_TIMER, 2, &timerCCInit);
  TIMER_InitCC(RGB2_TIMER, 0, &timerCCInit);
  TIMER_InitCC(RGB2_TIMER, 1, &timerCCInit);
  TIMER_InitCC(RGB2_TIMER, 2, &timerCCInit);

  //  // Set top value to overflow at the desired PWM_FREQ frequency
  //  timerFreq = CMU_ClockFreqGet(cmuClock_RGB1_TIMER) / (timerInit.prescale + 1);
  //  topValue = (timerFreq / OUT_FREQ);
  TIMER_TopSet(RGB1_TIMER, 0xFFF);
  TIMER_TopSet(RGB2_TIMER, 0xFFF);

  // Now start the TIMER
  TIMER_Enable(RGB1_TIMER, true);
  TIMER_Enable(RGB2_TIMER, true);

  // Trigger DMA on compare event to set CCVB to update duty cycle on next period
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

  rgb__init_done = true;
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


  if(CMU->CLKEN0 & CMU_CLKEN0_GPIO)
  {
  GPIO_PinModeSet(RGB1_RED_PORT, RGB1_RED_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(RGB1_GREEN_PORT, RGB1_GREEN_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(RGB1_BLUE_PORT, RGB1_BLUE_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(RGB2_RED_PORT, RGB2_RED_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(RGB2_GREEN_PORT, RGB2_GREEN_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(RGB2_BLUE_PORT, RGB2_BLUE_PIN, gpioModeDisabled, 0);
  }

  // RGB1_TIMER->CC[0].OCB = 0;
  // RGB1_TIMER->CC[1].OCB = 0;
  // RGB1_TIMER->CC[2].OCB = 0;
  // RGB2_TIMER->CC[0].OCB = 0;
  // RGB2_TIMER->CC[1].OCB = 0;
  // RGB2_TIMER->CC[2].OCB = 0;
}

void rgb__run_signal_intensity_state_machine(uint32_t current_tick)
{
  if (new_threshold_reached > current_threshold_reached)
    {
      current_threshold_reached = new_threshold_reached;
      new_threshold_reached = 0;
      current_threshold_reached_timestamp = current_tick;
      switch (current_threshold_reached)
      {
        case LOW_LEVEL_THRESHOLD_UPPER_8BIT:
          rgb__set_rgb_out(2, green, 1, 0x7F);
          break;
        case MEDIUM_LEVEL_THRESHOLD_UPPER_8BIT:
          rgb__set_rgb_out(2, yellow, 1, 0x7F);
          break;
        case HIGH_LEVEL_THRESHOLD_UPPER_8BIT:
          rgb__set_rgb_out(2, red, 1, 0x7F);
          break;
        default:
          //this shouldn't happen
          break;
      }
    }
  else if (new_threshold_reached == current_threshold_reached)
    {
      new_threshold_reached = 0;
      current_threshold_reached_timestamp = current_tick;
    }
  if (current_tick - current_threshold_reached_timestamp > 300)
    {
      current_threshold_reached = 0;
      rgb__set_rgb_out(2, black, 1, 0x7F);
    }

}

void rgb__check_level(uint8_t* new_data_pointer)
{
  if (new_data_pointer == 0)
    {
      debug__printf_to_buf_append_time(0,"NULL POINTER \n");
      assert(0);
    }
  uint16_t* two_byte_pointer = (uint16_t*)new_data_pointer;

  uint16_t highest_signal_level = 0;
  uint16_t lowest_signal_level = 0xFFFF;

  for (uint32_t i=0 ; i< 127; i++)
    {
      uint16_t* two_byte_pointer_temp = (two_byte_pointer + i);
      if (*two_byte_pointer_temp & 0x8000)
        {
          // sample is negative
          if (*two_byte_pointer_temp < lowest_signal_level)
            {
              lowest_signal_level = *two_byte_pointer_temp;
            }
        }
      else
        {
          // sample is positive
          if (*two_byte_pointer_temp > highest_signal_level)
            {
              highest_signal_level = *two_byte_pointer_temp;
            }
        }

      //
      //          uint32_t threshold_low = (LOW_LEVEL_THRESHOLD_LOW_8BIT); //1.25V for a 500mV p-p
      //          uint32_t threshold_mid = (MEDIUM_LEVEL_THRESHOLD_LOW_8BIT);
      //          uint32_t threshold_high = (HIGH_LEVEL_THRESHOLD_LOW_8BIT);
      //          // sample is negative
      //          if (*sample_pointer < threshold_high)
      //            {
      //              if (*sample_pointer < threshold_mid)
      //                {
      //                  if (*sample_pointer < threshold_low)
      //                    {
      //
      //                    }
      //                  else
      //                    {
      //                      // signal is very low
      //                    }
      //                }
      //              else
      //                {
      //                  //signal is low
      //                }
      //            }
      //          else
      //            {
      //                //signal is medium
      //            }
      //        }
      //      else
      //        {
      //          // sample is positive
      //          uint32_t threshold_low = (LOW_LEVEL_THRESHOLD_UPPER_8BIT);
      //          uint32_t threshold_mid = (MEDIUM_LEVEL_THRESHOLD_UPPER_8BIT);
      //          uint32_t threshold_high = (HIGH_LEVEL_THRESHOLD_UPPER_8BIT);
      //          if (*sample_pointer & threshold_high >> 24)
      //            {
      //              set_rgb(0, red, 1, 1);
      //            }
      //        }
    }
  uint32_t lowest_signal_level_abs =  0xFFFF - lowest_signal_level;
  uint32_t max_amplitude = (lowest_signal_level_abs > highest_signal_level) ? lowest_signal_level_abs : highest_signal_level;
  uint32_t threshold_low = (LOW_LEVEL_THRESHOLD_UPPER_8BIT);
  uint32_t threshold_mid = (MEDIUM_LEVEL_THRESHOLD_UPPER_8BIT);
  uint32_t threshold_high = (HIGH_LEVEL_THRESHOLD_UPPER_8BIT);

  latest_audio_level_max_mv = (highest_signal_level * VOLTAGE_RANGE_mV) / 0xFFFFUL;
  latest_audio_level_min_mv = (lowest_signal_level_abs * VOLTAGE_RANGE_mV) / 0xFFFFUL;

//  printf_to_buf_append_time(0,"Max: %umv - ",(highest_signal_level)*3000/0xFFFF);
//  printf_to_buf_append_time(0,"Min: -%umv\n",(0xFFFF - lowest_signal_level)*3000/0xFFFF);

  uint32_t threshold_reached = 0;

  if (max_amplitude > threshold_high)
    {
      // signal over HIGH threshold
      threshold_reached = HIGH_LEVEL_THRESHOLD_UPPER_8BIT;
      //rgb__set_rgb_out(2, red, 1, 127);
    }
  else if (max_amplitude > threshold_mid)
    {
      // signal over MEDIUM threshold
      threshold_reached = MEDIUM_LEVEL_THRESHOLD_UPPER_8BIT;
      //rgb__set_rgb_out(2, yellow, 1, 127);
    }
  else if (max_amplitude > threshold_low)
    {
      // signal over LOW threshold
      threshold_reached = LOW_LEVEL_THRESHOLD_UPPER_8BIT;
      //rgb__set_rgb_out(2, green, 1, 127);
    }
  else
    {
      //signal barely present
      //rgb__set_rgb_out(2, black, 1, 1);
    }
  if (threshold_reached >= current_threshold_reached)
    {
      new_threshold_reached = threshold_reached;
      current_threshold_reached_timestamp = get_msTicks();
    }
}

//void set_rgb(uint8_t RGB_number, color_t color, bool clear_other_colors, uint8_t intensity)
//{
//  uint32_t RED_port;
//  uint32_t RED_pin;
//  uint32_t GREEN_port;
//  uint32_t GREEN_pin;
//  uint32_t BLUE_port;
//  uint32_t BLUE_pin;
//
//  if (RGB_number == 0)
//    {
//      RED_port = RGB1_RED_PORT;
//      RED_pin = RGB1_RED_PIN;
//      GREEN_port = RGB1_GREEN_PORT;
//      GREEN_pin = RGB1_GREEN_PIN;
//      BLUE_port = RGB1_BLUE_PORT;
//      BLUE_pin = RGB1_BLUE_PIN;
//    }
//  else if (RGB_number == 1)
//    {
//      RED_port = RGB2_RED_PORT;
//      RED_pin = RGB2_RED_PIN;
//      GREEN_port = RGB2_GREEN_PORT;
//      GREEN_pin = RGB2_GREEN_PIN;
//      BLUE_port = RGB2_BLUE_PORT;
//      BLUE_pin = RGB2_BLUE_PIN;
//    }
//  else
//    {
//      return;
//    }
//
////  uint32_t output_red = color.red *
//
//
//  if (color.red)
//    {
//      GPIO_PinOutSet(RED_port,RED_pin);
//    }
//  else
//    {
//      GPIO_PinOutClear(RED_port,RED_pin);
//    }
//
//  if (color.green)
//    {
//      GPIO_PinOutSet(GREEN_port,GREEN_pin);
//    }
//  else
//    {
//      GPIO_PinOutClear(GREEN_port,GREEN_pin);
//    }
//
//  if (color.blue)
//    {
//      GPIO_PinOutSet(BLUE_port,BLUE_pin);
//    }
//  else
//    {
//      GPIO_PinOutClear(BLUE_port,BLUE_pin);
//    }
//}

void rgb__set_rgb_out(uint8_t RGB_number, color_t color, bool clear_other_colors, uint8_t intensity)
{
  (void)clear_other_colors;
  // if ((TIMER_peripheral != TIMER0) && (TIMER_peripheral != TIMER1))
  // {
  //   //TODO NOTE ERROR
  //   return;
  // }
  TIMER_TypeDef * TIMER_peripheral;
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
      //TODO NOTE ERROR
      return;
    }

    if ((TIMER_peripheral->STATUS & TIMER_STATUS_RUNNING) == 0)
    {
//TODO NOTE ERROR
      return;
    }

  if ((color.red == 0) && (color.green == 0) && (color.blue == 0))
    {
      TIMER_peripheral->CC[0].OCB = 0;
      TIMER_peripheral->CC[1].OCB = 0;
      TIMER_peripheral->CC[2].OCB = 0;
      return;
    }

  TIMER_peripheral->CC[0].OCB = (color.red * intensity) >> 4;
  TIMER_peripheral->CC[1].OCB = (color.green * intensity) >> 4;
  TIMER_peripheral->CC[2].OCB = (color.blue * intensity) >> 4;
}

