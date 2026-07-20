#include <assert.h>
#include <stdio.h>

#include "ADC.h"

#include "dmadrv.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_usart.h"

#include <string.h>

static void adc__init_i2s(void);
static void adc__ldma_init(void);
static void adc__wait_for_startup_validation(void);
static void adc__print_sample_format_summary(void);
static bool DMA_left_callback(unsigned int channel, unsigned int sequenceNo,
                              void *userParam);
static bool DMA_right_callback(unsigned int channel, unsigned int sequenceNo,
                               void *userParam);

typedef struct adc_dma_buffer_s {
  volatile uint8_t data[ADC_BUFFER_SIZE] __attribute__((aligned(4)));
  bool used;
  bool processed;
  uint32_t micros_timestamp;
} adc_dma_buffer_t;

static bool adc__start_left_dma_transfer(void);
static bool adc__start_right_dma_transfer(void);

typedef enum adc_counter_index_s {
  ADC_COUNTER_SAMPLES_RECEIVED_LEFT = 0,
  ADC_COUNTER_SAMPLES_RECEIVED_RIGHT = 1,
  ADC_COUNTER_BYTES_RECEIVED_LEFT = 2,
  ADC_COUNTER_BYTES_RECEIVED_RIGHT = 3,
  ADC_COUNTER_LEFT_BUFFERS_IN_USE = 4,
  ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN = 5,
  ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX = 6,
  ADC_COUNTER_RIGHT_BUFFERS_IN_USE = 7,
  ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN = 8,
  ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX = 9,
  ADC_NUMBER_OF_COUNTERS
} adc_counter_index_t;

static volatile uint32_t adc_counter_values[ADC_NUMBER_OF_COUNTERS] = {0};
static const char *adc_counter_names[ADC_NUMBER_OF_COUNTERS] = {
    "adc_samples_received_left",    "adc_samples_received_right",
    "adc_bytes_received_left",      "adc_bytes_received_right",
    "adc_left_buffers_in_use",      "adc_left_buffers_in_use_min",
    "adc_left_buffers_in_use_max",  "adc_right_buffers_in_use",
    "adc_right_buffers_in_use_min", "adc_right_buffers_in_use_max",
};

static bool library_initialized = false;

LDMA_Descriptor_t leftDesc[2];
LDMA_Descriptor_t rightDesc[2];
LDMA_TransferCfg_t leftCfg;
LDMA_TransferCfg_t rightCfg;

static adc_dma_buffer_t leftBuffers[ADC_DMA_BUFFER_COUNT];
static adc_dma_buffer_t rightBuffers[ADC_DMA_BUFFER_COUNT];
static volatile uint8_t leftDmaPingPongBuffers[2][ADC_BUFFER_SIZE]
    __attribute__((aligned(4)));
static volatile uint8_t rightDmaPingPongBuffers[2][ADC_BUFFER_SIZE]
    __attribute__((aligned(4)));

static volatile uint32_t left_dma_buffer_index = 0;
static volatile uint32_t right_dma_buffer_index = 0;

volatile bool new_data_ready_flag = false;
volatile uint32_t *new_data_pointer;

volatile uint8_t *leftBuffer_to_process;
volatile uint8_t *rightBuffer_to_process;

volatile uint32_t (*leftBuffer_output)[ADC_BUFFER_SIZE];
volatile uint32_t (*rightBuffer_output)[ADC_BUFFER_SIZE];

volatile uint8_t leftBuffer_converted[ADC_BUFFER_SIZE]
    __attribute__((aligned(4)));
;
volatile uint8_t rightBuffer_converted[ADC_BUFFER_SIZE]
    __attribute__((aligned(4)));
;

volatile uint32_t leftBuffer_converted_word[ADC_BUFFER_SIZE / 4];
volatile uint32_t rightBuffer_converted_word[ADC_BUFFER_SIZE / 4];

volatile uint8_t leftBuffer_test[ADC_BUFFER_SIZE] __attribute__((aligned(4)));
;
volatile uint8_t rightBuffer_test[ADC_BUFFER_SIZE] __attribute__((aligned(4)));
;

unsigned int LDMA_CHANNEL_LEFT;
unsigned int LDMA_CHANNEL_RIGHT;

extern uint32_t start;
extern volatile uint32_t debug_signals[10];

static bool stereo_flag = false;

static void adc__update_left_buffer_counters(void);
static void adc__update_right_buffer_counters(void);
static void
adc__store_completed_left_buffer(const volatile uint8_t *source_buffer);
static void
adc__store_completed_right_buffer(const volatile uint8_t *source_buffer);

static volatile bool startup_left_buffer_ready = false;
static volatile bool startup_right_buffer_ready = false;

static const uint32_t adc_sample_size_bytes = 4;
static const bool adc_sample_is_little_endian = true;
static const bool adc_sample_is_left_justified = true;

bool new_packet_ready_left_flag = false;
bool new_packet_ready_right_flag = false;

uint8_t *new_packet_left_pointer = 0;
uint8_t *new_packet_right_pointer = 0;

__attribute__((weak)) uint32_t adc__get_microsecond_ticks(void) {
  return DWT->CYCCNT;
}

__attribute__((weak)) void adc__printf(bool add_timestamp, const char *format,
                                       ...) {
  (void)add_timestamp;
  (void)format;
  return;
}

/**
 * @brief Gets the next audio packet ready for processing from the right
 * channel. Returns pointer to the oldest right channel buffer if available.
 * Clears the ready flag after returning data.
 *
 * @param data_pointer - Output pointer to audio data (pointer to pointer)
 * @return true if packet available, false if no data ready
 */
bool adc__get_new_packet_ready_for_processing(uint8_t **data_pointer) {
  if (new_packet_ready_right_flag == true) {
    if (new_packet_right_pointer != 0) {
      new_packet_ready_right_flag = false;
      *data_pointer = new_packet_right_pointer;
      return true;
    }
  }
  return false;
}

/**
 * @brief Gets the audio stereo flag state.
 * Indicates whether the ADC is configured for stereo (true) or mono (false)
 * mode.
 *
 * @param None
 * @return true if stereo mode enabled, false if mono
 */
bool adc__get_audio_stereo_flag(void) { return stereo_flag; }

uint32_t adc__get_sample_size_bytes(void) { return adc_sample_size_bytes; }

bool adc__is_sample_little_endian(void) { return adc_sample_is_little_endian; }

bool adc__is_sample_left_justified(void) {
  return adc_sample_is_left_justified;
}

uint32_t adc__get_number_of_counters(void) { return ADC_NUMBER_OF_COUNTERS; }

const char *adc__get_counter_name(uint32_t counter_index) {
  if (counter_index >= ADC_NUMBER_OF_COUNTERS) {
    return NULL;
  }

  return adc_counter_names[counter_index];
}

volatile uint32_t *adc__get_counter_address(uint32_t counter_index) {
  if (counter_index >= ADC_NUMBER_OF_COUNTERS) {
    return NULL; // Invalid index
  }

  return (volatile uint32_t *)&adc_counter_values[counter_index];
}

void adc__reset_counters(void) {
  adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_LEFT] = 0;
  adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_RIGHT] = 0;
  adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_LEFT] = 0;
  adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_RIGHT] = 0;
  adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE] = 0;
  adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN] = 0;
  adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX] = 0;
  adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE] = 0;
  adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN] = 0;
  adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX] = 0;
}

static void adc__update_left_buffer_counters(void) {
  uint32_t current_count = adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE];

  if (current_count < adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN]) {
    adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN] = current_count;
  }

  if (current_count > adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX]) {
    adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX] = current_count;
  }
}

static void adc__update_right_buffer_counters(void) {
  uint32_t current_count = adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE];

  if (current_count <
      adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN]) {
    adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN] = current_count;
  }

  if (current_count >
      adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX]) {
    adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX] = current_count;
  }
}

bool adc__get_oldest_left_dma_buffer(uint8_t **buffer, uint32_t *buffer_index,
                                     uint32_t *buffer_size) {
  uint32_t oldest_index_timestamp = 0xFFFFFFFF;
  bool found = false;

  if ((buffer == NULL) || (buffer_index == NULL) || (buffer_size == NULL)) {
    return false;
  }

  for (uint32_t i = 0; i < ADC_DMA_BUFFER_COUNT; i++) {
    if ((leftBuffers[i].used == true) && (leftBuffers[i].processed == false)) {
      if (leftBuffers[i].micros_timestamp < oldest_index_timestamp) {
        oldest_index_timestamp = leftBuffers[i].micros_timestamp;
        *buffer_index = i;
        found = true;
      }
    }
  }

  if (found == true) {
    *buffer = (uint8_t *)leftBuffers[*buffer_index].data;
    *buffer_size = sizeof(leftBuffers[*buffer_index].data);
  }

  return found;
}

bool adc__get_oldest_right_dma_buffer(uint8_t **buffer, uint32_t *buffer_index,
                                      uint32_t *buffer_size) {
  uint32_t oldest_index_timestamp = 0xFFFFFFFF;
  bool found = false;

  if ((buffer == NULL) || (buffer_index == NULL) || (buffer_size == NULL)) {
    return false;
  }

  for (uint32_t i = 0; i < ADC_DMA_BUFFER_COUNT; i++) {
    if ((rightBuffers[i].used == true) &&
        (rightBuffers[i].processed == false)) {
      if (rightBuffers[i].micros_timestamp < oldest_index_timestamp) {
        oldest_index_timestamp = rightBuffers[i].micros_timestamp;
        *buffer_index = i;
        found = true;
      }
    }
  }

  if (found == true) {
    *buffer = (uint8_t *)rightBuffers[*buffer_index].data;
    *buffer_size = sizeof(rightBuffers[*buffer_index].data);
  }

  return found;
}

