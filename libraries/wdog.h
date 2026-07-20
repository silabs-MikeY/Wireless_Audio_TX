#ifndef WDOG_H_
#define WDOG_H_

#include "em_wdog.h"
#include "em_core.h"
#include "em_chip.h"
#include "sl_core.h"
#include "generic.h"
#include "em_cmu.h"
#include "em_rmu.h"
#include <stdbool.h>
#include "print_interfacing.h"

void wdog__printf(bool add_timestamp, const char *format, ...);
void wdog__on_irq(void);

void wdog__init(void);

#endif /* WDOG_H_ */
