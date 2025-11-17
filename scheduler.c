#include "scheduler.h"

#define DEBUG 3

volatile uint32_t millisecond_ticks = 0;
uint32_t non_volatile_millisecond_ticks = 0;

bool tick_flag = false;

static uint32_t countdown_5s = 5000;
static uint32_t countdown_1s = 1000;
static uint32_t countdown_100ms = 100;

static void run_scheduler_1_ms(void);
static void run_scheduler_10_ms(void);
static void run_scheduler_100_ms(void);
static void run_scheduler_1s(void);

//bool tick_flag_10ms = false;
//bool tick_flag_100ms = false;
//bool tick_flag_1s = false;

void scheduler__reset_5s_countdown(void)
{
  countdown_5s = 5000;
}

uint32_t scheduler__get_millisecond_ticks(void)
{
  return non_volatile_millisecond_ticks;
}

uint32_t scheduler__get_microsecond_ticks(void)
{
  return ((non_volatile_millisecond_ticks * 1000) + microseconds__get_micros_count());
}

void SysTick_Handler(void)
{
  millisecond_ticks++;
  //printf("tick : %u\n",microseconds__get_micros_count());
  microseconds__reset_micros_count();
  tick_flag = true;
}

void scheduler__deinit_SysTick(void)
{
  printf("Starting SysTick DeInit\n");
  SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk;

  printf("Finish SysTick DeInit\n");
}

void scheduler__init_SysTick(void)
{
  printf("Starting SysTick\n");
  uint32_t core_speed = CMU_ClockFreqGet(cmuClock_CORE);
  printf("Core Speed : %u\n", (unsigned int)core_speed);
  if (SysTick_Config(core_speed/1000) != 0)
    {
      printf("Systick Failed to start\n");
      assert(0);
    }
  microseconds__reset_micros_count();
  printf("Systick Started\n");
}

static void run_scheduler_1_ms(void)
{
  rgb__run_signal_intensity_state_machine(non_volatile_millisecond_ticks);
  rgb__run_radio_status_blink(non_volatile_millisecond_ticks);
}

static void run_scheduler_10_ms(void)
{

}

static void run_scheduler_100_ms(void)
{
   WDOGn_Feed(WDOG0);
   //printf("Feed WDOG\n");
}


static void run_scheduler_1s(void)
{
  //radio__validate_radio_statistics();
  debug__one_second_print();
}

static void run_scheduler_5s(void)
{
  rgb__start_radio_status_blink(non_volatile_millisecond_ticks);
}