void adc__mark_left_dma_buffer_stale(uint32_t buffer_index) {
  if (buffer_index < ADC_DMA_BUFFER_COUNT) {
    if ((leftBuffers[buffer_index].used == true) &&
        (leftBuffers[buffer_index].processed == false) &&
        (adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE] > 0)) {
      adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE]--;
      adc__update_left_buffer_counters();
    }
    leftBuffers[buffer_index].processed = true;
  }
}

void adc__mark_right_dma_buffer_stale(uint32_t buffer_index) {
  if (buffer_index < ADC_DMA_BUFFER_COUNT) {
    if ((rightBuffers[buffer_index].used == true) &&
        (rightBuffers[buffer_index].processed == false) &&
        (adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE] > 0)) {
      adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE]--;
      adc__update_right_buffer_counters();
    }
    rightBuffers[buffer_index].processed = true;
  }
}

/**
 * @brief Initializes the ADC subsystem.
 * Configures LDMA for DMA transfers and initializes I2S interface for audio
 * input. Must be called once at startup before audio samples are expected.
 *
 * @param None
 * @return void
 */
void adc__init(bool is_stereo) {
  if (library_initialized == true) {
    adc__printf(true, "ADC Library Already Initialized. De-Initializing\n");
    adc__deinit();
  }

  stereo_flag = is_stereo;
  adc__printf(true, "ADC Init Stereo Flag: %u\n", (unsigned int)stereo_flag);
  adc__reset_counters();
  memset((void *)leftBuffers, 0, sizeof(leftBuffers));
  memset((void *)rightBuffers, 0, sizeof(rightBuffers));
  leftBuffer_to_process = (volatile uint8_t *)leftDmaPingPongBuffers[0];
  rightBuffer_to_process = (volatile uint8_t *)rightDmaPingPongBuffers[0];
  startup_left_buffer_ready = false;
  startup_right_buffer_ready = false;
  adc__ldma_init();
  adc__init_i2s();
  USART_Enable(USART0, usartEnable);
  adc__wait_for_startup_validation();

  library_initialized = true;
  adc__printf(true, "ADC Init Complete\n");
}

/**
 * @brief De-initializes the ADC subsystem.
 * Stops USART0 (I2S interface), frees DMA channels, and cleans up resources.
 * Handles both allocated and already-freed DMA channels gracefully.
 * Called during shutdown or state transitions.
 *
 * @param None
 * @return void
 */
void adc__deinit(void) {
  adc__printf(true, "DEINITIALIZING ADC\n");

  if (CMU->CLKEN0 & CMU_CLKEN0_USART0) {
    adc__printf(true, "RESETTING USART\n");
    USART_Reset(USART0);
    adc__printf(true, "RESET USART\n");
  }

  // printf_to_buf_append_time(0,"STOPPING LDMA LEFT\n");
  // LDMA_StopTransfer(LDMA_CHANNEL_LEFT);
  // printf_to_buf_append_time(0,"STOPPED LDMA LEFT\n");

  // printf_to_buf_append_time(0,"STOPPING LDMA RIGHT\n");
  // LDMA_StopTransfer(LDMA_CHANNEL_RIGHT);
  // printf_to_buf_append_time(0,"STOPPED LDMA RIGHT\n");

  if (!((LDMA_CHANNEL_LEFT == 0) && (LDMA_CHANNEL_RIGHT == 0))) {
    Ecode_t DMADRV_return_status;

    DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_LEFT);
    DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_LEFT);
    if (DMADRV_return_status == ECODE_EMDRV_DMADRV_OK) {
      adc__printf(true, "DEINITIALIZED LEFT DMA CHANNEL: %u\n",
                  (unsigned int)LDMA_CHANNEL_LEFT);
    } else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_ALREADY_FREED) {
      adc__printf(true, "LEFT DMA CHANNEL: %u already freed\n",
                  (unsigned int)LDMA_CHANNEL_LEFT);
    } else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_NOT_INITIALIZED) {
      assert(0);
    }

    DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_RIGHT);
    DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_RIGHT);
    if (DMADRV_return_status == ECODE_EMDRV_DMADRV_OK) {
      adc__printf(true, "DEINITIALIZED RIGHT DMA CHANNEL: %u\n",
                  (unsigned int)LDMA_CHANNEL_RIGHT);
    } else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_ALREADY_FREED) {
      adc__printf(true, "RIGHT DMA CHANNEL: %u ALREADY FREED\n",
                  (unsigned int)LDMA_CHANNEL_RIGHT);
    } else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_NOT_INITIALIZED) {
      assert(0);
    }
  }
}

bool adc__get_new_data_ready_flag(uint32_t **new_data_pointer_return) {
  if (new_data_ready_flag == true) {
    new_data_ready_flag = false;
    *new_data_pointer_return = (uint32_t *)new_data_pointer;
    return true;
  }
  return false;
}

static void adc__wait_for_startup_validation(void) {
  uint32_t start_cycles = DWT->CYCCNT;
  uint32_t timeout_cycles = CMU_ClockFreqGet(cmuClock_CORE);
  bool left_ready_logged = false;
  bool right_ready_logged = false;

  adc__printf(true, "Waiting for ADC startup buffers\n");

  while ((startup_left_buffer_ready == false) ||
         (startup_right_buffer_ready == false)) {
    if ((startup_left_buffer_ready == true) && (left_ready_logged == false)) {
      left_ready_logged = true;
      adc__printf(true, "ADC startup left buffer ready\n");
    }

    if ((startup_right_buffer_ready == true) && (right_ready_logged == false)) {
      right_ready_logged = true;
      adc__printf(true, "ADC startup right buffer ready\n");
    }

    if ((uint32_t)(DWT->CYCCNT - start_cycles) > timeout_cycles) {
      assert(0);
      // printf("ADC startup buffers validation timeout\n");
      // custom_assert(0, __FILE__, __LINE__);
    }
  }

  adc__print_sample_format_summary();

  adc__printf(true, "ADC Left buffer preview:\n");
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)leftDmaPingPongBuffers[0][0],
              (unsigned int)leftDmaPingPongBuffers[0][1],
              (unsigned int)leftDmaPingPongBuffers[0][2],
              (unsigned int)leftDmaPingPongBuffers[0][3]);
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)leftDmaPingPongBuffers[0][4],
              (unsigned int)leftDmaPingPongBuffers[0][5],
              (unsigned int)leftDmaPingPongBuffers[0][6],
              (unsigned int)leftDmaPingPongBuffers[0][7]);
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)leftDmaPingPongBuffers[0][8],
              (unsigned int)leftDmaPingPongBuffers[0][9],
              (unsigned int)leftDmaPingPongBuffers[0][10],
              (unsigned int)leftDmaPingPongBuffers[0][11]);

  adc__printf(true, "ADC Right buffer preview:\n");
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)rightDmaPingPongBuffers[0][0],
              (unsigned int)rightDmaPingPongBuffers[0][1],
              (unsigned int)rightDmaPingPongBuffers[0][2],
              (unsigned int)rightDmaPingPongBuffers[0][3]);
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)rightDmaPingPongBuffers[0][4],
              (unsigned int)rightDmaPingPongBuffers[0][5],
              (unsigned int)rightDmaPingPongBuffers[0][6],
              (unsigned int)rightDmaPingPongBuffers[0][7]);
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)rightDmaPingPongBuffers[0][8],
              (unsigned int)rightDmaPingPongBuffers[0][9],
              (unsigned int)rightDmaPingPongBuffers[0][10],
              (unsigned int)rightDmaPingPongBuffers[0][11]);

  // adc__print_startup_buffer_preview("left", leftBuffer_to_process);
  // adc__print_startup_buffer_preview("right", rightBuffer_to_process);

  adc__printf(true, "ADC startup buffers validated\n");
}

static void adc__print_sample_format_summary(void) {
  adc__printf(true, "ADC sample size: %u bytes\n",
              (unsigned int)adc__get_sample_size_bytes());
  adc__printf(true, "ADC sample appears to be %s-endian\n",
              adc__is_sample_little_endian() ? "little" : "big");
  adc__printf(true, "ADC sample appears to be %s-justified\n",
              adc__is_sample_left_justified() ? "left" : "right");
}

static bool adc__start_left_dma_transfer(void) {
  Ecode_t status = DMADRV_PeripheralMemoryPingPong(
      LDMA_CHANNEL_LEFT, dmadrvPeripheralSignal_USART0_RXDATAV,
      (void *)leftDmaPingPongBuffers[0], (void *)leftDmaPingPongBuffers[1],
      (void *)&(USART0->RXDATA), true, ADC_BUFFER_SIZE, dmadrvDataSize1,
      DMA_left_callback, NULL);

  if (status != ECODE_EMDRV_DMADRV_OK) {
    adc__printf(true, "LDMA left start error: %X\n", (unsigned int)status);
    return false;
  }

  return true;
}

