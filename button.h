#ifndef BUTTON_H_
#define BUTTON_H_

#include "em_gpio.h"
#include "em_cmu.h"
#include "em_vdac.h"
#include "stddef.h"
#include "hardware_config.h"
#include "sl_gpio.h"
#include "app_process.h"
#include <stdio.h>
#include "scheduler.h"
// #include "radio.h"

void button__init(void);
void button__deinit(void);

#endif /* BUTTON_H_ */
