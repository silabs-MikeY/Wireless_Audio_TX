#ifndef GENERIC_H_
#define GENERIC_H_

#include "em_core.h"
#include "em_chip.h"
#include <stdio.h>
#include "em_cmu.h"
#include "em_rmu.h"
#include "em_emu.h"

#define BIT(bit) (1U << (bit))

#define NUMBER_OF_BURAM_REGISTERS_AVAILABLE 32
#define CANARY_BEFORE 0x5F1B
#define CANARY_AFTER 0x3B96


typedef struct BURAM_contents_s {
  uint32_t canary_before;
  uint32_t contents;
  uint32_t canary_after;
} BURAM_contents_t;

typedef enum
{
  UNKNOWN,
  VREGIN,
  SETAMPER,
  IOVDD0BOD,
  AVDDBOD,
  DECBOD,
  DVDDLEBOD,
  DVDDBOD,
  SYSREQ,
  LOCKUP,
  WDOG_1,
  WDOG_0,
  EM4,
  PIN,
  POR,
  //Add new RMU reset causes here

  NUMBER_OF_RMU_CAUSE_TYPES, // Gets the number of RMU reset causes
} reset_causes_xg24_t;

static const char *NUMBER_OF_RMU_CAUSE_TYPES_NAMES[] __attribute__ ((used)) = {
    "UNKNOWN",
    "VREGIN",
    "SETAMPER",
    "IOVDD0BOD",
    "AVDDBOD",
    "DECBOD",
    "DVDDLEBOD",
    "DVDDBOD",
    "SYSREQ",
    "LOCKUP",
    "WDOG_1",
    "WDOG_0",
    "EM4",
    "PIN",
    "POR",
};



typedef enum
{
  RESET_REGISTERS = NUMBER_OF_RMU_CAUSE_TYPES-1, // Don't touch this

  //add new registers here

  NUMBER_OF_BURTC_REGISTERS_USED // Don't touch this
} BURTC_register_map_t;

_Static_assert(NUMBER_OF_BURTC_REGISTERS_USED < NUMBER_OF_BURAM_REGISTERS_AVAILABLE,"TOO MANY BURTC BUFFERS USED");


void process_device_reset(void);

void memcpy_from_volatile(uint8_t* destination, volatile uint8_t* source, uint32_t count);
void memcpy_to_volatile(volatile uint8_t* destination, uint8_t* source, uint32_t count);
void memcpy_from_volatile_to_volatile(volatile uint8_t* destination, volatile uint8_t* source, uint32_t count);
void memset_volatile(volatile uint8_t* destination, uint8_t value, uint32_t count);
void custom_assert(uint32_t counter, const char* file,uint32_t line);
void reset_device(void);

#define MY_COUNTER __COUNTER__
#define MY_VERSION 123
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)


#define CUSTOM_ASSERT() \
    do{ \
        custom_assert(MY_COUNTER,__FILE__,__LINE__); \
    } while(0)

/*
#define CUSTOM_MACRO() \
    do{ \
        enum { CURRENT_COUNTER = __COUNTER__ }; \
        _Pragma message "Captured __COUNTER__ = " STR(CURRENT_COUNTER) \
} while(0)

#define TRACE_COUNTER() \
    enum { TRACE_COUNTER_VALUE = __COUNTER__ }; \
    _Pragma(STR(message("TRACE_COUNTER = " STR(TRACE_COUNTER_VALUE))))

  custom_assert(INCREMENT,__FILE__,__LINE__);       \
  #pragma message("Compiling this file...")         \
  )

  #define FORCE_ASSERT_EXPAND (custom_assert(__FILE__,__LINE__))
  #define CUSTOM_ASSERT (custom_assert(INCREMENT)
*/


#endif /* GENERIC_H_ */
