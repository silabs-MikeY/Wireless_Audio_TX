#include "ADC.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_usart.h"
#include "ldma_manager.h"

#include <string.h>

#define ADC_INVALID_LDMA_CHANNEL ((unsigned int)UINT32_MAX)

typedef enum adc_channel_s {
  ADC_CHANNEL_LEFT = 0,
  ADC_CHANNEL_RIGHT,
  ADC_CHANNEL_COUNT
} adc_channel_t;

typedef struct adc_dma_buffer_s {
  volatile uint8_t data[ADC_BUFFER_SIZE] __attribute__((aligned(4)));
  bool used;
  bool processed;
  uint32_t micros_timestamp;
} adc_dma_buffer_t;

typedef struct adc_channel_state_s {
  adc_dma_buffer_t buffers[ADC_DMA_BUFFER_COUNT];
  LDMA_Descriptor_t descriptors[ADC_DMA_BUFFER_COUNT];
  LDMA_TransferCfg_t transfer_config;
  unsigned int ldma_channel;
  volatile bool startup_buffer_ready;
} adc_channel_state_t;

// -----------------------------------------------------------------------------
//                     Exported Counters
// -----------------------------------------------------------------------------

typedef enum adc_counter_index_s {
  ADC_COUNTER_SAMPLES_RECEIVED_LEFT = 0,
  ADC_COUNTER_SAMPLES_RECEIVED_RIGHT,
  ADC_COUNTER_BYTES_RECEIVED_LEFT,
  ADC_COUNTER_BYTES_RECEIVED_RIGHT,
  ADC_COUNTER_LEFT_BUFFERS_IN_USE,
  ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN,
  ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX,
  ADC_COUNTER_RIGHT_BUFFERS_IN_USE,
  ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN,
  ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX,
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

uint32_t adc__get_number_of_counters(void) { return ADC_NUMBER_OF_COUNTERS; }

const char *adc__get_counter_name(uint32_t counter_index) {
  if (counter_index >= ADC_NUMBER_OF_COUNTERS) {
    return NULL;
  }

  return adc_counter_names[counter_index];
}

volatile uint32_t *adc__get_counter_address(uint32_t counter_index) {
  if (counter_index >= ADC_NUMBER_OF_COUNTERS) {
    return NULL;
  }

  return (volatile uint32_t *)&adc_counter_values[counter_index];
}

void adc__reset_counters(void) {
  memset((void *)adc_counter_values, 0, sizeof(adc_counter_values));
}

// -----------------------------------------------------------------------------
//                     Exported Counters End
// -----------------------------------------------------------------------------

static bool library_initialized = false;
static bool stereo_flag = false;

static adc_channel_state_t adc_channels[ADC_CHANNEL_COUNT] = {
    [ADC_CHANNEL_LEFT] = {.ldma_channel = ADC_INVALID_LDMA_CHANNEL},
    [ADC_CHANNEL_RIGHT] = {.ldma_channel = ADC_INVALID_LDMA_CHANNEL},
};

static const uint32_t adc_sample_size_bytes = 4;
static const bool adc_sample_is_little_endian = true;
static const bool adc_sample_is_left_justified = true;

// Legacy debug compatibility for VDAC.c. ADC no longer writes this buffer.
volatile uint32_t leftBuffer_converted_word[ADC_BUFFER_SIZE / 4];

static bool adc__init_i2s(void);
static bool adc__ldma_init(void);
static bool adc__start_dma_transfer(adc_channel_t channel);
static bool adc__wait_for_startup_validation(void);
static void adc__print_sample_format_summary(void);

static bool adc__left_dma_callback(unsigned int channel, unsigned int sequenceNo,
                                   void *userParam);
static bool adc__right_dma_callback(unsigned int channel,
                                    unsigned int sequenceNo, void *userParam);

static bool adc__dma_callback(adc_channel_t channel, unsigned int sequence_no);
static bool adc__get_oldest_dma_buffer(adc_channel_t channel, uint8_t **buffer,
                                       uint32_t *buffer_index,
                                       uint32_t *buffer_size);
static void adc__mark_dma_buffer_stale(adc_channel_t channel,
                                       uint32_t buffer_index);
static void adc__mark_completed_buffer(adc_channel_t channel,
                                       uint32_t buffer_index);
static void adc__update_buffer_counters(adc_channel_t channel);
static uint32_t adc__counter_samples_received(adc_channel_t channel);
static uint32_t adc__counter_bytes_received(adc_channel_t channel);
static uint32_t adc__counter_buffers_in_use(adc_channel_t channel);
static uint32_t adc__counter_buffers_in_use_min(adc_channel_t channel);
static uint32_t adc__counter_buffers_in_use_max(adc_channel_t channel);

__attribute__((weak)) uint32_t adc__get_microsecond_ticks(void) {
  return DWT->CYCCNT;
}

__attribute__((weak)) void adc__printf(bool add_timestamp, const char *format,
                                       ...) {
  (void)add_timestamp;
  (void)format;
}

// -----------------------------------------------------------------------------
//                     ADC General
// -----------------------------------------------------------------------------

bool adc__init(bool is_stereo, unsigned int ldma_channel_left,
               unsigned int ldma_channel_right) {
  if (library_initialized == true) {
    adc__printf(true, "ADC Library Already Initialized. De-Initializing\n");
    adc__deinit();
  }

  stereo_flag = is_stereo;
  adc_channels[ADC_CHANNEL_LEFT].ldma_channel = ldma_channel_left;
  adc_channels[ADC_CHANNEL_RIGHT].ldma_channel = ldma_channel_right;

  adc__printf(true, "ADC Init Stereo Flag: %u\n", (unsigned int)stereo_flag);

  adc__reset_counters();
  memset(adc_channels[ADC_CHANNEL_LEFT].buffers, 0,
         sizeof(adc_channels[ADC_CHANNEL_LEFT].buffers));
  memset(adc_channels[ADC_CHANNEL_RIGHT].buffers, 0,
         sizeof(adc_channels[ADC_CHANNEL_RIGHT].buffers));
  adc_channels[ADC_CHANNEL_LEFT].startup_buffer_ready = false;
  adc_channels[ADC_CHANNEL_RIGHT].startup_buffer_ready = false;

  if (!adc__ldma_init()) {
    return false;
  }

  if (!adc__init_i2s()) {
    adc__deinit();
    return false;
  }

  USART_Enable(USART0, usartEnable);

  if (!adc__wait_for_startup_validation()) {
    adc__deinit();
    return false;
  }

  library_initialized = true;
  adc__printf(true, "ADC Init Complete\n");
  return true;
}

void adc__deinit(void) {
  adc__printf(true, "DEINITIALIZING ADC\n");

  if (CMU->CLKEN0 & CMU_CLKEN0_USART0) {
    adc__printf(true, "RESETTING USART\n");
    USART_Reset(USART0);
    adc__printf(true, "RESET USART\n");
  }

  for (uint32_t channel = 0; channel < ADC_CHANNEL_COUNT; channel++) {
    unsigned int ldma_channel = adc_channels[channel].ldma_channel;

    if (ldma_channel != ADC_INVALID_LDMA_CHANNEL) {
      LDMA_StopTransfer((int)ldma_channel);
      (void)ldma_manager__set_callback(ldma_channel, NULL, NULL);
      adc__printf(true, "DEINITIALIZED %s DMA CHANNEL: %u\n",
                  (channel == ADC_CHANNEL_LEFT) ? "LEFT" : "RIGHT",
                  (unsigned int)ldma_channel);
      adc_channels[channel].ldma_channel = ADC_INVALID_LDMA_CHANNEL;
    }
  }

  library_initialized = false;
}

bool adc__get_audio_stereo_flag(void) { return stereo_flag; }

uint32_t adc__get_sample_size_bytes(void) { return adc_sample_size_bytes; }

bool adc__is_sample_little_endian(void) { return adc_sample_is_little_endian; }

bool adc__is_sample_left_justified(void) {
  return adc_sample_is_left_justified;
}

// -----------------------------------------------------------------------------
//                     ADC General End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     DMA Buffer Access
// -----------------------------------------------------------------------------

bool adc__get_oldest_left_dma_buffer(uint8_t **buffer, uint32_t *buffer_index,
                                     uint32_t *buffer_size) {
  return adc__get_oldest_dma_buffer(ADC_CHANNEL_LEFT, buffer, buffer_index,
                                    buffer_size);
}

bool adc__get_oldest_right_dma_buffer(uint8_t **buffer, uint32_t *buffer_index,
                                      uint32_t *buffer_size) {
  return adc__get_oldest_dma_buffer(ADC_CHANNEL_RIGHT, buffer, buffer_index,
                                    buffer_size);
}

void adc__mark_left_dma_buffer_stale(uint32_t buffer_index) {
  adc__mark_dma_buffer_stale(ADC_CHANNEL_LEFT, buffer_index);
}

void adc__mark_right_dma_buffer_stale(uint32_t buffer_index) {
  adc__mark_dma_buffer_stale(ADC_CHANNEL_RIGHT, buffer_index);
}

static bool adc__get_oldest_dma_buffer(adc_channel_t channel, uint8_t **buffer,
                                       uint32_t *buffer_index,
                                       uint32_t *buffer_size) {
  uint32_t oldest_index_timestamp = UINT32_MAX;
  bool found = false;

  if ((channel >= ADC_CHANNEL_COUNT) || (buffer == NULL) ||
      (buffer_index == NULL) || (buffer_size == NULL)) {
    return false;
  }

  adc_dma_buffer_t *buffers = adc_channels[channel].buffers;

  for (uint32_t i = 0; i < ADC_DMA_BUFFER_COUNT; i++) {
    if ((buffers[i].used == true) && (buffers[i].processed == false) &&
        (buffers[i].micros_timestamp < oldest_index_timestamp)) {
      oldest_index_timestamp = buffers[i].micros_timestamp;
      *buffer_index = i;
      found = true;
    }
  }

  if (found == true) {
    *buffer = (uint8_t *)buffers[*buffer_index].data;
    *buffer_size = sizeof(buffers[*buffer_index].data);
  }

  return found;
}

static void adc__mark_dma_buffer_stale(adc_channel_t channel,
                                       uint32_t buffer_index) {
  if ((channel >= ADC_CHANNEL_COUNT) || (buffer_index >= ADC_DMA_BUFFER_COUNT)) {
    return;
  }

  adc_dma_buffer_t *buffer = &adc_channels[channel].buffers[buffer_index];
  uint32_t buffers_in_use_counter = adc__counter_buffers_in_use(channel);

  if ((buffer->used == true) && (buffer->processed == false) &&
      (adc_counter_values[buffers_in_use_counter] > 0)) {
    adc_counter_values[buffers_in_use_counter]--;
    adc__update_buffer_counters(channel);
  }

  buffer->processed = true;
}

// -----------------------------------------------------------------------------
//                     DMA Buffer Access End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Interrupt Handlers
// -----------------------------------------------------------------------------

void USART0_RX_IRQHandler(void) {
  GPIO_PinModeSet(gpioPortB, 0, gpioModePushPull, 0);
  USART_IntClear(USART0, USART_IntGet(USART0));
}

// -----------------------------------------------------------------------------
//                     Interrupt Handlers End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     DMA Callbacks
// -----------------------------------------------------------------------------

static bool adc__left_dma_callback(unsigned int channel, unsigned int sequenceNo,
                                   void *userParam) {
  (void)channel;
  (void)userParam;

  return adc__dma_callback(ADC_CHANNEL_LEFT, sequenceNo);
}

static bool adc__right_dma_callback(unsigned int channel,
                                    unsigned int sequenceNo, void *userParam) {
  (void)channel;
  (void)userParam;

  return adc__dma_callback(ADC_CHANNEL_RIGHT, sequenceNo);
}

static bool adc__dma_callback(adc_channel_t channel, unsigned int sequence_no) {
  if (channel >= ADC_CHANNEL_COUNT) {
    return true;
  }

  adc_channels[channel].startup_buffer_ready = true;
  adc_counter_values[adc__counter_samples_received(channel)] +=
      ADC_BUFFER_SIZE / adc_sample_size_bytes;
  adc_counter_values[adc__counter_bytes_received(channel)] += ADC_BUFFER_SIZE;

  if ((channel == ADC_CHANNEL_RIGHT) || (stereo_flag == true)) {
    adc__mark_completed_buffer(channel, sequence_no % ADC_DMA_BUFFER_COUNT);
  }

  return true;
}

// -----------------------------------------------------------------------------
//                     DMA Callbacks End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     DMA Buffer Helpers
// -----------------------------------------------------------------------------

static void adc__mark_completed_buffer(adc_channel_t channel,
                                       uint32_t buffer_index) {
  if ((channel >= ADC_CHANNEL_COUNT) || (buffer_index >= ADC_DMA_BUFFER_COUNT)) {
    return;
  }

  adc_dma_buffer_t *buffer = &adc_channels[channel].buffers[buffer_index];
  const char *channel_name = (channel == ADC_CHANNEL_LEFT) ? "left" : "right";

  if ((buffer->used == true) && (buffer->processed == false)) {
    adc__printf(true,
                "ADC %s DMA buffer %u not stale yet, overwriting oldest "
                "unread data\n",
                channel_name, (unsigned int)buffer_index);
  } else {
    adc_counter_values[adc__counter_buffers_in_use(channel)]++;
  }

  buffer->used = true;
  buffer->processed = false;
  buffer->micros_timestamp = adc__get_microsecond_ticks();
  adc__update_buffer_counters(channel);
}

static void adc__update_buffer_counters(adc_channel_t channel) {
  uint32_t current_count =
      adc_counter_values[adc__counter_buffers_in_use(channel)];
  uint32_t min_counter = adc__counter_buffers_in_use_min(channel);
  uint32_t max_counter = adc__counter_buffers_in_use_max(channel);

  if (current_count < adc_counter_values[min_counter]) {
    adc_counter_values[min_counter] = current_count;
  }

  if (current_count > adc_counter_values[max_counter]) {
    adc_counter_values[max_counter] = current_count;
  }
}

static uint32_t adc__counter_samples_received(adc_channel_t channel) {
  return (channel == ADC_CHANNEL_LEFT) ? ADC_COUNTER_SAMPLES_RECEIVED_LEFT
                                       : ADC_COUNTER_SAMPLES_RECEIVED_RIGHT;
}

static uint32_t adc__counter_bytes_received(adc_channel_t channel) {
  return (channel == ADC_CHANNEL_LEFT) ? ADC_COUNTER_BYTES_RECEIVED_LEFT
                                       : ADC_COUNTER_BYTES_RECEIVED_RIGHT;
}

static uint32_t adc__counter_buffers_in_use(adc_channel_t channel) {
  return (channel == ADC_CHANNEL_LEFT) ? ADC_COUNTER_LEFT_BUFFERS_IN_USE
                                       : ADC_COUNTER_RIGHT_BUFFERS_IN_USE;
}

static uint32_t adc__counter_buffers_in_use_min(adc_channel_t channel) {
  return (channel == ADC_CHANNEL_LEFT) ? ADC_COUNTER_LEFT_BUFFERS_IN_USE_MIN
                                       : ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MIN;
}

static uint32_t adc__counter_buffers_in_use_max(adc_channel_t channel) {
  return (channel == ADC_CHANNEL_LEFT) ? ADC_COUNTER_LEFT_BUFFERS_IN_USE_MAX
                                       : ADC_COUNTER_RIGHT_BUFFERS_IN_USE_MAX;
}

// -----------------------------------------------------------------------------
//                     DMA Buffer Helpers End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Initialization Helpers
// -----------------------------------------------------------------------------

static bool adc__wait_for_startup_validation(void) {
  uint32_t start_cycles = DWT->CYCCNT;
  uint32_t timeout_cycles = CMU_ClockFreqGet(cmuClock_CORE);
  bool left_ready_logged = false;
  bool right_ready_logged = false;

  adc__printf(true, "Waiting for ADC startup buffers\n");

  while ((adc_channels[ADC_CHANNEL_LEFT].startup_buffer_ready == false) ||
         (adc_channels[ADC_CHANNEL_RIGHT].startup_buffer_ready == false)) {
    if ((adc_channels[ADC_CHANNEL_LEFT].startup_buffer_ready == true) &&
        (left_ready_logged == false)) {
      left_ready_logged = true;
      adc__printf(true, "ADC startup left buffer ready\n");
    }

    if ((adc_channels[ADC_CHANNEL_RIGHT].startup_buffer_ready == true) &&
        (right_ready_logged == false)) {
      right_ready_logged = true;
      adc__printf(true, "ADC startup right buffer ready\n");
    }

    if ((uint32_t)(DWT->CYCCNT - start_cycles) > timeout_cycles) {
      adc__printf(true, "ADC startup buffers validation timeout\n");
      return false;
    }
  }

  adc__print_sample_format_summary();

  adc__printf(true, "ADC Left buffer preview:\n");
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[0],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[1],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[2],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[3]);
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[4],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[5],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[6],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[7]);
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[8],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[9],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[10],
              (unsigned int)adc_channels[ADC_CHANNEL_LEFT].buffers[0].data[11]);

  adc__printf(true, "ADC Right buffer preview:\n");
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[0],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[1],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[2],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[3]);
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[4],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[5],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[6],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[7]);
  adc__printf(true, " 0x%X, 0x%X, 0x%X, 0x%X\n",
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[8],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[9],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[10],
              (unsigned int)adc_channels[ADC_CHANNEL_RIGHT].buffers[0].data[11]);

  adc__printf(true, "ADC startup buffers validated\n");
  return true;
}

