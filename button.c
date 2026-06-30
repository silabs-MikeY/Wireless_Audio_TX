#include "button.h"
#include "RGB.h"
#include "print.h"

// uint32_t channel_changed_flag = false;

uint32_t last_button_press_timestamp = 0;

void button_callback(uint8_t int_no, void *context)
{
  (void)int_no;
  (void)context;

  if ((scheduler__get_millisecond_ticks() - last_button_press_timestamp) > 100)
    {
      last_button_press_timestamp = scheduler__get_millisecond_ticks();
      radio__request_increment_channel();
      rgb__start_radio_status_blink_force();
      // uint32_t current_channel = radio__get_channel();
      // current_channel++;
      // if (current_channel == 20)
      //   {
      //     current_channel = 0;
      //   }
      // set_channel(current_channel);
      // channel_changed_flag = true;
    }
}

void button__init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(BUTTON_PORT, BUTTON_PIN, gpioModeInputPullFilter, 0);

  sl_gpio_t pin;
  pin.pin = BUTTON_PIN;
  pin.port = BUTTON_PORT;
  int32_t int_no = SL_GPIO_INTERRUPT_UNAVAILABLE;
  sl_status_t return_status = sl_gpio_configure_external_interrupt(&pin, &int_no, SL_GPIO_INTERRUPT_RISING_EDGE, button_callback, NULL);
  if (return_status != SL_STATUS_OK)
    {
      debug__printf_to_buf_append_time(0,"GPIO Configure Error: %X\n", (unsigned int)return_status);
      while(1);
    }
  debug__printf_to_buf_append_time(0,"Enabled Button \n");
}

void button__deinit(void)
{
  if (CMU->CLKEN0 & CMU_CLKEN0_GPIO)
  {
    GPIO_PinModeSet(BUTTON_PORT, BUTTON_PIN, gpioModeDisabled, 0);
  }

  sl_status_t return_status = sl_gpio_deconfigure_external_interrupt(SL_GPIO_INTERRUPT_UNAVAILABLE);
  if (return_status != SL_STATUS_OK)
    {
      debug__printf_to_buf_append_time(0,"GPIO Deconfigure Error: %X\n", (unsigned int)return_status);
      //while(1);
    }
}