void scheduler__run_scheduler(void)
{
  //printf("tick");
  if (tick_flag == true)
    {
      //printf("**tock**");
      tick_flag = false;
      non_volatile_millisecond_ticks = millisecond_ticks;
      run_scheduler_1_ms();

      static uint32_t countdown_10ms = 10;
      countdown_10ms--;
      if (countdown_10ms == 0)
        {
          countdown_10ms = 10;
          run_scheduler_10_ms();

          //          static uint32_t countdown_100ms = 10;
          //          countdown_100ms--;
          //          if (countdown_100ms == 0)
          //            {
          //              countdown_100ms = 10;
          //              run_scheduler_100_ms();
          //
          //              static uint32_t countdown_1s = 10;
          //              countdown_1s--;
          //              if (countdown_1s == 0)
          //                {
          //                  countdown_1s = 10;
          //                  run_scheduler_1s();
          //                }
          //            }
        }

      static uint32_t countdown_100ms = 100;
      countdown_100ms--;
      if (countdown_100ms == 0)
        {
          countdown_100ms = 100;
          run_scheduler_100_ms();
        }

      static uint32_t countdown_1s = 1000;
      countdown_1s--;
      if (countdown_1s == 0)
        {
          countdown_1s = 1000;
          run_scheduler_1s();
        }

      static uint32_t countdown_5s = 5000;
      countdown_5s--;
      if (countdown_5s == 0)
        {
          countdown_5s = 5000;
          run_scheduler_5s();
        }
    }
}
//
//  /* Increment counter necessary in Delay()*/
//  millisecond_ticks++;
//
//  if (!(msTicks%1000))
//    {
//
//    }
//
//  if (!(msTicks%1))
//    {
//      static uint32_t counter = 0;
//      counter++;
//
//#if (DEBUG == 1)
//      printf("\n\n");
//#endif
//
//      RAIL_Handle_t* temp = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST0);
//      //      uint32_t return_status = RAIL_WriteTxFifo(temp, &(buffer[0]), PACKET_SIZE, false);
//      //      RAIL_Status_t status = RAIL_StartTx(temp, 0, RAIL_TX_OPTIONS_DEFAULT, NULL);
//
//#if (DEBUG == 1)
//      if (status != SL_STATUS_OK)
//        {
//          printf("%i : %X\n", counter, status);
//        }
//      else
//        {
//          printf("%i : Success \n", counter);
//        }
//      if (status != SL_STATUS_OK)
//        {
//          buffer[0] = 0;
//        }
//#endif
//    }
//
//#if (DEBUG == 2)
//  if (!(msTicks%100))
//    {
//      uint8_t random = (uint8_t)((DWT->CYCCNT) % 18);
//      switch(random)
//      {
//        case 0:
//          set_rgb(0, red, 1, 1);
//          break;
//        case 1:
//          set_rgb(0, green, 1, 1);
//          break;
//        case 2:
//          set_rgb(0, blue, 1, 1);
//          break;
//        case 3:
//          set_rgb(0, pink, 1, 1);
//          break;
//        case 4:
//          set_rgb(0, yellow, 1, 1);
//          break;
//        case 5:
//          set_rgb(0, teal, 1, 1);
//          break;
//        case 6:
//          set_rgb(0, white, 1, 1);
//          break;
//        case 7:
//          set_rgb(0, purple, 1, 1);
//          break;
//        case 8:
//          set_rgb(0, orange, 1, 1);
//          break;
//
//        case 9:
//          set_rgb(1, red, 1, 1);
//          break;
//        case 10:
//          set_rgb(1, green, 1, 1);
//          break;
//        case 11:
//          set_rgb(1, blue, 1, 1);
//          break;
//        case 12:
//          set_rgb(1, pink, 1, 1);
//          break;
//        case 13:
//          set_rgb(1, yellow, 1, 1);
//          break;
//        case 14:
//          set_rgb(1, teal, 1, 1);
//          break;
//        case 15:
//          set_rgb(1, white, 1, 1);
//          break;
//        case 16:
//          set_rgb(1, purple, 1, 1);
//          break;
//        case 17:
//          set_rgb(1, orange, 1, 1);
//          break;
//
//
//        default:
//          printf("!");
//          break;
//      }
//      //      static uint8_t state = 0;
//      //
//      //
//      //      GPIO_PinOutClear(gpioPortA, 4);
//      //      GPIO_PinOutClear(gpioPortA, 5);
//      //      GPIO_PinOutClear(gpioPortA, 6);
//      //      GPIO_PinOutClear(gpioPortA, 0);
//      //      GPIO_PinOutClear(gpioPortB, 0);
//      //      GPIO_PinOutClear(gpioPortB, 1);
//      //
//      //      switch(state)
//      //      {
//      //        case 0:
//      //          GPIO_PinOutSet(gpioPortA, 4);
//      //          break;
//      //        case 1:
//      //          GPIO_PinOutSet(gpioPortA, 5);
//      //          break;
//      //        case 2:
//      //          GPIO_PinOutSet(gpioPortA, 6);
//      //          break;
//      //        case 3:
//      //          GPIO_PinOutSet(gpioPortA, 0);
//      //          break;
//      //        case 4:
//      //          GPIO_PinOutSet(gpioPortB, 0);
//      //          break;
//      //        case 5:
//      //          GPIO_PinOutSet(gpioPortB, 1);
//      //          break;
//      //        default:
//      //          printf("!");
//      //          break;
//      //      }
//      //      state++;
//      //      if (state > 5)
//      //        {
//      //          state = 0;
//      //        }
//    }
//#endif
//
//#if (DEBUG == 3)
//  if (!(msTicks%100))
//    {
//      //      GPIO_PinOutClear(gpioPortA, 4);
//      //      GPIO_PinOutClear(gpioPortA, 5);
//      //      GPIO_PinOutClear(gpioPortA, 6);
//      //      GPIO_PinOutClear(gpioPortA, 0);
//      //      //GPIO_PinOutClear(gpioPortB, 0);
//      //      GPIO_PinOutClear(gpioPortB, 1);
//    }
//
//#endif
//
//  if (!(msTicks%100))
//    {
//      one_second_tick = 1;
//    }
//}