static void adc__print_sample_format_summary(void) {
  adc__printf(true, "ADC sample size: %u bytes\n",
              (unsigned int)adc__get_sample_size_bytes());
  adc__printf(true, "ADC sample appears to be %s-endian\n",
              adc__is_sample_little_endian() ? "little" : "big");
  adc__printf(true, "ADC sample appears to be %s-justified\n",
              adc__is_sample_left_justified() ? "left" : "right");
}

static bool adc__start_dma_transfer(adc_channel_t channel) {
  if (channel >= ADC_CHANNEL_COUNT) {
    return false;
  }

  adc_channel_state_t *state = &adc_channels[channel];
  LDMA_PeripheralSignal_t signal = (channel == ADC_CHANNEL_LEFT)
                                       ? ldmaPeripheralSignal_USART0_RXDATAV
                                       : ldmaPeripheralSignal_USART0_RXDATAVRIGHT;
  bool (*callback)(unsigned int, unsigned int, void *) =
      (channel == ADC_CHANNEL_LEFT) ? adc__left_dma_callback
                                    : adc__right_dma_callback;

  for (uint32_t i = 0; i < ADC_DMA_BUFFER_COUNT; i++) {
    const int link_jump =
        (i + 1u < ADC_DMA_BUFFER_COUNT) ? 1 : -((int)ADC_DMA_BUFFER_COUNT - 1);
    state->descriptors[i] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(
        &USART0->RXDATA, state->buffers[i].data, ADC_BUFFER_SIZE, link_jump);
  }

  state->transfer_config = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(signal);

  if (!ldma_manager__set_callback(state->ldma_channel, callback, NULL)) {
    adc__printf(true, "LDMA %s callback registration failed\n",
                (channel == ADC_CHANNEL_LEFT) ? "left" : "right");
    return false;
  }

  LDMA_StartTransfer((int)state->ldma_channel, &state->transfer_config,
                     state->descriptors);
  return true;
}

