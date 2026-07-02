#include <stdio.h>
#include <assert.h>

#include "ADC.h"

#include "dmadrv.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_usart.h"
#include "sl_hal_ldma.h"

static void adc__init_i2s(void);
static void adc__ldma_init_2(void);
static void adc__wait_for_startup_validation(void);
static void adc__print_startup_buffer_preview(const char *buffer_name, const volatile uint8_t *buffer);
static void adc__print_sample_format_summary(void);

typedef enum adc_counter_index_s {
  ADC_COUNTER_SAMPLES_RECEIVED_LEFT = 0,
  ADC_COUNTER_SAMPLES_RECEIVED_RIGHT = 1,
  ADC_COUNTER_BYTES_RECEIVED_LEFT = 2,
  ADC_COUNTER_BYTES_RECEIVED_RIGHT = 3,
  ADC_COUNTER_BYTES_OUTPUT_LEFT = 4,
  ADC_COUNTER_BYTES_OUTPUT_RIGHT = 5,
  ADC_NUMBER_OF_COUNTERS
} adc_counter_index_t;

static volatile uint32_t adc_counter_values[ADC_NUMBER_OF_COUNTERS] = {0};
static const char *adc_counter_names[ADC_NUMBER_OF_COUNTERS] = {
  "adc_samples_received_left",
  "adc_samples_received_right",
  "adc_bytes_received_left",
  "adc_bytes_received_right",
  "adc_bytes_output_left",
  "adc_bytes_output_right",
};

volatile bool library_initialized = false;

LDMA_Descriptor_t leftDesc[2];
LDMA_Descriptor_t rightDesc[2];
LDMA_TransferCfg_t leftCfg;
LDMA_TransferCfg_t rightCfg;

volatile uint8_t leftBuffer_1[BUFFER_SIZE] __attribute__((aligned(4)));; 
volatile uint8_t leftBuffer_2[BUFFER_SIZE] __attribute__((aligned(4)));;

volatile uint8_t rightBuffer_1[BUFFER_SIZE] __attribute__((aligned(4)));;
volatile uint8_t rightBuffer_2[BUFFER_SIZE] __attribute__((aligned(4)));;

volatile bool new_data_ready_flag = false;
volatile uint32_t *new_data_pointer;

volatile uint8_t *leftBuffer_to_process;
volatile uint8_t *rightBuffer_to_process;

volatile uint32_t (*leftBuffer_output)[BUFFER_SIZE];
volatile uint32_t (*rightBuffer_output)[BUFFER_SIZE];

volatile uint8_t leftBuffer_converted[BUFFER_SIZE] __attribute__((aligned(4)));;
volatile uint8_t rightBuffer_converted[BUFFER_SIZE] __attribute__((aligned(4)));;

volatile uint32_t leftBuffer_converted_word[BUFFER_SIZE / 4];
volatile uint32_t rightBuffer_converted_word[BUFFER_SIZE / 4];

volatile uint8_t leftBuffer_test[BUFFER_SIZE] __attribute__((aligned(4)));;
volatile uint8_t rightBuffer_test[BUFFER_SIZE] __attribute__((aligned(4)));;

unsigned int LDMA_CHANNEL_LEFT;
unsigned int LDMA_CHANNEL_RIGHT;

extern uint32_t start;
extern volatile uint32_t debug_signals[10];

static bool stereo_flag = false;

static volatile bool startup_left_buffer_ready = false;
static volatile bool startup_right_buffer_ready = false;

static const uint32_t adc_sample_size_bytes = 4;
static const bool adc_sample_is_little_endian = true;
static const bool adc_sample_is_left_justified = true;

bool new_packet_ready_left_flag = false;
bool new_packet_ready_right_flag = false;

uint8_t *new_packet_left_pointer = 0;
uint8_t *new_packet_right_pointer = 0;

__attribute__((weak)) void adc__process_left_buffer(uint8_t *buffer)
{
  (void)buffer;
  assert(0); // This function should be implemented by the user to process left channel audio data.
}

__attribute__((weak)) void adc__process_right_buffer(uint8_t *buffer)
{
  (void)buffer;
  assert(0); // This function should be implemented by the user to process right channel audio data.
}

__attribute__((weak)) void adc__printf(bool add_timestamp, const char *format, ...)
{
  (void)add_timestamp;
  (void)format;
}

/**
 * @brief Gets the next audio packet ready for processing from the right channel.
 * Returns pointer to the oldest right channel buffer if available.
 * Clears the ready flag after returning data.
 * 
 * @param data_pointer - Output pointer to audio data (pointer to pointer)
 * @return true if packet available, false if no data ready
 */
