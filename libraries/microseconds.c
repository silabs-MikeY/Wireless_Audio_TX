#include "microseconds.h"

// -----------------------------------------------------------------------------
//                     Weak function implementations, do not rename.
// -----------------------------------------------------------------------------

__attribute__((weak)) void microseconds__printf(bool add_timestamp, const char *format, ...)
{
    (void)add_timestamp;
    (void)format;
}

// -----------------------------------------------------------------------------
//                     Weak function implementations End
// -----------------------------------------------------------------------------

typedef enum microseconds_counter_index_s {
  MICROSECONDS_SINCE_LAST_RESET = 0,
  MICROSECONDS_NUMBER_OF_COUNTERS
} microseconds_counter_index_t;

static volatile uint32_t microseconds_counter_values[MICROSECONDS_NUMBER_OF_COUNTERS] = {0};
static const char *microseconds_counter_names[MICROSECONDS_NUMBER_OF_COUNTERS] = {
    "micro_ticks_since_last_reset",
};

// -----------------------------------------------------------------------------
//                     Microsecond Counter Access
// -----------------------------------------------------------------------------

/**
 * @brief Gets the current microsecond counter value.
 * Returns elapsed microseconds from the hardware timer.
 * Timer must be enabled; returns 0 if not initialized.
 * 
 * @param None
 * @return Current microsecond count
 */
uint32_t microseconds__get_micros_count(void)
{
    if (timers__is_timer_enabled(MICROS_COUNT_TIMER))
    {
        // if (MICROS_COUNT_TIMER->STATUS == TIMER_STATUS_RUNNING)
        // {
            return (MICROS_COUNT_TIMER->CNT);
        // }
        // else
        // {
            assert(0);
        // }
    }
    return 0;

    // else
    // {
    //     assert(0);
    // }
}

// /**
//  * @brief Resets the microsecond counter to zero.
//  * Clears the timer counter register if timer is enabled.
//  * Called to establish new time reference points.
//  * 
//  * @param None
//  * @return void
//  */
// void microseconds__reset_micros_count(void)
// {
//     if (timers__is_timer_enabled(MICROS_COUNT_TIMER))
//     {
//             microseconds_counter_values[MICROSECONDS_SINCE_LAST_RESET] += MICROS_COUNT_TIMER->CNT;
//             TIMER_CounterSet(MICROS_COUNT_TIMER, 0);
//     }
// }

void microseconds__trigger_counter_update(void)
{
    static uint32_t last_micros_count = 0;
    uint32_t current_micros_count = microseconds__get_micros_count() - last_micros_count;
    last_micros_count = microseconds__get_micros_count();
    microseconds_counter_values[MICROSECONDS_SINCE_LAST_RESET] = current_micros_count;
}

uint32_t microseconds__get_number_of_counters(void)
{
    return MICROSECONDS_NUMBER_OF_COUNTERS;
}

const char *microseconds__get_counter_name(uint32_t counter_index)
{
    if (counter_index >= microseconds__get_number_of_counters())
    {
        return NULL;
    }

    return microseconds_counter_names[counter_index];
}

volatile uint32_t *microseconds__get_counter_address(uint32_t counter_index)
{
    if (counter_index >= microseconds__get_number_of_counters())
    {
        return NULL;
    }

    return &microseconds_counter_values[counter_index];
}

void microseconds__reset_counters(void)
{
    microseconds_counter_values[MICROSECONDS_SINCE_LAST_RESET] = 0;
}

// -----------------------------------------------------------------------------
//                     Microsecond Counter Access End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Microsecond Timer General
// -----------------------------------------------------------------------------

/**
 * @brief De-initializes the microsecond timer system.
 * Disables both the microsecond counter and tick timer.
 * Validates timer before attempting to disable.
 * Called during shutdown or state transitions.
 * 
 * @param None
 * @return void
 */
void microseconds__deinit_microsecond(void)
{
    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"-- Stopping Microsecond Tick Timer\n"));
    if (TIMER_Valid(MICROS_TICK_TIMER) == false)
    {
        DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Bad MICROS_TICK_TIMER choice\n"));
        assert(0);
    }
    if (TIMER_Valid(MICROS_COUNT_TIMER) == false)
    {
        DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Bad MICROS_COUNT_TIMER choice\n"));
        assert(0);
    }

    if (timers__is_timer_enabled(MICROS_COUNT_TIMER))
    {
        TIMER_Enable(MICROS_COUNT_TIMER, false);
        TIMER_Reset(MICROS_COUNT_TIMER);
        timers__deinit_timer_cmu(MICROS_COUNT_TIMER);
    }

    if (timers__is_timer_enabled(MICROS_TICK_TIMER))
    {
        TIMER_Enable(MICROS_TICK_TIMER, false);
        TIMER_Reset(MICROS_TICK_TIMER);
        timers__deinit_timer_cmu(MICROS_TICK_TIMER);
    }
    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"-- Finished Stopping Microsecond Tick Timer\n"));
}