static bool adc__start_right_dma_transfer(void) {
  Ecode_t status = DMADRV_PeripheralMemoryPingPong(
      LDMA_CHANNEL_RIGHT, dmadrvPeripheralSignal_USART0_RXDATAVRIGHT,
      (void *)rightDmaPingPongBuffers[0], (void *)rightDmaPingPongBuffers[1],
      (void *)&(USART0->RXDATA), true, ADC_BUFFER_SIZE, dmadrvDataSize1,
      DMA_right_callback, NULL);

  if (status != ECODE_EMDRV_DMADRV_OK) {
    adc__printf(true, "LDMA right start error: %X\n", (unsigned int)status);
    return false;
  }

  return true;
}

/**
 * @brief DMA callback for left channel audio transfer completion.
 * Called when LDMA finishes transferring a left audio buffer.
 * Advances the five-buffer linear ring and re-arms the next transfer.
 *
 * @param channel - LDMA channel identifier (unused)
 * @param sequenceNo - Sequence number indicating which buffer completed
 * @param userParam - User parameter (unused)
 * @return true always
 */
static bool DMA_left_callback(unsigned int channel, unsigned int sequenceNo,
                              void *userParam) {
  (void)channel;
  (void)userParam;

  // printf_to_buf_append_time(0,"Left ADC seq: %X\n",(unsigned int)sequenceNo);

  // adc__printf(true, "ADC left buffer finished: %u\n", (unsigned
  // int)left_dma_buffer_index);

  startup_left_buffer_ready = true;
  adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_LEFT] +=
      ADC_BUFFER_SIZE >> 2; // divide by sample size
  adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_LEFT] += ADC_BUFFER_SIZE;

  if (stereo_flag == false) {
    return true;
  }

  adc__store_completed_left_buffer(leftDmaPingPongBuffers[sequenceNo & 0x1]);

  return true;
}

/**
 * @brief DMA callback for right channel audio transfer completion.
 * Called when LDMA finishes transferring a right audio buffer.
 * Advances the five-buffer linear ring and re-arms the next transfer.
 * Called from interrupt context, so very timing-critical.
 *
 * @param channel - LDMA channel identifier (unused)
 * @param sequenceNo - Sequence number indicating which buffer completed
 * @param userParam - User parameter (unused)
 * @return true always
 */
static bool DMA_right_callback(unsigned int channel, unsigned int sequenceNo,
                               void *userParam) {
  (void)channel;
  (void)userParam;

  //  printf_to_buf_append_time(0,"Right ADC seq: %X\n",(unsigned
  //  int)sequenceNo);

  // adc__printf(true, "ADC right buffer finished: %u\n", (unsigned
  // int)right_dma_buffer_index);

  startup_right_buffer_ready = true;
  adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_RIGHT] +=
      ADC_BUFFER_SIZE >> 2; // divide by sample size
  adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_RIGHT] += ADC_BUFFER_SIZE;

  adc__store_completed_right_buffer(rightDmaPingPongBuffers[sequenceNo & 0x1]);

  return true;
}

static void
adc__store_completed_left_buffer(const volatile uint8_t *source_buffer) {
  if ((leftBuffers[left_dma_buffer_index].used == true) &&
      (leftBuffers[left_dma_buffer_index].processed == false)) {
    adc__printf(true,
                "ADC left DMA buffer %u not stale yet, overwriting oldest "
                "unread data\n",
                (unsigned int)left_dma_buffer_index);
  }

  memcpy((void *)leftBuffers[left_dma_buffer_index].data,
         (const void *)source_buffer, ADC_BUFFER_SIZE);

  leftBuffers[left_dma_buffer_index].used = true;
  leftBuffers[left_dma_buffer_index].processed = false;
  leftBuffers[left_dma_buffer_index].micros_timestamp =
      adc__get_microsecond_ticks();
  adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE]++;
  adc__update_left_buffer_counters();

  leftBuffer_to_process =
      (volatile uint8_t *)leftBuffers[left_dma_buffer_index].data;
  new_data_pointer = (volatile uint32_t *)leftBuffer_to_process;
  new_data_ready_flag = true;

  left_dma_buffer_index++;
  if (left_dma_buffer_index >= ADC_DMA_BUFFER_COUNT) {
    left_dma_buffer_index = 0;
  }
}

static void
adc__store_completed_right_buffer(const volatile uint8_t *source_buffer) {
  (void)source_buffer;

  if ((rightBuffers[right_dma_buffer_index].used == true) &&
      (rightBuffers[right_dma_buffer_index].processed == false)) {
    adc__printf(true,
                "ADC right DMA buffer %u not stale yet, overwriting oldest "
                "unread data\n",
                (unsigned int)right_dma_buffer_index);
  }

  #define TEST_PIPELINE_8 0
  #define TEST_PIPELINE_8_ADD_PADDING 0
  #define TEST_PIPELINE_16 0
  #define TEST_PIPELINE_16_ADD_PADDING 0
  #define TEST_PIPELINE_32 0

  const uint32_t sample_count = ADC_BUFFER_SIZE / sizeof(uint32_t);
  
  #if (TEST_PIPELINE_8 == 1)
  static uint32_t test_counter = 0;
  if (TEST_PIPELINE_8_ADD_PADDING == 1) {
    volatile uint32_t *sample_buffer =
        (volatile uint32_t *)rightBuffers[right_dma_buffer_index].data;
    for (uint32_t i = 0; i < sample_count; i++) {
      sample_buffer[i] = test_counter & 0xFF;
      test_counter++;
      if (test_counter > 0xFF) {
        test_counter = 0;
      }
    }
  } else {
    for (uint32_t i = 0; i < ADC_BUFFER_SIZE; i++) {
      rightBuffers[right_dma_buffer_index].data[i] = test_counter & 0xFF;
      test_counter++;
      if (test_counter > 0xFF) {
        test_counter = 0;
      }
    }
  }
  #elif (TEST_PIPELINE_16 == 1)
  static uint32_t test_counter = 0;
  if (TEST_PIPELINE_16_ADD_PADDING == 1) {
    volatile uint32_t *sample_buffer =
        (volatile uint32_t *)rightBuffers[right_dma_buffer_index].data;
    for (uint32_t i = 0; i < sample_count; i++) {
      sample_buffer[i] = test_counter & 0xFFFF;
      test_counter++;
      if (test_counter > 0xFFFF) {
        test_counter = 0;
      }
    }
  } else {
    volatile uint16_t *sample_buffer =
        (volatile uint16_t *)rightBuffers[right_dma_buffer_index].data;
    for (uint32_t i = 0; i < (ADC_BUFFER_SIZE / sizeof(uint16_t)); i++) {
      sample_buffer[i] = (uint16_t)test_counter;
      test_counter++;
      if (test_counter > 0xFFFF) {
        test_counter = 0;
      }
    }
  }
  #elif (TEST_PIPELINE_32 == 1)
  static uint32_t test_counter = 0;
  volatile uint32_t *sample_buffer =
      (volatile uint32_t *)rightBuffers[right_dma_buffer_index].data;
  for (uint32_t i = 0; i < (ADC_BUFFER_SIZE / sizeof(uint32_t)); i++) {
    sample_buffer[i] = test_counter;
    test_counter++;
  }
  #else
  memcpy((void *)rightBuffers[right_dma_buffer_index].data,
         (const void *)source_buffer, ADC_BUFFER_SIZE);
  #endif

  rightBuffers[right_dma_buffer_index].used = true;
  rightBuffers[right_dma_buffer_index].processed = false;
  rightBuffers[right_dma_buffer_index].micros_timestamp =
      adc__get_microsecond_ticks();
  adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE]++;
  adc__update_right_buffer_counters();

  rightBuffer_to_process =
      (volatile uint8_t *)rightBuffers[right_dma_buffer_index].data;
  new_data_pointer = (volatile uint32_t *)rightBuffer_to_process;
  new_data_ready_flag = true;

  right_dma_buffer_index++;
  if (right_dma_buffer_index >= ADC_DMA_BUFFER_COUNT) {
    right_dma_buffer_index = 0;
  }
}

/**
 * @brief USART0 RX interrupt handler for I2S overflow/error detection.
 * Reads interrupt flags, detects overflow conditions on receive.
 * Sets GPIO debug pin and clears interrupt flags.
 * Called from interrupt context.
 *
 * @param None
 * @return void
 */
void USART0_RX_IRQHandler(void) {
  // debug_signals[0] = USART_IntGet(USART0);
  GPIO_PinModeSet(gpioPortB, 0, gpioModePushPull, 0);
  USART_IntClear(USART0, USART_IntGet(USART0));
}

/**
 * @brief Initializes I2S (Integrated Inter-IC Sound) interface on USART0.
 * Configures GPIO pins for I2S clock, frame sync, data in/out.
 * Sets up I2S in slave mode with 32-bit words, left-justified format.
 * Configures split DMA for left/right channels and enables RX interrupt.
 * Must be called after LDMA is initialized.
 *
 * @param None
 * @return void
 */