bool adc__get_new_packet_ready_for_processing(uint8_t **data_pointer)
{
  if (new_packet_ready_right_flag == true)
  {
    if (new_packet_right_pointer != 0)
    {
      new_packet_ready_right_flag = false;
      *data_pointer = new_packet_right_pointer;
      return true;
    }
  }
  return false;
}

/**
 * @brief Gets the audio stereo flag state.
 * Indicates whether the ADC is configured for stereo (true) or mono (false) mode.
 * 
 * @param None
 * @return true if stereo mode enabled, false if mono
 */
bool adc__get_audio_stereo_flag(void)
{
  return stereo_flag;
}

uint32_t adc__get_sample_size_bytes(void)
{
  return adc_sample_size_bytes;
}

bool adc__is_sample_little_endian(void)
{
  return adc_sample_is_little_endian;
}

bool adc__is_sample_left_justified(void)
{
  return adc_sample_is_left_justified;
}

uint32_t adc__get_number_of_counters(void)
{
  return ADC_NUMBER_OF_COUNTERS;
}

const char* adc__get_counter_name(uint32_t counter_index)
{
  if (counter_index >= ADC_NUMBER_OF_COUNTERS)
  {
    return NULL;
  }

  return adc_counter_names[counter_index];
}

volatile uint32_t* adc__get_counter_address(uint32_t counter_index)
{
  if (counter_index >= ADC_NUMBER_OF_COUNTERS)
  {
    return NULL; // Invalid index
  }

  return (volatile uint32_t *)&adc_counter_values[counter_index];
}

void adc__reset_counters(void)
{
  adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_LEFT] = 0;
  adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_RIGHT] = 0;
  adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_LEFT] = 0;
  adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_RIGHT] = 0;
  adc_counter_values[ADC_COUNTER_BYTES_OUTPUT_LEFT] = 0;
  adc_counter_values[ADC_COUNTER_BYTES_OUTPUT_RIGHT] = 0;
}

/**
 * @brief Initializes the ADC subsystem.
 * Configures LDMA for DMA transfers and initializes I2S interface for audio input.
 * Must be called once at startup before audio samples are expected.
 * 
 * @param None
 * @return void
 */
void adc__init(bool is_stereo)
{
  if (library_initialized == true)
  {
    adc__printf(true, "ADC Library Already Initialized. De-Initializing\n");
    adc__deinit();
  }

  stereo_flag = is_stereo;
  adc__printf(true, "ADC Init Stereo Flag: %u\n", (unsigned int)stereo_flag);
  adc__reset_counters();
  startup_left_buffer_ready = false;
  startup_right_buffer_ready = false;
  adc__ldma_init_2();
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
void adc__deinit(void)
{
  adc__printf(true, "DEINITIALIZING ADC\n");

  if (CMU->CLKEN0 & CMU_CLKEN0_USART0)
  {
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

  if (!((LDMA_CHANNEL_LEFT == 0) && (LDMA_CHANNEL_RIGHT == 0)))
  {
    Ecode_t DMADRV_return_status;

    DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_LEFT);
    DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_LEFT);
    if (DMADRV_return_status == ECODE_EMDRV_DMADRV_OK)
    {
      adc__printf(true, "DEINITIALIZED LEFT DMA CHANNEL: %u\n", (unsigned int)LDMA_CHANNEL_LEFT);
    }
    else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_ALREADY_FREED)
    {
      adc__printf(true, "LEFT DMA CHANNEL: %u already freed\n", (unsigned int)LDMA_CHANNEL_LEFT);
    }
    else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_NOT_INITIALIZED)
    {
      assert(0);
    }

    DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_RIGHT);
    DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_RIGHT);
    if (DMADRV_return_status == ECODE_EMDRV_DMADRV_OK)
    {
      adc__printf(true, "DEINITIALIZED RIGHT DMA CHANNEL: %u\n", (unsigned int)LDMA_CHANNEL_RIGHT);
    }
    else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_ALREADY_FREED)
    {
      adc__printf(true, "RIGHT DMA CHANNEL: %u ALREADY FREED\n", (unsigned int)LDMA_CHANNEL_RIGHT);
    }
    else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_NOT_INITIALIZED)
    {
      assert(0);
    }
  }
}

bool adc__get_new_data_ready_flag(uint32_t **new_data_pointer_return)
{
  if (new_data_ready_flag == true)
  {
    new_data_ready_flag = false;
    *new_data_pointer_return = (uint32_t *)new_data_pointer;
    return true;
  }
  return false;
}

