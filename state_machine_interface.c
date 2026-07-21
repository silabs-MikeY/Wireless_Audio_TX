#include "state_machine_interface.h"
#include "adc.h"
#include "app_process.h"
#include "audio_intensity.h"
#include "audio_pipeline.h"
#include "audio_ring_buffer.h"
#include "button.h"
#include "counter_interface.h"
#include "counters.h"
#include "ldma_manager.h"
#include "microseconds.h"
#include "print_interfacing.h"
#include "radio_base.h"
#include "radio_packet_buffers.h"
#include "rgb.h"
#include "scheduler.h"
#include "sl_core.h"
#include "state_machine.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "wdog.h"

static bool state_machine__init_peripherals(void);
static bool state_machine__de_init_peripherals(void);
static unsigned int adc_ldma_channel_left = LDMA_MANAGER_INVALID_CHANNEL;
static unsigned int adc_ldma_channel_right = LDMA_MANAGER_INVALID_CHANNEL;

// -----------------------------------------------------------------------------
//                     State Machine State Handlers
// -----------------------------------------------------------------------------

bool state_machine__process_state_init(states_t last_state) {
  if (last_state != INIT) {
    state_machine__de_init_peripherals(); // TODO Implement this
    print_interfacing__printf(0, false, "\n**Init Peripherals Start\n");
    if (state_machine__init_peripherals() == true) {
      print_interfacing__printf(0, false, "\n**Init Peripherals Successful\n");
      rgb__start_radio_status_blink_force();
      state_machine__set_next_state(RUNNING);
    } else {
      print_interfacing__flush_blocking();
      assert(0);

      print_interfacing__printf(0, false, "\n**Init Peripherals Failed\n");
      state_machine__set_next_state(ERROR);
    }
    return true;
  }
  return false;
}

bool state_machine__process_state_running(states_t last_state) {
  if (last_state != RUNNING) {
  }
  return false;
}