static bool microseconds__validate_microsecond_count_timer(void)
{
    uint32_t start_count = MICROS_COUNT_TIMER->CNT;
    DEBUG_PERIPHERALS_LOG(microseconds__printf(DWT->CYCCNT,"Testing Microsecond Timer with 50uS Wait : Current %u\n", (unsigned int)start_count));

    uint32_t tick_count = DWT->CYCCNT;

    while ((uint32_t)(MICROS_COUNT_TIMER->CNT - start_count) < 500u)
    {
        if ((DWT->CYCCNT - tick_count) > 1000000u)
        {
            DEBUG_PERIPHERALS_LOG(microseconds__printf(DWT->CYCCNT,"Microsecond Timer Test Failed : Current %u\n", (unsigned int)MICROS_COUNT_TIMER->CNT));
            return false;
        }
    }

    DEBUG_PERIPHERALS_LOG(microseconds__printf(DWT->CYCCNT,"Microsecond Timer Finished Testing\n"));
    return true;
}

/**
 * @brief Initializes the microsecond tick timer.
 * Sets up timer peripheral to generate microsecond-precision interrupts.
 * Called once at startup to establish microsecond timing.
 * 
 * @param None
 * @return void
 */
bool microseconds__init_microsecond(void)
{
    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"-- Starting Microsecond Tick Timer\n"));
    if (TIMER_Valid(MICROS_TICK_TIMER) == false)
    {
        DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Bad MICROS_TIMER choice\n"));
        return false;
    }

    // CMU_ClockEnable(cmuClock_PRS, true);

    // uint32_t core_speed = CMU_ClockFreqGet(cmuClock_CORE);
    // printf_to_buf_append_time(0,"Core Speed : %u\n",(unsigned int)core_speed);
    // if (SysTick_Config(core_speed/1000) != 0)
    //   {
    //     printf_to_buf_append_time(0,"Microcecond Tick Failed to start\n");
    //     assert(0);
    //   }

    // #if (RGB1_TIMER == RGB2_TIMER)
    // #error "Can't use the same timer"
    // #endif

    //   CMU_Clock_TypeDef clock_to_enable;
    //   PRS_Signal_t PRS_signal;
    //   uint32_t PRS_source;

    if (timers__init_timer_cmu(MICROS_TICK_TIMER) == false)
    {
        return false;
    }

    //   if (MICROS_TICK_TIMER == TIMER0)
    //   {
    //     clock_to_enable = cmuClock_TIMER0;
    //     // PRS_signal = PRS_ASYNC_CH_CTRL_SIGSEL_TIMER0OF;
    //     // PRS_source = PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER0;
    //   }
    //   else if (MICROS_TICK_TIMER == TIMER1)
    //   {
    //     clock_to_enable = cmuClock_TIMER1;
    //     // PRS_signal = PRS_ASYNC_CH_CTRL_SIGSEL_TIMER1OF;
    //     // PRS_source = PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER1;
    //   }
    //   #if defined(TIMER3)
    //   else if (MICROS_TICK_TIMER == TIMER2)
    //   {
    //     clock_to_enable = cmuClock_TIMER2;
    //     // PRS_signal = PRS_ASYNC_CH_CTRL_SIGSEL_TIMER2OF;
    //     // //PRS_signal = prsSignalTIMER2_CC0;
    //     // PRS_source = PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER2;
    //   }
    //   #endif
    //   #if defined(TIMER3)
    //   else if (MICROS_TICK_TIMER == TIMER3)
    //   {
    //     clock_to_enable = cmuClock_TIMER3;
    //     // PRS_signal = PRS_ASYNC_CH_CTRL_SIGSEL_TIMER3OF;
    //     // PRS_source = PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER3;
    //   }
    //   #endif
    //   #if defined(TIMER4)
    //   else if (MICROS_TICK_TIMER == TIMER4)
    //   {
    //     clock_to_enable = cmuClock_TIMER4;
    //     // PRS_signal = PRS_ASYNC_CH_CTRL_SIGSEL_TIMER4OF;
    //     // PRS_source = PRS_ASYNC_CH_CTRL_SOURCESEL_TIMER4;
    //   }
    //   #endif
    //   else
    //   {
    //     //BAD RGB1_TIMER value
    //     //TODO Handle Error
    //     printf_to_buf_append_time(0,"Bad MICROS_TIMER choice\n");
    //     assert(0);
    //   }

    // CMU_ClockEnable(clock_to_enable, true);

    uint32_t timer_speed = CMU_ClockFreqGet(timers__get_cmu_type(MICROS_TICK_TIMER));
    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Micros timer base peripheral speed: %u\n", (unsigned int)timer_speed));

    if (timer_speed < 1000000)
    {
        // Under 1MHZ Too Low
        DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Micros timer base peripheral speed too Low\n"));
        return false;
    }

    if (timer_speed % 1000000 != 0)
    {
        // Clock Isn't a multipler of 1MHz
        // Shouldn't be possible AFAIK but still something that could be possible
        // Don't have a solution for this yet
        // TODO find a solution
        DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Micros timer base peripheral speed is not multiple of 1MHz\n"));
        return false;
    }

    // Find Highest Prescaler Possible to Create 1MHz Timer
    TIMER_Prescale_TypeDef maximum_prescale_possible = timerPrescale1;
    uint32_t prescaled_speed = timer_speed;

    // TODO probably not worth the flash space, just use the base timer speed
    if ((timer_speed % 64 == 0) && ((timer_speed / 64) % 1000000) == 0)
    {
        maximum_prescale_possible = timerPrescale64;
        prescaled_speed = timer_speed / 64;
    }
    else if ((timer_speed % 32 == 0) && ((timer_speed / 32) % 1000000) == 0)
    {
        maximum_prescale_possible = timerPrescale32;
        prescaled_speed = timer_speed / 32;
    }
    else if ((timer_speed % 16 == 0) && ((timer_speed / 16) % 1000000) == 0)
    {
        maximum_prescale_possible = timerPrescale16;
        prescaled_speed = timer_speed / 16;
    }
    else if ((timer_speed % 8 == 0) && ((timer_speed / 8) % 1000000) == 0)
    {
        maximum_prescale_possible = timerPrescale8;
        prescaled_speed = timer_speed / 8;
    }
    else if ((timer_speed % 4 == 0) && ((timer_speed / 4) % 1000000) == 0)
    {
        maximum_prescale_possible = timerPrescale4;
        prescaled_speed = timer_speed / 4;
    }
    else if ((timer_speed % 2 == 0) && ((timer_speed / 2) % 1000000) == 0)
    {
        maximum_prescale_possible = timerPrescale2;
        prescaled_speed = timer_speed / 2;
    }

    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Prescaler Chosen: %u\n", (unsigned int)(maximum_prescale_possible+1)));
    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"New Prescaled Speed: %u\n", (unsigned int)prescaled_speed));

    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
    TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

    timerInit.prescale = maximum_prescale_possible;
    timerInit.enable = false;

    // timerCCInit.mode = timerCCModeCompare;
    timerCCInit.mode = timerCCModePWM;

    // timerCCInit.prsOutput = timerPrsOutputLevel;
    //  timerCCInit.edge = timerEdgeRising;              // Input capture on rising edges

    TIMER_Init(MICROS_TICK_TIMER, &timerInit);
    TIMER_InitCC(MICROS_TICK_TIMER, 0, &timerCCInit);

    uint32_t TIMER_TOP = prescaled_speed / 1000000 - 1; // 1MHz Top Value
    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Top Value: %u\n", (unsigned int)TIMER_TOP));
    TIMER_TopSet(MICROS_TICK_TIMER, TIMER_TOP);

    uint32_t dutyCount = (TIMER_TOP >> 1); // half duty cycle
    TIMER_CompareSet(MICROS_TICK_TIMER, 0, dutyCount);

    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Set 50per duty cycle value: %u\n", (unsigned int)dutyCount));

    // GPIO_PinModeSet(RGB1_BLUE_PORT, RGB1_BLUE_PIN, gpioModePushPull, 0);

    // GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
    // GPIO->TIMERROUTE[0].CC0ROUTE = (RGB1_BLUE_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) | (RGB1_BLUE_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

    // CMU_ClockEnable(cmuClock_PRS, true);
    // PRS_SourceAsyncSignalSet(MICROS_COUNT_PRS_CH,
    //                            PRS_source,
    //                            PRS_signal);

    //            PRS_PinOutput(MICROS_COUNT_PRS_CH,
    // prsTypeAsync,
    // RGB1_BLUE_PORT,
    // RGB1_BLUE_PIN);

    //   printf_to_buf_append_time(0,"Set PRS output\n");

    TIMER_Enable(MICROS_TICK_TIMER, true);

    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"-- Started Microsecond Tick Timer\n"));

    if (!microseconds__init_microsecond_count()) {
        return false;
    }
    if (!microseconds__validate_microsecond_count_timer()) {
        return false;
    }
    return true;
}