void adc__init_i2s(void) {
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART0, true);

  GPIO_PinModeSet(I2S_CLK_PORT, I2S_CLK_PIN, gpioModeInputPullFilter, 0);
  GPIO_PinModeSet(I2S_CS_PORT, I2S_CS_PIN, gpioModeInputPullFilter, 0);

  GPIO_PinModeSet(I2S_DATAIN_PORT, I2S_DATAIN_PIN, gpioModeInputPullFilter, 0);
  GPIO_PinModeSet(I2S_DATAOUT_PORT, I2S_DATAOUT_PIN, gpioModePushPull, 0);

  USART_InitI2s_TypeDef init = USART_INITI2S_DEFAULT;

  init.sync.master = false;
  init.sync.enable = usartDisable;
  init.sync.databits = usartDatabits8;
  init.sync.baudrate = BAUD_RATE;
  // init.sync.autoTx = true;
  init.sync.clockMode = usartClockMode3;

  init.dmaSplit = true;
  init.mono = false;
  init.format = USART_I2SCTRL_FORMAT_W32D32;
  init.justify = usartI2sJustifyLeft;
  init.delay = false;
  // init.sync.clockMode = false;

  GPIO->USARTROUTE[0].CSROUTE =
      (I2S_CS_PORT << _GPIO_USART_CSROUTE_PORT_SHIFT) |
      (I2S_CS_PIN << _GPIO_USART_CSROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].CLKROUTE =
      (I2S_CLK_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT) |
      (I2S_CLK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);

#if (I2S_MODE == I2S_MODE_MASTER)
  GPIO->USARTROUTE[0].TXROUTE =
      (I2S_DATAOUT_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) |
      (I2S_DATAOUT_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].RXROUTE =
      (I2S_DATAIN_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) |
      (I2S_DATAIN_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN |
                                GPIO_USART_ROUTEEN_CSPEN |
                                GPIO_USART_ROUTEEN_CLKPEN;
#elif (I2S_MODE == I2S_MODE_SLAVE)
  GPIO->USARTROUTE[0].TXROUTE =
      (I2S_DATAIN_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) |
      (I2S_DATAIN_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  // GPIO->USARTROUTE[0].RXROUTE = (I2S_DATAOUT_PORT <<
  // _GPIO_USART_RXROUTE_PORT_SHIFT)| (I2S_DATAOUT_PIN <<
  // _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN |
                                GPIO_USART_ROUTEEN_CSPEN |
                                GPIO_USART_ROUTEEN_CLKPEN;
#endif

  // Enable USART interface pins

  USART_InitI2s(USART0, &init);

  USART_IntEnable(USART0, USART_IEN_RXOF | USART_IEN_RXFULL);

  // NVIC_ClearPendingIRQ(USART0_RX_IRQn);
  // NVIC_ClearPendingIRQ(USART0_TX_IRQn);
  NVIC_EnableIRQ(USART0_RX_IRQn);
  // NVIC_EnableIRQ(USART0_TX_IRQn);
}

/**
 * @brief Initializes LDMA (Linked DMA) for audio data transfers.
 * Allocates two DMA channels (left and right) and configures five-buffer
 * linear rings. Sets up transfers from USART0 RX (I2S data) to the buffer
 * ring for each channel. Attaches DMA callbacks for buffer completion
 * notification. Must be called before I2S initialization.
 *
 * @param None
 * @return void
 */
void adc__ldma_init(void) {
  CMU_ClockEnable(cmuClock_LDMA, true);
  Ecode_t DMADRV_return_status;

  DMADRV_return_status = DMADRV_Init();
  if ((DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) &&
      (DMADRV_return_status != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED)) {
    adc__printf(true, "DMADRV init failed: %X\n",
                (unsigned int)DMADRV_return_status);
    assert(0);
  }

  DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_LEFT, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) {
    adc__printf(true, "Allocate Error: %X\n",
                (unsigned int)DMADRV_return_status);
    assert(0);
  }
  adc__printf(true, "Got RX Left DMA Channel: %u\n",
              (unsigned int)LDMA_CHANNEL_LEFT);

  DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_RIGHT, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) {
    adc__printf(true, "Allocate Error: %X\n",
                (unsigned int)DMADRV_return_status);
    assert(0);
  }
  adc__printf(true, "Got RX Right DMA Channel: %u\n",
              (unsigned int)LDMA_CHANNEL_RIGHT);

  left_dma_buffer_index = 0;
  right_dma_buffer_index = 0;

  if (adc__start_left_dma_transfer() == false) {
    assert(0);
  }
  adc__printf(true, "Started LDMA_CHANNEL_RX_LEFT\n");

  if (adc__start_right_dma_transfer() == false) {
    assert(0);
  }
  adc__printf(true, "Started LDMA_CHANNEL_RX_RIGHT\n");
}








// #include <assert.h>
// #include <stdio.h>

// #include "ADC.h"

// #include "dmadrv.h"
// #include "em_cmu.h"
// #include "em_gpio.h"
// #include "em_ldma.h"
// #include "em_usart.h"

// #include <string.h>

// static void adc__init_i2s(void);
// static void adc__ldma_init(void);
// static void adc__wait_for_startup_validation(void);
// static void adc__print_sample_format_summary(void);
// static bool DMA_left_callback(unsigned int channel, unsigned int sequenceNo,
//                               void *userParam);
// static bool DMA_right_callback(unsigned int channel, unsigned int sequenceNo,
//                                void *userParam);

// typedef struct adc_dma_buffer_s {
//   volatile uint8_t data[ADC_BUFFER_SIZE] __attribute__((aligned(4)));
//   bool used;
//   bool processed;
//   uint32_t micros_timestamp;
// } adc_dma_buffer_t;

// static bool adc__start_left_dma_transfer(void);
// static bool adc__start_right_dma_transfer(void);

// typedef enum adc_counter_index_s {
//   ADC_COUNTER_SAMPLES_RECEIVED_LEFT = 0,
//   ADC_COUNTER_SAMPLES_RECEIVED_RIGHT = 1,
//   ADC_COUNTER_BYTES_RECEIVED_LEFT = 2,
//   ADC_COUNTER_BYTES_RECEIVED_RIGHT = 3,
//   ADC_COUNTER_LEFT_BUFFERS_IN_USE = 4,
//   ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN = 5,
//   ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX = 6,
//   ADC_COUNTER_RIGHT_BUFFERS_IN_USE = 7,
//   ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN = 8,
//   ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX = 9,
//   ADC_NUMBER_OF_COUNTERS
// } adc_counter_index_t;

// static volatile uint32_t adc_counter_values[ADC_NUMBER_OF_COUNTERS] = {0};
// static const char *adc_counter_names[ADC_NUMBER_OF_COUNTERS] = {
//     "adc_samples_received_left",    "adc_samples_received_right",
//     "adc_bytes_received_left",      "adc_bytes_received_right",
//     "adc_left_buffers_in_use",      "adc_left_buffers_in_use_min",
//     "adc_left_buffers_in_use_max",  "adc_right_buffers_in_use",
//     "adc_right_buffers_in_use_min", "adc_right_buffers_in_use_max",
// };

// static bool library_initialized = false;

// LDMA_Descriptor_t leftDesc[ADC_DMA_BUFFER_COUNT];
// LDMA_Descriptor_t rightDesc[ADC_DMA_BUFFER_COUNT];
// LDMA_TransferCfg_t leftCfg;
// LDMA_TransferCfg_t rightCfg;

// static volatile uint8_t left_data[ADC_DMA_BUFFER_COUNT][ADC_BUFFER_SIZE] __attribute__((aligned(4)));
// static volatile uint8_t right_data[ADC_DMA_BUFFER_COUNT][ADC_BUFFER_SIZE] __attribute__((aligned(4)));
// static volatile uint32_t left_DMA_CH = 0;
// static volatile uint32_t right_DMA_CH = 0;


// static adc_dma_buffer_t leftBuffers[ADC_DMA_BUFFER_COUNT];
// static adc_dma_buffer_t rightBuffers[ADC_DMA_BUFFER_COUNT];
// static volatile uint8_t leftDmaPingPongBuffers[2][ADC_BUFFER_SIZE]
//     __attribute__((aligned(4)));
// static volatile uint8_t rightDmaPingPongBuffers[2][ADC_BUFFER_SIZE]
//     __attribute__((aligned(4)));

// static volatile uint32_t left_dma_buffer_index = 0;
// static volatile uint32_t right_dma_buffer_index = 0;

// volatile bool new_data_ready_flag = false;
// volatile uint32_t *new_data_pointer;

// volatile uint8_t *leftBuffer_to_process;
// volatile uint8_t *rightBuffer_to_process;

// volatile uint32_t (*leftBuffer_output)[ADC_BUFFER_SIZE];
// volatile uint32_t (*rightBuffer_output)[ADC_BUFFER_SIZE];

// volatile uint8_t leftBuffer_converted[ADC_BUFFER_SIZE]
//     __attribute__((aligned(4)));
// ;
// volatile uint8_t rightBuffer_converted[ADC_BUFFER_SIZE]
//     __attribute__((aligned(4)));
// ;

// volatile uint32_t leftBuffer_converted_word[ADC_BUFFER_SIZE / 4];
// volatile uint32_t rightBuffer_converted_word[ADC_BUFFER_SIZE / 4];

// volatile uint8_t leftBuffer_test[ADC_BUFFER_SIZE] __attribute__((aligned(4)));
// ;
// volatile uint8_t rightBuffer_test[ADC_BUFFER_SIZE] __attribute__((aligned(4)));
// ;

// unsigned int LDMA_CHANNEL_LEFT;
// unsigned int LDMA_CHANNEL_RIGHT;