bool state_machine__process_state_error(states_t last_state) {
  // radio__stop_rx();
  if (last_state == RUNNING) {
    state_machine__set_next_state(INIT);
    rgb__start_radio_status_blink_force();
    // set_reset_loop_flag();
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
//                     State Machine State Handlers End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Peripheral Init / Deinit
// -----------------------------------------------------------------------------

static bool state_machine__init_peripherals(void) {
  print_interfacing__printf(0, false, "\n");
  print_interfacing__printf(0, false, "\n- Start WDOG Init\n");
  if (!wdog__init()) {
    print_interfacing__printf(0, false, "WDOG init failed\n");
    return false;
  }
  print_interfacing__printf(0, false, "- End WDOG Init\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Starting Microsecond Init\n");
  if (!microseconds__init_microsecond()) {
    print_interfacing__printf(0, false, "Microsecond init failed\n");
    return false;
  }
  print_interfacing__printf(0, false, "- End Microsecond Init\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Start Counters Init\n");
  if (!counters__init()) {
    print_interfacing__printf(0, false, "Counters init failed\n");
    return false;
  }
  if (!counter_interface__init()) {
    print_interfacing__printf(0, false, "Counter interface init failed\n");
    return false;
  }
  print_interfacing__printf(0, false, "- End Counters Init\n");
  print_interfacing__flush_blocking();

  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"micros : %u\n",
  // (unsigned int)microseconds__get_micros_count()));
  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"\n- Start Scheduler
  // Init\n");) scheduler__init_SysTick();
  // DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"- End Scheduler
  // Init\n");) DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"micros :
  // %u\n", (unsigned int)microseconds__get_micros_count()));

  // print_interfacing__printf(0, false, "micros : %u\n",
  //                           (unsigned int)microseconds__get_micros_count());
  print_interfacing__printf(0, false, "\n- Start Audio Buffers Init\n");
  print_interfacing__flush_blocking();
  bool is_stereo = app_process__is_audio_stereo();
  bool enable_encoder = app_process__is_audio_encoder_enabled();

  if (!ring_buffer__init(is_stereo, enable_encoder)) {
    print_interfacing__printf(0, false, "Ring buffer init failed\n");
    return false;
  }
  if (!audio_pipeline__init(is_stereo, enable_encoder)) {
    print_interfacing__printf(0, false, "Audio pipeline init failed\n");
    return false;
  }
  print_interfacing__printf(0, false, "- End Audio Buffers Init\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Start Radio Init\n");
  // radio__init_packet_buffers();
  if (!radio_packet_buffers__init()) {
    print_interfacing__printf(0, false, "Radio packet buffers init failed\n");
    return false;
  }
  if (!radio__init()) {
    print_interfacing__printf(0, false, "Radio init failed\n");
    return false;
  }
  print_interfacing__printf(0, false, "- End Radio Init\n");
  print_interfacing__flush_blocking();

  // print_interfacing__printf(0, false, "micros : %u\n",
  //                           (unsigned int)microseconds__get_micros_count());
  print_interfacing__printf(0, false, "\n- Start RGB Init\n");
  if (!rgb__init()) {
    print_interfacing__printf(0, false, "RGB init failed\n");
    return false;
  }
  print_interfacing__printf(0, false, "- End RGB Init\n");
  // print_interfacing__printf(0, false, "micros : %u\n",
  //                           (unsigned int)microseconds__get_micros_count());
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Start Audio Intensity Init\n");
  if (!audio_intensity__init(false, sizeof(int16_t), ADC_BUFFER_SIZE >> 1)) {
    print_interfacing__printf(0, false, "Audio intensity init failed\n");
    return false;
  }
  print_interfacing__printf(0, false, "- End Audio Intensity Init\n");
  print_interfacing__flush_blocking();

  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start I2C Init - "));
  //  i2c__init();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Test I2C"));
  //  i2c__test();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End I2C Init\n"));

  // print_interfacing__printf(0, false, "micros : %u\n",
  //                           (unsigned int)microseconds__get_micros_count());
  print_interfacing__printf(0, false, "\n- Start Button Init - \n");
  if (!button__init()) {
    print_interfacing__printf(0, false, "Button init failed\n");
    return false;
  }
  print_interfacing__printf(0, false, "- End Button Init\n");
  // print_interfacing__printf(0, false, "micros : %u\n",
  //                           (unsigned int)microseconds__get_micros_count());
  print_interfacing__flush_blocking();


  print_interfacing__printf(0, false, "\n- Start ADC Init - \n");
  if (!ldma_manager__allocate(&adc_ldma_channel_left)) {
    print_interfacing__printf(0, false, "ADC left LDMA allocation failed\n");
    print_interfacing__flush_blocking();
    return false;
  }
  if (!ldma_manager__allocate(&adc_ldma_channel_right)) {
    print_interfacing__printf(0, false, "ADC right LDMA allocation failed\n");
    ldma_manager__free(adc_ldma_channel_left);
    adc_ldma_channel_left = LDMA_MANAGER_INVALID_CHANNEL;
    print_interfacing__flush_blocking();
    return false;
  }

  print_interfacing__printf(0, false, "ADC left CH: %u, ADC right CH: %u\n", (unsigned int)adc_ldma_channel_left, (unsigned int)adc_ldma_channel_right);
  print_interfacing__flush_blocking();


  if (!adc__init(is_stereo, adc_ldma_channel_left, adc_ldma_channel_right)) {
    print_interfacing__printf(0, false, "ADC init failed\n");
    ldma_manager__free(adc_ldma_channel_left);
    adc_ldma_channel_left = LDMA_MANAGER_INVALID_CHANNEL;
    ldma_manager__free(adc_ldma_channel_right);
    adc_ldma_channel_right = LDMA_MANAGER_INVALID_CHANNEL;
    print_interfacing__flush_blocking();
    return false;
  }
  print_interfacing__printf(0, false, "- End ADC Init\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n");

  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start Timer Init"));
  //  vdac__init_timer();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End Timer Init\n"));
  //
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"Start VDAC Init"));
  //  vdac__init_vdac();
  //  DEBUG_PERIPHERALS_LOG(printf_to_buf_append_time(0,"End VDAC Init\n"));

  return true;
}

static bool state_machine__de_init_peripherals(void) {
  // CORE_DECLARE_IRQ_STATE;
  // CORE_ENTER_CRITICAL();

  print_interfacing__printf(0, false, "\n- Starting RADIO Deinit\n");
  radio__deinit();
  print_interfacing__printf(0, false, "- End Radio Deinit\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Starting Microsecond Deinit\n");
  microseconds__deinit_microsecond();
  print_interfacing__printf(0, false, "- End Microsecond Deinit\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Starting SysTick Deinit\n");
  scheduler__deinit_SysTick();
  print_interfacing__printf(0, false, "- End SysTick Deinit\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Starting Button Deinit\n");
  button__deinit();
  print_interfacing__printf(0, false, "- End Button Deinit\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Starting RGB Deinit\n");
  rgb__deinit();
  print_interfacing__printf(0, false, "- End RGB Deinit\n");
  print_interfacing__flush_blocking();

  print_interfacing__printf(0, false, "\n- Starting ADC Deinit\n");
  adc__deinit();
  print_interfacing__printf(0, false, "- End ADC Deinit\n");
  print_interfacing__flush_blocking();

  if (adc_ldma_channel_left != LDMA_MANAGER_INVALID_CHANNEL) {
    ldma_manager__free(adc_ldma_channel_left);
    adc_ldma_channel_left = LDMA_MANAGER_INVALID_CHANNEL;
  }
  if (adc_ldma_channel_right != LDMA_MANAGER_INVALID_CHANNEL) {
    ldma_manager__free(adc_ldma_channel_right);
    adc_ldma_channel_right = LDMA_MANAGER_INVALID_CHANNEL;
  }

  // CORE_EXIT_CRITICAL();
  return false;
}

// -----------------------------------------------------------------------------
//                     Peripheral Init / Deinit End
// -----------------------------------------------------------------------------
