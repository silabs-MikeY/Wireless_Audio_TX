#include <assert.h>
#include <stdarg.h>

#include "generic.h"
#include "print.h"
#include "print_interfacing.h"

__attribute__((weak)) void generic__printf(bool add_timestamp, const char *format, ...)
{
  (void)add_timestamp;
  (void)format;
}

//MEMCPY not volatile safe

static BURAM_contents_t BURAM_contents[NUMBER_OF_BURTC_REGISTERS_USED];
static const uint32_t BURAM_MAGIC_VALUE = 0x4255524Du;

static void generic__initialize_buram_contents(void)
{
  for (uint32_t i = 0; i < NUMBER_OF_BURTC_REGISTERS_USED; i++)
  {
    BURAM_contents[i].contents = 0;
    BURAM_contents[i].canary_before = CANARY_BEFORE;
    BURAM_contents[i].canary_after = CANARY_AFTER;
  }

  BURAM_contents[BURAM_MAGIC].contents = BURAM_MAGIC_VALUE;
}

/**
 * @brief Determines the reason for the last device reset.
 * Reads EMU_RSTCAUSE register and maps to reset_causes_xg24_t enum.
 * Checks in order: VREGIN, SETAMPER, BOD variants, SYSREQ, LOCKUP, WDOG, EM4, PIN, POR.
 * Returns UNKNOWN if cause doesn't match known types.
 * 
 * @param None
 * @return Reset cause enum value
 */
reset_causes_xg24_t get_reset_cause(void)
{
  uint32_t reset_cause = EMU->RSTCAUSE;


  if (reset_cause & EMU_RSTCAUSE_VREGIN)
    {
      return VREGIN;
    }
  else if (reset_cause & EMU_RSTCAUSE_SETAMPER)
    {
      return SETAMPER;
    }
  else if (reset_cause & EMU_RSTCAUSE_IOVDD0BOD)
    {
      return IOVDD0BOD;
    }
  else if (reset_cause & EMU_RSTCAUSE_AVDDBOD)
    {
      return AVDDBOD;
    }
  else if (reset_cause & EMU_RSTCAUSE_DECBOD)
    {
      return DECBOD;
    }
  else if (reset_cause & EMU_RSTCAUSE_DVDDLEBOD)
    {
      return DVDDLEBOD;
    }
  else if (reset_cause & EMU_RSTCAUSE_DVDDBOD)
    {
      return DVDDBOD;
    }
  else if (reset_cause & EMU_RSTCAUSE_SYSREQ)
    {
      return SYSREQ;
    }
  else if (reset_cause & EMU_RSTCAUSE_LOCKUP)
    {
      return LOCKUP;
    }
  else if (reset_cause & EMU_RSTCAUSE_WDOG1)
    {
      return WDOG_1;
    }
  else if (reset_cause & EMU_RSTCAUSE_WDOG0)
    {
      return WDOG_0;
    }
  else if (reset_cause & EMU_RSTCAUSE_EM4)
    {
      return EM4;
    }
  else if (reset_cause & EMU_RSTCAUSE_PIN)
    {
      return PIN;
    }
  else if (reset_cause & EMU_RSTCAUSE_POR)
    {
      return POR;
    }
  return UNKNOWN;
}

/**
 * @brief Processes device reset and initializes backup RAM (BURAM).
 * Logs reset cause, initializes or retrieves BURAM contents with canaries.
 * Increments reset counter for detected cause.
 * Prints all reset counters for debugging.
 * Called at startup to detect reset history.
 * 
 * @param None
 * @return void
 */
void process_device_reset(void)
{
  //  CUSTOM_ASSERT();
  //#pragma message "Compiling file: " __FILE__
  //
  //
  //#define MY_VERSION 123
  //#define STRINGIFY(x) #x
  //#define TO_STRING(x) STRINGIFY(x)
  //
  //  uint32_t a = MY_COUNTER;
  //
  //#pragma message("Current version: " STR(a))
  //#pragma message("Current version: " STR(a))

  //  CUSTOM_MACRO()

  //  TRACE_COUNTER();


  generic__printf(0, "Reset, Checking Casuse \n");

  reset_causes_xg24_t reset_cause = get_reset_cause();

  CMU_ClockEnable(cmuClock_BURAM, true);

  generic__printf(0, "Reset Cause : %s\n", NUMBER_OF_RMU_CAUSE_TYPES_NAMES[reset_cause]);

  bool buram_is_valid = (BURAM->RET[BURAM_MAGIC].REG == BURAM_MAGIC_VALUE);

  if ((reset_cause == POR) ||
      (reset_cause == DVDDBOD) ||
      (reset_cause == DVDDLEBOD) ||
      (reset_cause == DECBOD) ||
      (reset_cause == AVDDBOD) ||
      (reset_cause == IOVDD0BOD) ||
      (buram_is_valid == false))
    {
      generic__printf(0, "Initializing BURAM:\n");
      generic__initialize_buram_contents();
    }
  else
    {
      generic__printf(0, "Fetching BURAM:\n");
      for (uint32_t i=0 ; i<NUMBER_OF_BURTC_REGISTERS_USED ; i++)
        {
          BURAM_contents[i].contents = BURAM->RET[i].REG;
          BURAM_contents[i].canary_before = CANARY_BEFORE;
          BURAM_contents[i].canary_after = CANARY_AFTER;
        }
    }



  BURAM_contents[reset_cause].contents++;

  generic__printf(0, "Reset Counters:\n");
  for (uint32_t i=0 ; i<NUMBER_OF_RMU_CAUSE_TYPES ; i++)
    {
      generic__printf(0, "- %s : %u\n", NUMBER_OF_RMU_CAUSE_TYPES_NAMES[i], (unsigned int)BURAM_contents[i].contents);
    }

  switch(reset_cause)
  {
    case VREGIN:
      break;
    case SETAMPER:
      break;
    case IOVDD0BOD:
      break;
    case AVDDBOD:
      break;
    case DECBOD:
      break;
    case DVDDLEBOD:
      break;
    case DVDDBOD:
      break;
    case SYSREQ:
      break;
    case LOCKUP:
      break;
    case WDOG_1:
      break;
    case WDOG_0:
      break;
    case EM4:
      break;
    case PIN:
      break;
    case POR:
      break;
    default:
      break;
  }
}