// extern uint32_t start;
// extern volatile uint32_t debug_signals[10];

// static bool stereo_flag = false;

// static void adc__update_left_buffer_counters(void);
// static void adc__update_right_buffer_counters(void);
// static void
// adc__store_completed_left_buffer(const volatile uint8_t *source_buffer);
// static void
// adc__store_completed_right_buffer(const volatile uint8_t *source_buffer);

// static volatile bool startup_left_buffer_ready = false;
// static volatile bool startup_right_buffer_ready = false;

// static const uint32_t adc_sample_size_bytes = 4;
// static const bool adc_sample_is_little_endian = true;
// static const bool adc_sample_is_left_justified = true;

// bool new_packet_ready_left_flag = false;
// bool new_packet_ready_right_flag = false;

// uint8_t *new_packet_left_pointer = 0;
// uint8_t *new_packet_right_pointer = 0;

// __attribute__((weak)) uint32_t adc__get_microsecond_ticks(void) {
//   return DWT->CYCCNT;
// }

// bool adc__handle_left_interrupt(void) {
//   return true;
// }

// bool adc__handle_right_interrupt(void) {
//   return true;
// }

// __attribute__((weak)) void adc__printf(bool add_timestamp, const char *format,
//                                        ...) {
//   (void)add_timestamp;
//   (void)format;
//   return;
// }

// /**
//  * @brief Gets the next audio packet ready for processing from the right
//  * channel. Returns pointer to the oldest right channel buffer if available.
//  * Clears the ready flag after returning data.
//  *
//  * @param data_pointer - Output pointer to audio data (pointer to pointer)
//  * @return true if packet available, false if no data ready
//  */
// bool adc__get_new_packet_ready_for_processing(uint8_t **data_pointer) {
//   if (new_packet_ready_right_flag == true) {
//     if (new_packet_right_pointer != 0) {
//       new_packet_ready_right_flag = false;
//       *data_pointer = new_packet_right_pointer;
//       return true;
//     }
//   }
//   return false;
// }

// /**
//  * @brief Gets the audio stereo flag state.
//  * Indicates whether the ADC is configured for stereo (true) or mono (false)
//  * mode.
//  *
//  * @param None
//  * @return true if stereo mode enabled, false if mono
//  */
// bool adc__get_audio_stereo_flag(void) { return stereo_flag; }

// uint32_t adc__get_sample_size_bytes(void) { return adc_sample_size_bytes; }

// bool adc__is_sample_little_endian(void) { return adc_sample_is_little_endian; }

// bool adc__is_sample_left_justified(void) {
//   return adc_sample_is_left_justified;
// }

// uint32_t adc__get_number_of_counters(void) { return ADC_NUMBER_OF_COUNTERS; }

// const char *adc__get_counter_name(uint32_t counter_index) {
//   if (counter_index >= ADC_NUMBER_OF_COUNTERS) {
//     return NULL;
//   }

//   return adc_counter_names[counter_index];
// }

// volatile uint32_t *adc__get_counter_address(uint32_t counter_index) {
//   if (counter_index >= ADC_NUMBER_OF_COUNTERS) {
//     return NULL; // Invalid index
//   }

//   return (volatile uint32_t *)&adc_counter_values[counter_index];
// }

// void adc__reset_counters(void) {
//   adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_LEFT] = 0;
//   adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_RIGHT] = 0;
//   adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_LEFT] = 0;
//   adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_RIGHT] = 0;
//   adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE] = 0;
//   adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN] = 0;
//   adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX] = 0;
//   adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE] = 0;
//   adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN] = 0;
//   adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX] = 0;
// }

// static void adc__update_left_buffer_counters(void) {
//   uint32_t current_count = adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE];

//   if (current_count < adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN]) {
//     adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN] = current_count;
//   }

//   if (current_count > adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX]) {
//     adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX] = current_count;
//   }
// }

// static void adc__update_right_buffer_counters(void) {
//   uint32_t current_count = adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE];

//   if (current_count <
//       adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN]) {
//     adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN] = current_count;
//   }

//   if (current_count >
//       adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX]) {
//     adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX] = current_count;
//   }
// }

// bool adc__get_oldest_left_dma_buffer(uint8_t **buffer, uint32_t *buffer_index,
//                                      uint32_t *buffer_size) {
//   uint32_t oldest_index_timestamp = 0xFFFFFFFF;
//   bool found = false;

//   if ((buffer == NULL) || (buffer_index == NULL) || (buffer_size == NULL)) {
//     return false;
//   }

//   for (uint32_t i = 0; i < ADC_DMA_BUFFER_COUNT; i++) {
//     if ((leftBuffers[i].used == true) && (leftBuffers[i].processed == false)) {
//       if (leftBuffers[i].micros_timestamp < oldest_index_timestamp) {
//         oldest_index_timestamp = leftBuffers[i].micros_timestamp;
//         *buffer_index = i;
//         found = true;
//       }
//     }
//   }

//   if (found == true) {
//     *buffer = (uint8_t *)leftBuffers[*buffer_index].data;
//     *buffer_size = sizeof(leftBuffers[*buffer_index].data);
//   }

//   return found;
// }

// bool adc__get_oldest_right_dma_buffer(uint8_t **buffer, uint32_t *buffer_index,
//                                       uint32_t *buffer_size) {
//   uint32_t oldest_index_timestamp = 0xFFFFFFFF;
//   bool found = false;

//   if ((buffer == NULL) || (buffer_index == NULL) || (buffer_size == NULL)) {
//     return false;
//   }

//   for (uint32_t i = 0; i < ADC_DMA_BUFFER_COUNT; i++) {
//     if ((rightBuffers[i].used == true) &&
//         (rightBuffers[i].processed == false)) {
//       if (rightBuffers[i].micros_timestamp < oldest_index_timestamp) {
//         oldest_index_timestamp = rightBuffers[i].micros_timestamp;
//         *buffer_index = i;
//         found = true;
//       }
//     }
//   }

//   if (found == true) {
//     *buffer = (uint8_t *)rightBuffers[*buffer_index].data;
//     *buffer_size = sizeof(rightBuffers[*buffer_index].data);
//   }

//   return found;
// }

// void adc__mark_left_dma_buffer_stale(uint32_t buffer_index) {
//   if (buffer_index < ADC_DMA_BUFFER_COUNT) {
//     if ((leftBuffers[buffer_index].used == true) &&
//         (leftBuffers[buffer_index].processed == false) &&
//         (adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE] > 0)) {
//       adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE]--;
//       adc__update_left_buffer_counters();
//     }
//     leftBuffers[buffer_index].processed = true;
//   }
// }

// void adc__mark_right_dma_buffer_stale(uint32_t buffer_index) {
//   if (buffer_index < ADC_DMA_BUFFER_COUNT) {
//     if ((rightBuffers[buffer_index].used == true) &&
//         (rightBuffers[buffer_index].processed == false) &&
//         (adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE] > 0)) {
//       adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE]--;
//       adc__update_right_buffer_counters();
//     }
//     rightBuffers[buffer_index].processed = true;
//   }
// }

// /**
//  * @brief Initializes the ADC subsystem.
//  * Configures LDMA for DMA transfers and initializes I2S interface for audio
//  * input. Must be called once at startup before audio samples are expected.
//  *
//  * @param None
//  * @return void
//  */
// void adc__init(bool is_stereo) {
//   if (library_initialized == true) {
//     adc__printf(true, "ADC Library Already Initialized. De-Initializing\n");
//     adc__deinit();
//   }

//   stereo_flag = is_stereo;
//   adc__printf(true, "ADC Init Stereo Flag: %u\n", (unsigned int)stereo_flag);
//   adc__reset_counters();
//   memset((void *)leftBuffers, 0, sizeof(leftBuffers));
//   memset((void *)rightBuffers, 0, sizeof(rightBuffers));
//   leftBuffer_to_process = (volatile uint8_t *)leftDmaPingPongBuffers[0];
//   rightBuffer_to_process = (volatile uint8_t *)rightDmaPingPongBuffers[0];
//   startup_left_buffer_ready = false;
//   startup_right_buffer_ready = false;
//   adc__ldma_init();
//   adc__init_i2s();
//   USART_Enable(USART0, usartEnable);
//   adc__wait_for_startup_validation();

//   library_initialized = true;
//   adc__printf(true, "ADC Init Complete\n");
// }

// /**
//  * @brief De-initializes the ADC subsystem.
//  * Stops USART0 (I2S interface), frees DMA channels, and cleans up resources.
//  * Handles both allocated and already-freed DMA channels gracefully.
//  * Called during shutdown or state transitions.
//  *
//  * @param None
//  * @return void
//  */
// void adc__deinit(void) {
//   adc__printf(true, "DEINITIALIZING ADC\n");

//   if (CMU->CLKEN0 & CMU_CLKEN0_USART0) {
//     adc__printf(true, "RESETTING USART\n");
//     USART_Reset(USART0);
//     adc__printf(true, "RESET USART\n");
//   }

//   // printf_to_buf_append_time(0,"STOPPING LDMA LEFT\n");
//   // LDMA_StopTransfer(LDMA_CHANNEL_LEFT);
//   // printf_to_buf_append_time(0,"STOPPED LDMA LEFT\n");

