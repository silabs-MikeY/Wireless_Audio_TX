#ifndef VDAC_H_
#define VDAC_H_

#include "em_gpio.h"
#include "em_cmu.h"
#include "em_vdac.h"
#include "stddef.h"
#include "hardware_config.h"
#include "sl_gpio.h"
#include "ADC.h"

void vdac__init_vdac(void);
void vdac__start_output(void);

#endif /* VDAC_H_ */