/**
 * @brief Initializes the microsecond counter timer.
 * Sets up timer to count elapsed microseconds since initialization.
 * Called once at startup to enable microsecond counter functionality.
 * 
 * @param None
 * @return void
 */
bool microseconds__init_microsecond_count(void)
{
    // TODO CHECK FOR ALREADY INIT
    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"-- Starting Microsecond Count Timer\n"));

    if (TIMER_Valid(MICROS_COUNT_TIMER) == false)
    {
        DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Bad MICROS_COUNT_TIMER choice\n"));
        return false;
    }

    // if (((uint8_t*)MICROS_COUNT_TIMER) - (sizeof(TIMER_TypeDef)) != MICROS_TICK_TIMER)
    // {
    //     printf_to_buf_append_time(0,"MICROS_COUNT_TIMER is not sequential counter after MICROS_TICK_TIMER\n");
    //     printf_to_buf_append_time(0,"MICROS_COUNT_TIMER address: %u\n", MICROS_COUNT_TIMER);
    //     printf_to_buf_append_time(0,"MICROS_TICK_TIMER address: %u\n", MICROS_TICK_TIMER);
    //     printf_to_buf_append_time(0,"Calculated MICROS_TICK_TIMER address: %u\n", ((uint8_t*)MICROS_COUNT_TIMER) - (sizeof(TIMER_TypeDef)));
    //     printf_to_buf_append_time(0,"Calculated TIMER_TypeDef size: %u\n", sizeof(TIMER_TypeDef));
    //     printf_to_buf_append_time(0,"MICROS_COUNT_TIMER - MICROS_TICK_TIMER: %u\n", ((uint8_t*)MICROS_COUNT_TIMER) - ((uint8_t*)MICROS_TICK_TIMER) );
    //     printf_to_buf_append_time(0,"TIMER0 address: %u\n", TIMER0);
    //     printf_to_buf_append_time(0,"TIMER1 address: %u\n", TIMER1);
    //     printf_to_buf_append_time(0,"TIMER2 address: %u\n", TIMER2);
    //     printf_to_buf_append_time(0,"TIMER3 address: %u\n", TIMER3);
    //     printf_to_buf_append_time(0,"TIMER4 address: %u\n", TIMER4);
    //     assert(0);
    // }

    if (timers__init_timer_cmu(MICROS_COUNT_TIMER) == false)
    {
        return false;
    }

    // CMU_Clock_TypeDef clock_to_enable;

    //   if (MICROS_COUNT_TIMER == TIMER0)
    //   {
    //     clock_to_enable = cmuClock_TIMER0;
    //   }
    //   else if (MICROS_COUNT_TIMER == TIMER1)
    //   {
    //     clock_to_enable = cmuClock_TIMER1;
    //   }
    //   #if defined(TIMER3)
    //   else if (MICROS_COUNT_TIMER == TIMER2)
    //   {
    //     clock_to_enable = cmuClock_TIMER2;
    //   }
    //   #endif
    //   #if defined(TIMER3)
    //   else if (MICROS_COUNT_TIMER == TIMER3)
    //   {
    //     clock_to_enable = cmuClock_TIMER3;
    //   }
    //   #endif
    //   #if defined(TIMER4)
    //   else if (MICROS_COUNT_TIMER == TIMER4)
    //   {
    //     clock_to_enable = cmuClock_TIMER4;
    //   }
    //   #endif
    //   else
    //   {
    //     //BAD RGB1_TIMER value
    //     //TODO Handle Error
    //     printf_to_buf_append_time(0,"Bad MICROS_TIMER choice\n");
    //     assert(0);
    //   }

    // CMU_ClockEnable(clock_to_enable, true);

    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
    TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
    timerInit.enable = false;
    timerInit.oneShot = false;
    timerInit.clkSel = timerClkSelCascade;
    timerCCInit.mode = timerCCModePWM;

    TIMER_Init(MICROS_COUNT_TIMER, &timerInit);
    TIMER_InitCC(MICROS_COUNT_TIMER, 0, &timerCCInit);

    uint32_t TIMER_TOP = 0xFFFFFFFF;
    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"Top Value: %u\n", (unsigned int)TIMER_TOP));
    TIMER_TopSet(MICROS_COUNT_TIMER, TIMER_TOP);

    TIMER_Enable(MICROS_COUNT_TIMER, true);

    DEBUG_PERIPHERALS_LOG(microseconds__printf(0,"-- Started Microsecond Count Timer\n"));
    return true;
}

/**
 * @brief De-initializes all microsecond timing systems.
 * Wrapper that calls microseconds__deinit_microsecond() for cleanup.
 * 
 * @param None
 * @return void
 */
void microseconds__deinit(void)
{
    TIMER_Enable(MICROS_TICK_TIMER, false);
    TIMER_Enable(MICROS_COUNT_TIMER, false);

    TIMER_Reset(MICROS_TICK_TIMER);
    TIMER_Reset(MICROS_COUNT_TIMER);
}

// -----------------------------------------------------------------------------
//                     Microsecond Timer General End
// -----------------------------------------------------------------------------