//   // printf_to_buf_append_time(0,"STOPPING LDMA RIGHT\n");
//   // LDMA_StopTransfer(LDMA_CHANNEL_RIGHT);
//   // printf_to_buf_append_time(0,"STOPPED LDMA RIGHT\n");

//   if (!((LDMA_CHANNEL_LEFT == 0) && (LDMA_CHANNEL_RIGHT == 0))) {
//     Ecode_t DMADRV_return_status;

//     DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_LEFT);
//     DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_LEFT);
//     if (DMADRV_return_status == ECODE_EMDRV_DMADRV_OK) {
//       adc__printf(true, "DEINITIALIZED LEFT DMA CHANNEL: %u\n",
//                   (unsigned int)LDMA_CHANNEL_LEFT);
//     } else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_ALREADY_FREED) {
//       adc__printf(true, "LEFT DMA CHANNEL: %u already freed\n",
//                   (unsigned int)LDMA_CHANNEL_LEFT);
//     } else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_NOT_INITIALIZED) {
//       assert(0);
//     }

//     DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_RIGHT);
//     DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_RIGHT);
//     if (DMADRV_return_status == ECODE_EMDRV_DMADRV_OK) {
//       adc__printf(true, "DEINITIALIZED RIGHT DMA CHANNEL: %u\n",
//                   (unsigned int)LDMA_CHANNEL_RIGHT);
//     } else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_ALREADY_FREED) {
//       adc__printf(true, "RIGHT DMA CHANNEL: %u ALREADY FREED\n",
//                   (unsigned int)LDMA_CHANNEL_RIGHT);
//     } else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_NOT_INITIALIZED) {
//       assert(0);
//     }
//   }
// }

// bool adc__get_new_data_ready_flag(uint32_t **new_data_pointer_return) {
//   if (new_data_ready_flag == true) {
//     new_data_ready_flag = false;
//     *new_data_pointer_return = (uint32_t *)new_data_pointer;
//     return true;
//   }
//   return false;
// }

// static void adc__wait_for_startup_validation(void) {
//   uint32_t start_cycles = DWT->CYCCNT;
//   uint32_t timeout_cycles = CMU_ClockFreqGet(cmuClock_CORE);
//   bool left_ready_logged = false;
//   bool right_ready_logged = false;

//   adc__printf(true, "Waiting for ADC startup buffers\n");

//   while ((startup_left_buffer_ready == false) ||
//          (startup_right_buffer_ready == false)) {
//     if ((startup_left_buffer_ready == true) && (left_ready_logged == false)) {
//       left_ready_logged = true;
//       adc__printf(true, "ADC startup left buffer ready\n");
//     }

//     if ((startup_right_buffer_ready == true) && (right_ready_logged == false)) {
//       right_ready_logged = true;
//       adc__printf(true, "ADC startup right buffer ready\n");
//     }

//     if ((uint32_t)(DWT->CYCCNT - start_cycles) > timeout_cycles) {
//       assert(0);
//       // printf("ADC startup buffers validation timeout\n");
//       // custom_assert(0, __FILE__, __LINE__);
//     }
//   }

//   adc__print_sample_format_summary();

//   adc__printf(true, "ADC Left buffer preview:\n");
//   adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
//               (unsigned int)leftDmaPingPongBuffers[0][0],
//               (unsigned int)leftDmaPingPongBuffers[0][1],
//               (unsigned int)leftDmaPingPongBuffers[0][2],
//               (unsigned int)leftDmaPingPongBuffers[0][3]);
//   adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
//               (unsigned int)leftDmaPingPongBuffers[0][4],
//               (unsigned int)leftDmaPingPongBuffers[0][5],
//               (unsigned int)leftDmaPingPongBuffers[0][6],
//               (unsigned int)leftDmaPingPongBuffers[0][7]);
//   adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
//               (unsigned int)leftDmaPingPongBuffers[0][8],
//               (unsigned int)leftDmaPingPongBuffers[0][9],
//               (unsigned int)leftDmaPingPongBuffers[0][10],
//               (unsigned int)leftDmaPingPongBuffers[0][11]);

//   adc__printf(true, "ADC Right buffer preview:\n");
//   adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
//               (unsigned int)rightDmaPingPongBuffers[0][0],
//               (unsigned int)rightDmaPingPongBuffers[0][1],
//               (unsigned int)rightDmaPingPongBuffers[0][2],
//               (unsigned int)rightDmaPingPongBuffers[0][3]);
//   adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
//               (unsigned int)rightDmaPingPongBuffers[0][4],
//               (unsigned int)rightDmaPingPongBuffers[0][5],
//               (unsigned int)rightDmaPingPongBuffers[0][6],
//               (unsigned int)rightDmaPingPongBuffers[0][7]);
//   adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
//               (unsigned int)rightDmaPingPongBuffers[0][8],
//               (unsigned int)rightDmaPingPongBuffers[0][9],
//               (unsigned int)rightDmaPingPongBuffers[0][10],
//               (unsigned int)rightDmaPingPongBuffers[0][11]);

//   // adc__print_startup_buffer_preview("left", leftBuffer_to_process);
//   // adc__print_startup_buffer_preview("right", rightBuffer_to_process);

//   adc__printf(true, "ADC startup buffers validated\n");
// }

// static void adc__print_sample_format_summary(void) {
//   adc__printf(true, "ADC sample size: %u bytes\n",
//               (unsigned int)adc__get_sample_size_bytes());
//   adc__printf(true, "ADC sample appears to be %s-endian\n",
//               adc__is_sample_little_endian() ? "little" : "big");
//   adc__printf(true, "ADC sample appears to be %s-justified\n",
//               adc__is_sample_left_justified() ? "left" : "right");
// }

// static bool adc__start_left_dma_transfer(void) {
//   Ecode_t status = DMADRV_PeripheralMemoryPingPong(
//       LDMA_CHANNEL_LEFT, dmadrvPeripheralSignal_USART0_RXDATAV,
//       (void *)leftDmaPingPongBuffers[0], (void *)leftDmaPingPongBuffers[1],
//       (void *)&(USART0->RXDATA), true, ADC_BUFFER_SIZE, dmadrvDataSize1,
//       DMA_left_callback, NULL);

//   if (status != ECODE_EMDRV_DMADRV_OK) {
//     adc__printf(true, "LDMA left start error: %X\n", (unsigned int)status);
//     return false;
//   }

//   return true;
// }

// static bool adc__start_right_dma_transfer(void) {
//   Ecode_t status = DMADRV_PeripheralMemoryPingPong(
//       LDMA_CHANNEL_RIGHT, dmadrvPeripheralSignal_USART0_RXDATAVRIGHT,
//       (void *)rightDmaPingPongBuffers[0], (void *)rightDmaPingPongBuffers[1],
//       (void *)&(USART0->RXDATA), true, ADC_BUFFER_SIZE, dmadrvDataSize1,
//       DMA_right_callback, NULL);

//   if (status != ECODE_EMDRV_DMADRV_OK) {
//     adc__printf(true, "LDMA right start error: %X\n", (unsigned int)status);
//     return false;
//   }

//   return true;
// }

// /**
//  * @brief DMA callback for left channel audio transfer completion.
//  * Called when LDMA finishes transferring a left audio buffer.
//  * Advances the five-buffer linear ring and re-arms the next transfer.
//  *
//  * @param channel - LDMA channel identifier (unused)
//  * @param sequenceNo - Sequence number indicating which buffer completed
//  * @param userParam - User parameter (unused)
//  * @return true always
//  */
// static bool DMA_left_callback(unsigned int channel, unsigned int sequenceNo,
//                               void *userParam) {
//   (void)channel;
//   (void)userParam;

//   // printf_to_buf_append_time(0,"Left ADC seq: %X\n",(unsigned int)sequenceNo);

//   // adc__printf(true, "ADC left buffer finished: %u\n", (unsigned
//   // int)left_dma_buffer_index);

//   startup_left_buffer_ready = true;
//   adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_LEFT] +=
//       ADC_BUFFER_SIZE >> 2; // divide by sample size
//   adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_LEFT] += ADC_BUFFER_SIZE;

//   if (stereo_flag == false) {
//     return true;
//   }

//   adc__store_completed_left_buffer(leftDmaPingPongBuffers[sequenceNo & 0x1]);

//   return true;
// }

// /**
//  * @brief DMA callback for right channel audio transfer completion.
//  * Called when LDMA finishes transferring a right audio buffer.
//  * Advances the five-buffer linear ring and re-arms the next transfer.
//  * Called from interrupt context, so very timing-critical.
//  *
//  * @param channel - LDMA channel identifier (unused)
//  * @param sequenceNo - Sequence number indicating which buffer completed
//  * @param userParam - User parameter (unused)
//  * @return true always
//  */
// static bool DMA_right_callback(unsigned int channel, unsigned int sequenceNo,
//                                void *userParam) {
//   (void)channel;
//   (void)userParam;

//   //  printf_to_buf_append_time(0,"Right ADC seq: %X\n",(unsigned
//   //  int)sequenceNo);

//   // adc__printf(true, "ADC right buffer finished: %u\n", (unsigned
//   // int)right_dma_buffer_index);

//   startup_right_buffer_ready = true;
//   adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_RIGHT] +=
//       ADC_BUFFER_SIZE >> 2; // divide by sample size
//   adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_RIGHT] += ADC_BUFFER_SIZE;