static void adc__wait_for_startup_validation(void)
{
  uint32_t start_cycles = DWT->CYCCNT;
  uint32_t timeout_cycles = CMU_ClockFreqGet(cmuClock_CORE);
  bool left_ready_logged = false;
  bool right_ready_logged = false;

  adc__printf(true, "Waiting for ADC startup buffers\n");

  while ((startup_left_buffer_ready == false) || (startup_right_buffer_ready == false))
  {
    if ((startup_left_buffer_ready == true) && (left_ready_logged == false))
    {
      left_ready_logged = true;
      adc__printf(true, "ADC startup left buffer ready\n");
    }

    if ((startup_right_buffer_ready == true) && (right_ready_logged == false))
    {
      right_ready_logged = true;
      adc__printf(true, "ADC startup right buffer ready\n");
    }

    if ((uint32_t)(DWT->CYCCNT - start_cycles) > timeout_cycles)
    {
      assert(0);
      // printf("ADC startup buffers validation timeout\n");
      //custom_assert(0, __FILE__, __LINE__);
    }
  }

  adc__print_sample_format_summary();
  adc__print_startup_buffer_preview("left", leftBuffer_to_process);
  adc__print_startup_buffer_preview("right", rightBuffer_to_process);

  adc__printf(true, "ADC startup buffers validated\n");
}

static void adc__print_sample_format_summary(void)
{
  adc__printf(true, "ADC sample size: %u bytes\n", (unsigned int)adc__get_sample_size_bytes());
  adc__printf(true, "ADC sample appears to be %s-endian\n",
              adc__is_sample_little_endian() ? "little" : "big");
  adc__printf(true, "ADC sample appears to be %s-justified\n",
              adc__is_sample_left_justified() ? "left" : "right");
}

static void adc__print_startup_buffer_preview(const char *buffer_name, const volatile uint8_t *buffer)
{
  const uint32_t sample_size_bytes = adc__get_sample_size_bytes();

  if ((buffer == NULL) || (sample_size_bytes == 0))
  {
    adc__printf(true, "ADC %s buffer preview unavailable\n", buffer_name);
    return;
  }

  adc__printf(true, "ADC %s buffer preview\n", buffer_name);

  for (uint32_t sample_index = 0; sample_index < 4; ++sample_index)
  {
    const volatile uint8_t *sample = buffer + (sample_index * sample_size_bytes);

    adc__printf(true,
                "%s sample %u: %02X %02X %02X %02X\n",
                buffer_name,
                (unsigned int)sample_index,
                (unsigned int)sample[0],
                (unsigned int)sample[1],
                (unsigned int)sample[2],
                (unsigned int)sample[3]);
  }
}

/**
 * @brief DMA callback for left channel audio transfer completion.
 * Called when LDMA finishes transferring a left audio buffer.
 * Sets leftBuffer_to_process to the completed buffer (1 or 2).
 * Currently only functional in stereo mode.
 * 
 * @param channel - LDMA channel identifier (unused)
 * @param sequenceNo - Sequence number indicating which buffer (0 or 1)
 * @param userParam - User parameter (unused)
 * @return true always
 */
static bool DMA_left_callback(unsigned int channel, unsigned int sequenceNo, void *userParam)
{
  (void)channel;
  (void)sequenceNo;
  (void)userParam;

   // printf_to_buf_append_time(0,"Left ADC seq: %X\n",(unsigned int)sequenceNo);

  startup_left_buffer_ready = true;
  adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_LEFT] += RADIO_PACKET_DATA_SIZE_PER_CHANNEL >> 1; // divide by 2
  adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_LEFT] += BUFFER_SIZE;

  if (stereo_flag == false)
  {
    return true;
  }

  if (sequenceNo & 0x1)
  {
    leftBuffer_to_process = leftBuffer_1;
  }
  else
  {
    leftBuffer_to_process = leftBuffer_2;
  }

  new_data_pointer = (volatile uint32_t *)leftBuffer_to_process;
  new_data_ready_flag = true;

  adc_counter_values[ADC_COUNTER_BYTES_OUTPUT_LEFT] += RADIO_PACKET_DATA_SIZE;
  adc__process_left_buffer((uint8_t *)leftBuffer_to_process);

  return true;
}

/**
 * @brief DMA callback for right channel audio transfer completion.
 * Called when LDMA finishes transferring a right audio buffer.
 * Truncates 32-bit I2S data to 16-bit format and adds to audio buffer queue.
 * Called from interrupt context, so very timing-critical.
 * 
 * @param channel - LDMA channel identifier (unused)
 * @param sequenceNo - Sequence number indicating which buffer (0 or 1)
 * @param userParam - User parameter (unused)
 * @return true always
 */