static bool adc__init_i2s(void) {
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
  init.sync.clockMode = usartClockMode3;
  init.dmaSplit = true;
  init.mono = false;
  init.format = USART_I2SCTRL_FORMAT_W32D32;
  init.justify = usartI2sJustifyLeft;
  init.delay = false;

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
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN |
                                GPIO_USART_ROUTEEN_CSPEN |
                                GPIO_USART_ROUTEEN_CLKPEN;
#endif

  USART_InitI2s(USART0, &init);
  USART_IntEnable(USART0, USART_IEN_RXOF | USART_IEN_RXFULL);
  NVIC_EnableIRQ(USART0_RX_IRQn);
  return true;
}

static bool adc__ldma_init(void) {
  CMU_ClockEnable(cmuClock_LDMA, true);

  if ((adc_channels[ADC_CHANNEL_LEFT].ldma_channel ==
       ADC_INVALID_LDMA_CHANNEL) ||
      (adc_channels[ADC_CHANNEL_RIGHT].ldma_channel ==
       ADC_INVALID_LDMA_CHANNEL)) {
    adc__printf(true, "ADC LDMA channels not assigned\n");
    return false;
  }

  if (adc__start_dma_transfer(ADC_CHANNEL_LEFT) == false) {
    adc__printf(true, "ADC left LDMA transfer start failed\n");
    return false;
  }
  adc__printf(true, "Started LDMA_CHANNEL_RX_LEFT\n");

  if (adc__start_dma_transfer(ADC_CHANNEL_RIGHT) == false) {
    adc__printf(true, "ADC right LDMA transfer start failed\n");
    return false;
  }
  adc__printf(true, "Started LDMA_CHANNEL_RX_RIGHT\n");
  return true;
}

// -----------------------------------------------------------------------------
//                     Initialization Helpers End
// -----------------------------------------------------------------------------