//   adc__store_completed_right_buffer(rightDmaPingPongBuffers[sequenceNo & 0x1]);

//   return true;
// }

// static void
// adc__store_completed_left_buffer(const volatile uint8_t *source_buffer) {
//   if ((leftBuffers[left_dma_buffer_index].used == true) &&
//       (leftBuffers[left_dma_buffer_index].processed == false)) {
//     adc__printf(true,
//                 "ADC left DMA buffer %u not stale yet, overwriting oldest "
//                 "unread data\n",
//                 (unsigned int)left_dma_buffer_index);
//   }

//   memcpy((void *)leftBuffers[left_dma_buffer_index].data,
//          (const void *)source_buffer, ADC_BUFFER_SIZE);

//   leftBuffers[left_dma_buffer_index].used = true;
//   leftBuffers[left_dma_buffer_index].processed = false;
//   leftBuffers[left_dma_buffer_index].micros_timestamp =
//       adc__get_microsecond_ticks();
//   adc_counter_values[ADC_COUNTER_LEFT_BUFFERS_IN_USE]++;
//   adc__update_left_buffer_counters();

//   leftBuffer_to_process =
//       (volatile uint8_t *)leftBuffers[left_dma_buffer_index].data;
//   new_data_pointer = (volatile uint32_t *)leftBuffer_to_process;
//   new_data_ready_flag = true;

//   left_dma_buffer_index++;
//   if (left_dma_buffer_index >= ADC_DMA_BUFFER_COUNT) {
//     left_dma_buffer_index = 0;
//   }
// }

// static void
// adc__store_completed_right_buffer(const volatile uint8_t *source_buffer) {
//   (void)source_buffer;

//   if ((rightBuffers[right_dma_buffer_index].used == true) &&
//       (rightBuffers[right_dma_buffer_index].processed == false)) {
//     adc__printf(true,
//                 "ADC right DMA buffer %u not stale yet, overwriting oldest "
//                 "unread data\n",
//                 (unsigned int)right_dma_buffer_index);
//   }

//   #define TEST_PIPELINE_8 0
//   #define TEST_PIPELINE_8_ADD_PADDING 0
//   #define TEST_PIPELINE_16 0
//   #define TEST_PIPELINE_16_ADD_PADDING 0
//   #define TEST_PIPELINE_32 0

//   const uint32_t sample_count = ADC_BUFFER_SIZE / sizeof(uint32_t);
  
//   #if (TEST_PIPELINE_8 == 1)
//   static uint32_t test_counter = 0;
//   if (TEST_PIPELINE_8_ADD_PADDING == 1) {
//     volatile uint32_t *sample_buffer =
//         (volatile uint32_t *)rightBuffers[right_dma_buffer_index].data;
//     for (uint32_t i = 0; i < sample_count; i++) {
//       sample_buffer[i] = test_counter & 0xFF;
//       test_counter++;
//       if (test_counter > 0xFF) {
//         test_counter = 0;
//       }
//     }
//   } else {
//     for (uint32_t i = 0; i < ADC_BUFFER_SIZE; i++) {
//       rightBuffers[right_dma_buffer_index].data[i] = test_counter & 0xFF;
//       test_counter++;
//       if (test_counter > 0xFF) {
//         test_counter = 0;
//       }
//     }
//   }
//   #elif (TEST_PIPELINE_16 == 1)
//   static uint32_t test_counter = 0;
//   if (TEST_PIPELINE_16_ADD_PADDING == 1) {
//     volatile uint32_t *sample_buffer =
//         (volatile uint32_t *)rightBuffers[right_dma_buffer_index].data;
//     for (uint32_t i = 0; i < sample_count; i++) {
//       sample_buffer[i] = test_counter & 0xFFFF;
//       test_counter++;
//       if (test_counter > 0xFFFF) {
//         test_counter = 0;
//       }
//     }
//   } else {
//     volatile uint16_t *sample_buffer =
//         (volatile uint16_t *)rightBuffers[right_dma_buffer_index].data;
//     for (uint32_t i = 0; i < (ADC_BUFFER_SIZE / sizeof(uint16_t)); i++) {
//       sample_buffer[i] = (uint16_t)test_counter;
//       test_counter++;
//       if (test_counter > 0xFFFF) {
//         test_counter = 0;
//       }
//     }
//   }
//   #elif (TEST_PIPELINE_32 == 1)
//   static uint32_t test_counter = 0;
//   volatile uint32_t *sample_buffer =
//       (volatile uint32_t *)rightBuffers[right_dma_buffer_index].data;
//   for (uint32_t i = 0; i < (ADC_BUFFER_SIZE / sizeof(uint32_t)); i++) {
//     sample_buffer[i] = test_counter;
//     test_counter++;
//   }
//   #else
//   memcpy((void *)rightBuffers[right_dma_buffer_index].data,
//          (const void *)source_buffer, ADC_BUFFER_SIZE);
//   #endif

//   rightBuffers[right_dma_buffer_index].used = true;
//   rightBuffers[right_dma_buffer_index].processed = false;
//   rightBuffers[right_dma_buffer_index].micros_timestamp =
//       adc__get_microsecond_ticks();
//   adc_counter_values[ADC_COUNTER_RIGHT_BUFFERS_IN_USE]++;
//   adc__update_right_buffer_counters();

//   rightBuffer_to_process =
//       (volatile uint8_t *)rightBuffers[right_dma_buffer_index].data;
//   new_data_pointer = (volatile uint32_t *)rightBuffer_to_process;
//   new_data_ready_flag = true;

//   right_dma_buffer_index++;
//   if (right_dma_buffer_index >= ADC_DMA_BUFFER_COUNT) {
//     right_dma_buffer_index = 0;
//   }
// }

// /**
//  * @brief USART0 RX interrupt handler for I2S overflow/error detection.
//  * Reads interrupt flags, detects overflow conditions on receive.
//  * Sets GPIO debug pin and clears interrupt flags.
//  * Called from interrupt context.
//  *
//  * @param None
//  * @return void
//  */
// void USART0_RX_IRQHandler(void) {
//   // debug_signals[0] = USART_IntGet(USART0);
//   GPIO_PinModeSet(gpioPortB, 0, gpioModePushPull, 0);
//   USART_IntClear(USART0, USART_IntGet(USART0));
// }

// // void LDMA_IRQHandler(void) {
// //   LDMA_IntClear(LDMA->IF);
// //   adc__printf(true, "LDMA\n");
// // }

// /**
//  * @brief Initializes I2S (Integrated Inter-IC Sound) interface on USART0.
//  * Configures GPIO pins for I2S clock, frame sync, data in/out.
//  * Sets up I2S in slave mode with 32-bit words, left-justified format.
//  * Configures split DMA for left/right channels and enables RX interrupt.
//  * Must be called after LDMA is initialized.
//  *
//  * @param None
//  * @return void
//  */
// void adc__init_i2s(void) {
//   CMU_ClockEnable(cmuClock_GPIO, true);
//   CMU_ClockEnable(cmuClock_USART0, true);

//   GPIO_PinModeSet(I2S_CLK_PORT, I2S_CLK_PIN, gpioModeInputPullFilter, 0);
//   GPIO_PinModeSet(I2S_CS_PORT, I2S_CS_PIN, gpioModeInputPullFilter, 0);

//   GPIO_PinModeSet(I2S_DATAIN_PORT, I2S_DATAIN_PIN, gpioModeInputPullFilter, 0);
//   GPIO_PinModeSet(I2S_DATAOUT_PORT, I2S_DATAOUT_PIN, gpioModePushPull, 0);

//   USART_InitI2s_TypeDef init = USART_INITI2S_DEFAULT;

//   init.sync.master = false;
//   init.sync.enable = usartDisable;
//   init.sync.databits = usartDatabits8;
//   init.sync.baudrate = BAUD_RATE;
//   // init.sync.autoTx = true;
//   init.sync.clockMode = usartClockMode3;

//   init.dmaSplit = true;
//   init.mono = false;
//   init.format = USART_I2SCTRL_FORMAT_W32D32;
//   init.justify = usartI2sJustifyLeft;
//   init.delay = false;
//   // init.sync.clockMode = false;

//   GPIO->USARTROUTE[0].CSROUTE =
//       (I2S_CS_PORT << _GPIO_USART_CSROUTE_PORT_SHIFT) |
//       (I2S_CS_PIN << _GPIO_USART_CSROUTE_PIN_SHIFT);
//   GPIO->USARTROUTE[0].CLKROUTE =
//       (I2S_CLK_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT) |
//       (I2S_CLK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);

// #if (I2S_MODE == I2S_MODE_MASTER)
//   GPIO->USARTROUTE[0].TXROUTE =
//       (I2S_DATAOUT_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) |
//       (I2S_DATAOUT_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
//   GPIO->USARTROUTE[0].RXROUTE =
//       (I2S_DATAIN_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) |
//       (I2S_DATAIN_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
//   GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN |
//                                 GPIO_USART_ROUTEEN_CSPEN |
//                                 GPIO_USART_ROUTEEN_CLKPEN;
// #elif (I2S_MODE == I2S_MODE_SLAVE)
//   GPIO->USARTROUTE[0].TXROUTE =
//       (I2S_DATAIN_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) |
//       (I2S_DATAIN_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
//   // GPIO->USARTROUTE[0].RXROUTE = (I2S_DATAOUT_PORT <<
//   // _GPIO_USART_RXROUTE_PORT_SHIFT)| (I2S_DATAOUT_PIN <<
//   // _GPIO_USART_RXROUTE_PIN_SHIFT);
//   GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN |
//                                 GPIO_USART_ROUTEEN_CSPEN |
//                                 GPIO_USART_ROUTEEN_CLKPEN;
// #endif