/**
 * @brief Validates canary values in BURAM to detect memory corruption.
 * Checks both canary_before and canary_after fields in all BURAM entries.
 * Returns false immediately if any canary is invalid (memory corruption detected).
 * Critical for safety; should trigger hard reset if false.
 * 
 * @param None
 * @return true if all canaries valid, false if corruption detected
 */
bool validate_cananries(void)
{
  for (uint32_t i=0 ; i<NUMBER_OF_BURTC_REGISTERS_USED ; i++)
    {
      if (BURAM_contents[i].canary_before != CANARY_BEFORE)
        {
          return false;
        }
      if (BURAM_contents[i].canary_after != CANARY_AFTER)
        {
          return false;
        }
    }
  return true;
}

/**
 * @brief Saves BURAM contents to hardware backup registers.
 * Validates canaries first to detect corruption.
 * Writes all BURAM_contents[] entries to BURAM->RET[] registers.
 * Triggers hard reset on canary failure (memory corruption detected).
 * Called before power loss or system reset to preserve state.
 * 
 * @param None
 * @return void
 */
void save_BURTC(void)
{
  debug__printf_to_buf_append_time(0,"Saving BURAM:\n");
  if (validate_cananries() != true)
    {
      debug__printf_to_buf_append_time(0,"Canaries Invalid\n"); //This is really really bad and means memory corruption
      assert(0);
      CORE_ResetSystem();
    }

  for (uint32_t i=0 ; i<NUMBER_OF_BURTC_REGISTERS_USED ; i++)
    {
      BURAM->RET[i].REG = BURAM_contents[i].contents;
    }
}

/**
 * @brief Performs a hard system reset.
 * Calls CORE_ResetSystem() to trigger immediate reset.
 * Used to restart system after unrecoverable errors.
 * 
 * @param None
 * @return void (does not return)
 */
void reset_device(void)
{
  CORE_ResetSystem();
}

/**
 * @brief Copies data from volatile memory to non-volatile buffer.
 * Byte-by-byte copy to ensure volatile semantics are respected.
 * Used instead of memcpy() to prevent compiler optimizations that ignore volatile.
 * 
 * @param destination - Non-volatile destination buffer
 * @param source - Volatile source buffer
 * @param count - Number of bytes to copy
 * @return void
 */
void memcpy_from_volatile(uint8_t* destination, volatile uint8_t* source, uint32_t count)
{
  for (uint32_t i=0 ; i<count ; i++)
    {
      destination[i] = source[i];
    }
}

/**
 * @brief Copies data from non-volatile buffer to volatile memory.
 * Byte-by-byte copy to ensure volatile semantics are respected.
 * Used instead of memcpy() to prevent compiler optimizations that ignore volatile.
 * 
 * @param destination - Volatile destination buffer
 * @param source - Non-volatile source buffer
 * @param count - Number of bytes to copy
 * @return void
 */
void memcpy_to_volatile(volatile uint8_t* destination, uint8_t* source, uint32_t count)
{
  for (uint32_t i=0 ; i<count ; i++)
    {
      destination[i] = source[i];
    }
}

/**
 * @brief Copies data between volatile memory locations.
 * Byte-by-byte copy to ensure volatile semantics are respected.
 * Used instead of memcpy() to prevent compiler optimizations that ignore volatile.
 * 
 * @param destination - Volatile destination buffer
 * @param source - Volatile source buffer
 * @param count - Number of bytes to copy
 * @return void
 */
void memcpy_from_volatile_to_volatile(volatile uint8_t* destination, volatile uint8_t* source, uint32_t count)
{
  for (uint32_t i=0 ; i<count ; i++)
    {
      destination[i] = source[i];
    }
}

/**
 * @brief Sets volatile memory to a specific value.
 * Byte-by-byte set to ensure volatile semantics are respected.
 * Used instead of memset() to prevent compiler optimizations that ignore volatile.
 * 
 * @param destination - Volatile buffer to set
 * @param value - Byte value to write to all positions
 * @param count - Number of bytes to set
 * @return void
 */
void memset_volatile(volatile uint8_t* destination, uint8_t value, uint32_t count)
{
  for (uint32_t i=0 ; i<count ; i++)
    {
      destination[i] = value;
    }
}

/**
 * @brief Custom assertion handler (currently unimplemented).
 * Intended to log assertion failures with file, line, and condition.
 * Placeholder for future debug/assert enhancement.
 * 
 * @param counter - Assertion counter/index
 * @param file - Source file path where assertion occurred
 * @param line - Line number where assertion occurred
 * @return void
 */
void custom_assert(uint32_t counter, const char* file,uint32_t line)
{
  (void)counter;
  (void)file;
  (void)line;
  //printf("Assertion failed: counter %u, file %s, line %u\n", (unsigned int)counter, file, (unsigned int)line);
  //fflush(stdout);
  //debug_print_all_buffers();

  //__disable_irq();

  assert(0);
  while (1)
  {
  }
}