static bool DMA_right_callback(unsigned int channel, unsigned int sequenceNo, void *userParam)
{
  (void)channel;
  (void)sequenceNo;
  (void)userParam;

  //  printf_to_buf_append_time(0,"Right ADC seq: %X\n",(unsigned int)sequenceNo);

  startup_right_buffer_ready = true;
  adc_counter_values[ADC_COUNTER_SAMPLES_RECEIVED_RIGHT] += RADIO_PACKET_DATA_SIZE_PER_CHANNEL >> 1; // divide by 2
  adc_counter_values[ADC_COUNTER_BYTES_RECEIVED_RIGHT] += BUFFER_SIZE;

  if (sequenceNo & 0x1)
  {
    rightBuffer_to_process = rightBuffer_1;
  }
  else
  {
    rightBuffer_to_process = rightBuffer_2;
  }

  new_data_pointer = (volatile uint32_t *)rightBuffer_to_process;
  new_data_ready_flag = true;

  adc_counter_values[ADC_COUNTER_BYTES_OUTPUT_RIGHT] += RADIO_PACKET_DATA_SIZE;
  adc__process_right_buffer((uint8_t *)rightBuffer_to_process);

  return true;
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
void USART0_RX_IRQHandler(void)
{
  debug_signals[0] = USART_IntGet(USART0);
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
void adc__init_i2s(void)
{
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

  GPIO->USARTROUTE[0].CSROUTE = (I2S_CS_PORT << _GPIO_USART_CSROUTE_PORT_SHIFT) | (I2S_CS_PIN << _GPIO_USART_CSROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].CLKROUTE = (I2S_CLK_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT) | (I2S_CLK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);

#if (I2S_MODE == I2S_MODE_MASTER)
  GPIO->USARTROUTE[0].TXROUTE = (I2S_DATAOUT_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (I2S_DATAOUT_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].RXROUTE = (I2S_DATAIN_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (I2S_DATAIN_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_CSPEN | GPIO_USART_ROUTEEN_CLKPEN;
#elif (I2S_MODE == I2S_MODE_SLAVE)
  GPIO->USARTROUTE[0].TXROUTE = (I2S_DATAIN_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (I2S_DATAIN_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  // GPIO->USARTROUTE[0].RXROUTE = (I2S_DATAOUT_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)| (I2S_DATAOUT_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_CSPEN | GPIO_USART_ROUTEEN_CLKPEN;
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
 * Allocates two DMA channels (left and right) and configures ping-pong transfers.
 * Sets up transfers from USART0 RX (I2S data) to dual audio buffers.
 * Attaches DMA callbacks for buffer completion notification.
 * Must be called before I2S initialization.
 * 
 * @param None
 * @return void
 */
void adc__ldma_init_2(void)
{
  CMU_ClockEnable(cmuClock_LDMA, true);
  Ecode_t DMADRV_return_status;

  DMADRV_return_status = DMADRV_Init();
  if ((DMADRV_return_status != ECODE_EMDRV_DMADRV_OK) && (DMADRV_return_status != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED))
  {
    adc__printf(true, "DMADRV init failed: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }

  DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_LEFT, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  {
    adc__printf(true, "Allocate Error: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }
  adc__printf(true, "Got RX Left DMA Channel: %u\n", (unsigned int)LDMA_CHANNEL_LEFT);

  DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_RIGHT, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  {
    adc__printf(true, "Allocate Error: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }
  adc__printf(true, "Got RX Right DMA Channel: %u\n", (unsigned int)LDMA_CHANNEL_RIGHT);

  DMADRV_return_status = DMADRV_PeripheralMemoryPingPong(LDMA_CHANNEL_LEFT, dmadrvPeripheralSignal_USART0_RXDATAV, (void *)leftBuffer_1, (void *)leftBuffer_2, (void *)&(USART0->RXDATA), true, BUFFER_SIZE, SL_HAL_LDMA_CTRL_SIZE_BYTE, DMA_left_callback, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  {
    adc__printf(true, "LDMA_CHANNEL_RX_LEFT start Error: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }
  adc__printf(true, "Started LDMA_CHANNEL_RX_LEFT\n");

  DMADRV_return_status = DMADRV_PeripheralMemoryPingPong(LDMA_CHANNEL_RIGHT, dmadrvPeripheralSignal_USART0_RXDATAVRIGHT, (void *)rightBuffer_1, (void *)rightBuffer_2, (void *)&(USART0->RXDATA), true, BUFFER_SIZE, SL_HAL_LDMA_CTRL_SIZE_BYTE, DMA_right_callback, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  {
    adc__printf(true, "LDMA_CHANNEL_RX_RIGHT start Error: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }
  adc__printf(true, "Started LDMA_CHANNEL_RX_RIGHT\n");
}