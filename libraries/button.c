#include "button.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "hardware_config.h"
#include "sl_gpio.h"

#include <stdbool.h>

// -----------------------------------------------------------------------------
//                     Weak function implementations, do not rename.
// -----------------------------------------------------------------------------

__attribute__((weak)) void button__printf(bool add_timestamp, const char *format, ...)
{
  (void)add_timestamp;
  (void)format;
}

__attribute__((weak)) void button__rising_edge(void)
{
}

__attribute__((weak)) void button__falling_edge(void)
{
}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Button General
// -----------------------------------------------------------------------------

void button_callback(uint8_t int_no, void *context);

static bool button_waiting_for_falling_edge = false;
static int32_t button_interrupt_number = SL_GPIO_INTERRUPT_UNAVAILABLE;

static sl_gpio_t button_pin = {
  .pin = BUTTON_PIN,
  .port = BUTTON_PORT,
};

static bool button__configure_interrupt(int32_t edge)
{
  sl_status_t return_status = sl_gpio_configure_external_interrupt(&button_pin, &button_interrupt_number, edge, button_callback, 0);
  if (return_status != SL_STATUS_OK)
  {
    button__printf(true, "GPIO Configure Error: %X\n", (unsigned int)return_status);
    return false;
  }
  return true;
}

void button_callback(uint8_t int_no, void *context)
{
  (void)int_no;
  (void)context;

  if (button_waiting_for_falling_edge == false)
  {
    button_waiting_for_falling_edge = true;
    button__rising_edge();
    (void)sl_gpio_deconfigure_external_interrupt(button_interrupt_number);
    (void)button__configure_interrupt(SL_GPIO_INTERRUPT_FALLING_EDGE);
  }
  else
  {
    button_waiting_for_falling_edge = false;
    button__falling_edge();
    (void)sl_gpio_deconfigure_external_interrupt(button_interrupt_number);
    (void)button__configure_interrupt(SL_GPIO_INTERRUPT_RISING_EDGE);
  }
}

bool button__init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(BUTTON_PORT, BUTTON_PIN, gpioModeInputPullFilter, 0);

  button_waiting_for_falling_edge = false;
  button_interrupt_number = SL_GPIO_INTERRUPT_UNAVAILABLE;
  if (!button__configure_interrupt(SL_GPIO_INTERRUPT_RISING_EDGE)) {
    return false;
  }
  button__printf(true, "Enabled Button \n");
  return true;
}

void button__deinit(void)
{
  if (CMU->CLKEN0 & CMU_CLKEN0_GPIO)
  {
    GPIO_PinModeSet(BUTTON_PORT, BUTTON_PIN, gpioModeDisabled, 0);
  }

  (void)sl_gpio_deconfigure_external_interrupt(button_interrupt_number);
}

// -----------------------------------------------------------------------------
//                     Button General End
// -----------------------------------------------------------------------------
