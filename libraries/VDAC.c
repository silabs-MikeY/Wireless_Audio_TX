#include <stdio.h>
#include <assert.h>

#include "VDAC.h"
#include "ADC.h"
#include "print.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_vdac.h"
#include "sl_gpio.h"
#include "sl_status.h"

uint32_t start = false;

static uint32_t index = 0;
uint32_t* array_pointer;
extern volatile uint8_t leftBuffer_1[BUFFER_SIZE];
extern volatile uint8_t leftBuffer_2[BUFFER_SIZE];
extern volatile uint32_t leftBuffer_converted_word[BUFFER_SIZE/4];
extern volatile uint32_t debug_signals[10];

bool running = false;

uint32_t get_next_value(void)
{
  uint32_t return_value = array_pointer[index];

  index++;
  if (index >= (BUFFER_SIZE/4))
    {
      index = 0;
    }
  return return_value;
}

void LRCLK_callback(uint8_t int_no, void *context)
{
  (void) int_no;
  (void) context;

  //GPIO_PinOutToggle(VDAC_PORT, VDAC_PIN);
  uint32_t output;
  if (running == true)
    {
      output = get_next_value();
      // static uint8_t temp = 0;
      //VDAC_ChannelOutputSet(VDAC0, 0, output>>20);
//      if (!(VDAC0->STATUS & VDAC_STATUS_CH0ENS))
//        {
//          debug_signals[1] = 1;
//        }
//      if (temp == 0)
//        {
//          VDAC_ChannelOutputSet(VDAC0, 0, 0xFFF);
//          temp = 1;
//        }
//      else
//        {
//          VDAC_ChannelOutputSet(VDAC0, 0, 0x0);
//          temp = 0;
//        }
    }
  return;
}

void vdac__start_output(void)
{
  array_pointer = leftBuffer_converted_word;
  running = true;
}

//void GPIO_EVEN_IRQHandler(void)
//{
//  GPIO_IRQ();
//}
//
//void GPIO_ODD_IRQHandler(void)
//{
//  GPIO_IRQ();
//}

void init_vdac_interrupt(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  //  // Configure Button PB0 as input and enable interrupt
  //  //GPIO_PinModeSet(I2S_LRCLK_PORT, I2S_LRCLK_PIN, gpioModeInputPull, 1);
  //  GPIO_ExtIntConfig(I2S_LRCLK_PORT,
  //                    I2S_LRCLK_PIN,
  //                    I2S_LRCLK_PIN,
  //                    false,
  //                    true,
  //                    true);
  //
  //  // Enable EVEN interrupt to catch button press that changes slew rate
  //  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  //  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
  //
  //  // Enable ODD interrupt to catch button press that changes slew rate
  //  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  //  NVIC_EnableIRQ(GPIO_ODD_IRQn);

  sl_gpio_t pin;
  pin.pin = I2S_LRCLK_PIN;
  pin.port = I2S_LRCLK_PORT;
  int32_t int_no = SL_GPIO_INTERRUPT_UNAVAILABLE;
  sl_status_t return_status = sl_gpio_configure_external_interrupt(&pin, &int_no, SL_GPIO_INTERRUPT_RISING_EDGE, LRCLK_callback, NULL);
  if (return_status != SL_STATUS_OK)
    {
      debug__printf_to_buf_append_time(0,"GPIO Configure Error: %X\n",(unsigned int)return_status);
      assert(0);
    }
  debug__printf_to_buf_append_time(0,"Enabled GPIO \n");
}

void init_vdac_output(void)
{
  GPIO_PinModeSet(VDAC_PORT, VDAC_PIN, gpioModeDisabled, 0);
  GPIO->BBUSALLOC = GPIO_BBUSALLOC_BEVEN0_VDAC0CH0;

  // Use default settings
  VDAC_Init_TypeDef        init        = VDAC_INIT_DEFAULT;
  VDAC_InitChannel_TypeDef initChannel = VDAC_INITCHANNEL_DEFAULT;

  // Use the HFRCOEM23 to clock the VDAC in order to operate in EM3 mode
  CMU_ClockSelectSet(cmuClock_VDAC0, cmuSelect_HFRCOEM23);

  // Enable the HFRCOEM23 and VDAC clocks
  CMU_ClockEnable(cmuClock_HFRCOEM23, true);
  CMU_ClockEnable(cmuClock_VDAC0, true);

#define CLK_VDAC_FREQ              1000000
#define CHANNEL_NUM 0

  //VDAC0->OUTCTRL = (VDAC_OUTCTRL_ABUSPORTSELCH0_PORTB | VDAC_OUTCTRL_AUXOUTENCH0);
  //initChannel.enable = false;
//  initChannel.auxOutEnable = true;
  initChannel.mainOutEnable = true;
//  initChannel.port = vdacChPortB;
//  initChannel.pin = VDAC_PIN;

  // Set the VDAC to max frequency of 1 MHz
#define CLK_VDAC_FREQ              1000000

  // Calculate the VDAC clock prescaler value resulting in a 1 MHz VDAC clock
  init.prescaler = VDAC_PrescaleCalc(VDAC0, CLK_VDAC_FREQ);

  // Clocking is requested on demand
  init.onDemandClk = false;

  init.reference = vdacRefAvdd;

  // Disable High Capacitance Load mode
  initChannel.highCapLoadEnable = false;

  // Use Low Power mode
  initChannel.powerMode = vdacPowerModeHighPower;

  // Initialize the VDAC and VDAC channel
  VDAC_Init(VDAC0, &init);
  VDAC_InitChannel(VDAC0, &initChannel, CHANNEL_NUM);

  // Enable the VDAC
  VDAC_Enable(VDAC0, CHANNEL_NUM, true);
}

void vdac__init_vdac(void)
{
  //init_vdac_output();
  //init_vdac_interrupt();
}