//   // Enable USART interface pins

//   USART_InitI2s(USART0, &init);

//   USART_IntEnable(USART0, USART_IEN_RXOF | USART_IEN_RXFULL);

//   // NVIC_ClearPendingIRQ(USART0_RX_IRQn);
//   // NVIC_ClearPendingIRQ(USART0_TX_IRQn);
//   NVIC_EnableIRQ(USART0_RX_IRQn);
//   // NVIC_EnableIRQ(USART0_TX_IRQn);
// }

// /**
//  * @brief Initializes LDMA (Linked DMA) for audio data transfers.
//  * Allocates two DMA channels (left and right) and configures five-buffer
//  * linear rings. Sets up transfers from USART0 RX (I2S data) to the buffer
//  * ring for each channel. Attaches DMA callbacks for buffer completion
//  * notification. Must be called before I2S initialization.
//  *
//  * @param None
//  * @return void
//  */
// void adc__ldma_init(void) {
//   CMU_ClockEnable(cmuClock_LDMA, true);
//   Ecode_t DMADRV_return_status;

//   DMADRV_return_status = DMADRV_Init();
//   if ((DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) &&
//       (DMADRV_return_status != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED)) {
//     adc__printf(true, "DMADRV init failed: %X\n",
//                 (unsigned int)DMADRV_return_status);
//     assert(0);
//   }

//   DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_LEFT, NULL);
//   if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) {
//     adc__printf(true, "Allocate Error: %X\n",
//                 (unsigned int)DMADRV_return_status);
//     assert(0);
//   }
//   adc__printf(true, "Got RX Left DMA Channel: %u\n",
//               (unsigned int)LDMA_CHANNEL_LEFT);

//   DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_RIGHT, NULL);
//   if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) {
//     adc__printf(true, "Allocate Error: %X\n",
//                 (unsigned int)DMADRV_return_status);
//     assert(0);
//   }
//   adc__printf(true, "Got RX Right DMA Channel: %u\n",
//               (unsigned int)LDMA_CHANNEL_RIGHT);

//   left_dma_buffer_index = 0;
//   right_dma_buffer_index = 0;

//   if (adc__start_left_dma_transfer() == false) {
//     assert(0);
//   }
//   adc__printf(true, "Started LDMA_CHANNEL_RX_LEFT\n");

//   if (adc__start_right_dma_transfer() == false) {
//     assert(0);
//   }
//   adc__printf(true, "Started LDMA_CHANNEL_RX_RIGHT\n");
// }


// // void adc__ldma_init(void)
// // {
// //   // Default LDMA init
// //   LDMA_Init_t init = LDMA_INIT_DEFAULT;
// //   LDMA_Init(&init);

// //   // Configure LDMA for transfer from USART to memory (left channel)
// //   // LDMA will loop continuously
// //   //LDMA_TransferCfg_t

// //   //Globally store and configure link descriptors for left microphone transfer

// //   for (int i = 0; i < ADC_DMA_BUFFER_COUNT; i++) {
// //     LDMA_Descriptor_t left_temp = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, (uint8_t *)left_data[i], ADC_BUFFER_SIZE, 1);
// //     leftDesc[i] = left_temp;

// //     leftDesc[i].xfer.dstInc = ldmaCtrlDstIncOne;
// //     leftDesc[i].xfer.blockSize = ldmaCtrlBlockSizeUnit1;
// //     leftDesc[i].xfer.doneIfs = 1;
// //     leftDesc[i].xfer.ignoreSrec = 0;
// //     leftDesc[i].xfer.size = ldmaCtrlSizeByte;


// //     LDMA_Descriptor_t right_temp = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, (uint8_t *)right_data[i], ADC_BUFFER_SIZE, 1);
// //     rightDesc[i] = right_temp;

// //     rightDesc[i].xfer.dstInc = ldmaCtrlDstIncOne;
// //     rightDesc[i].xfer.blockSize = ldmaCtrlBlockSizeUnit1;
// //     rightDesc[i].xfer.doneIfs = 1;
// //     rightDesc[i].xfer.ignoreSrec = 0;
// //     rightDesc[i].xfer.size = ldmaCtrlSizeByte;
// //   }
// //   leftDesc[ADC_DMA_BUFFER_COUNT - 1].xfer.link = 0; // Last descriptor links to 0 (end of chain)
// //   rightDesc[ADC_DMA_BUFFER_COUNT - 1].xfer.link = 0; // Last descriptor links to 0 (end of chain)

// //  leftCfg= (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_RXDATAV);
// //  rightCfg = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_RXDATAVRIGHT);

// //  uint32_t status;

// //  status = DMADRV_Init();
// //  if (status != ECODE_EMDRV_DMADRV_OK && status != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED) {
// //    adc__printf(true, "DMA Init Error: %X\n", (unsigned int)status);
// //    assert(0);
// //  }

// //  status = DMADRV_AllocateChannel(&left_DMA_CH, 0);
// //  if (status != ECODE_EMDRV_DMADRV_OK) {
// //    adc__printf(true, "Left DMA Allocate Error: %X\n", (unsigned int)status);
// //    assert(0);
// //  }
// //  status = DMADRV_AllocateChannel(&right_DMA_CH, 0);
// //  if (status != ECODE_EMDRV_DMADRV_OK) {
// //    adc__printf(true, "Right DMA Allocate Error: %X\n", (unsigned int)status);
// //    assert(0);
// //  }

// //  LDMA_IntClear((0xFFFFFFFF));
// //  LDMA_IntEnable((0x1<<left_DMA_CH)|(0x1<<right_DMA_CH));// | LDMA_IF_DONE3);
// //  NVIC_EnableIRQ(LDMA_IRQn);

// //  LDMA_StartTransfer(left_DMA_CH, (void *)&leftCfg, (void *)&leftDesc);
// //  LDMA_StartTransfer(right_DMA_CH, (void *)&rightCfg, (void *)&rightDesc);



// // //  DMADRV_Init();
// // //  uint32_t status;
// // //  unsigned int dmaChannelLeft;
// // //  status = DMADRV_AllocateChannel(&dmaChannelLeft, 0);
// // //  DMADRV_MemoryPeripheral(dmaChannelLeft, dmadrvPeripheralSignal_USART0_TXBL, &USART0->TXDATA, leftBuffer, true, BUFFER_SIZE, dmadrvDataSize1, 0, 0);
// // //
// // //  unsigned int dmaChannelRight;
// // //  status = DMADRV_AllocateChannel(&dmaChannelRight, 0);
// // //  DMADRV_MemoryPeripheral(dmaChannelRight, dmadrvPeripheralSignal_USART0_TXBLRIGHT, &USART0->TXDATA, rightBuffer, true, BUFFER_SIZE, dmadrvDataSize1, 0, 0);


// // }

// // // /**
// // //  * @brief Initializes LDMA (Linked DMA) for audio data transfers.
// // //  * Allocates two DMA channels (left and right) and configures five-buffer
// // //  * linear rings. Sets up transfers from USART0 RX (I2S data) to the buffer
// // //  * ring for each channel. Attaches DMA callbacks for buffer completion
// // //  * notification. Must be called before I2S initialization.
// // //  *
// // //  * @param None
// // //  * @return void
// // //  */
// // // void adc__ldma_init(void) {
// // //   CMU_ClockEnable(cmuClock_LDMA, true);
  
  
  
  
  
  
  
  
  
  
  
// //   Ecode_t DMADRV_return_status;

// //   DMADRV_return_status = DMADRV_Init();
// //   if ((DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) &&
// //       (DMADRV_return_status != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED)) {
// //     adc__printf(true, "DMADRV init failed: %X\n",
// //                 (unsigned int)DMADRV_return_status);
// //     assert(0);
// //   }

// //   DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_LEFT, NULL);
// //   if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) {
// //     adc__printf(true, "Allocate Error: %X\n",
// //                 (unsigned int)DMADRV_return_status);
// //     assert(0);
// //   }
// //   adc__printf(true, "Got RX Left DMA Channel: %u\n",
// //               (unsigned int)LDMA_CHANNEL_LEFT);

// //   DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_RIGHT, NULL);
// //   if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) {
// //     adc__printf(true, "Allocate Error: %X\n",
// //                 (unsigned int)DMADRV_return_status);
// //     assert(0);
// //   }
// //   adc__printf(true, "Got RX Right DMA Channel: %u\n",
// //               (unsigned int)LDMA_CHANNEL_RIGHT);

// //   left_dma_buffer_index = 0;
// //   right_dma_buffer_index = 0;

// //   if (adc__start_left_dma_transfer() == false) {
// //     assert(0);
// //   }
// //   adc__printf(true, "Started LDMA_CHANNEL_RX_LEFT\n");

// //   if (adc__start_right_dma_transfer() == false) {
// //     assert(0);
// //   }
// //   adc__printf(true, "Started LDMA_CHANNEL_RX_RIGHT\n");
// // }