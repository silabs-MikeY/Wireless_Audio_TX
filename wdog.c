#include "wdog.h"

void WDOG0_IRQHandler(void)
{
  uint32_t flags = WDOGn_IntGet(WDOG0);
  WDOGn_IntClear(WDOG0, flags);

  printf("WDOG HIT\n");

  reset_device();
  //assert(0);
}

void wdog__init(void)
{
  if (!(CMU->CLKEN1 & CMU_CLKEN1_WDOG1))
  {
    CMU_ClockSelectSet(cmuClock_WDOG0, cmuSelect_ULFRCO);
    CMU_ClockEnable(cmuClock_WDOG0, true);
  }

  
  if (CMU_ClockSelectGet(cmuClock_WDOG0) != cmuSelect_ULFRCO)
  {
    printf("WDOG Clock is Set Wrong: \n");
    CMU_ClockSelectSet(cmuClock_WDOG0, cmuSelect_ULFRCO);
  }

  if (!(WDOG0->EN & WDOG_EN_EN))
  {

    WDOG_Init_TypeDef wdogInit = WDOG_INIT_DEFAULT;
    wdogInit.debugRun = false;
    wdogInit.resetDisable = true;
    wdogInit.perSel = wdogPeriod_2k;

    WDOGn_IntEnable(WDOG0, WDOG_IEN_TOUT);
    NVIC_EnableIRQ(WDOG0_IRQn);

    WDOGn_Init(WDOG0, &wdogInit);

    WDOGn_Lock(WDOG0);
  }
}