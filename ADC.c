#include <stdio.h>
#include <assert.h>

#include "ADC.h"
#include "generic.h"
#include "audio_buffers.h"
#include "counters.h"
#include "print.h"

#include "dmadrv.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_usart.h"
#include "sl_hal_ldma.h"

static void adc__init_i2s(void);
static void adc__ldma_init_2(void);
static void adc__wait_for_startup_validation(void);



LDMA_Descriptor_t leftDesc[2];
LDMA_Descriptor_t rightDesc[2];
LDMA_TransferCfg_t leftCfg;
LDMA_TransferCfg_t rightCfg;

volatile uint8_t leftBuffer_1[BUFFER_SIZE];
volatile uint8_t leftBuffer_2[BUFFER_SIZE];

volatile uint8_t rightBuffer_1[BUFFER_SIZE];
volatile uint8_t rightBuffer_2[BUFFER_SIZE];

volatile bool new_data_ready_flag = false;
volatile uint32_t *new_data_pointer;

volatile uint8_t *leftBuffer_to_process;
volatile uint8_t *rightBuffer_to_process;

volatile uint32_t (*leftBuffer_output)[BUFFER_SIZE];
volatile uint32_t (*rightBuffer_output)[BUFFER_SIZE];

volatile uint8_t leftBuffer_converted[BUFFER_SIZE];
volatile uint8_t rightBuffer_converted[BUFFER_SIZE];

volatile uint32_t leftBuffer_converted_word[BUFFER_SIZE / 4];
volatile uint32_t rightBuffer_converted_word[BUFFER_SIZE / 4];

volatile uint8_t leftBuffer_test[BUFFER_SIZE];
volatile uint8_t rightBuffer_test[BUFFER_SIZE];

unsigned int LDMA_CHANNEL_LEFT;
unsigned int LDMA_CHANNEL_RIGHT;

extern uint32_t start;
extern volatile uint32_t debug_signals[10];

static bool stereo_flag = false;

static volatile bool startup_left_buffer_ready = false;
static volatile bool startup_right_buffer_ready = false;

bool new_packet_ready_left_flag = false;
bool new_packet_ready_right_flag = false;

uint8_t *new_packet_left_pointer = 0;
uint8_t *new_packet_right_pointer = 0;

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

/**
 * @brief Initializes the ADC subsystem.
 * Configures LDMA for DMA transfers and initializes I2S interface for audio input.
 * Must be called once at startup before audio samples are expected.
 * Sets stereo_flag to false (mono mode).
 * 
 * @param None
 * @return void
 */
void adc__init(void)
{
  stereo_flag = false;
  startup_left_buffer_ready = false;
  startup_right_buffer_ready = false;
  adc__ldma_init_2();
  adc__init_i2s();
  USART_Enable(USART0, usartEnable);
  adc__wait_for_startup_validation();
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
  debug__printf_to_buf_append_time(0,"DEINITIALIZING ADC\n");

  if (CMU->CLKEN0 & CMU_CLKEN0_USART0)
  {
    debug__printf_to_buf_append_time(0,"RESETTING USART\n");
    USART_Reset(USART0);
    debug__printf_to_buf_append_time(0,"RESET USART\n");
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
      debug__printf_to_buf_append_time(0,"DEINITIALIZED LEFT DMA CHANNEL: %u\n", (unsigned int)LDMA_CHANNEL_LEFT);
    }
    else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_ALREADY_FREED)
    {
      debug__printf_to_buf_append_time(0,"LEFT DMA CHANNEL: %u already freed\n", (unsigned int)LDMA_CHANNEL_LEFT);
    }
    else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_NOT_INITIALIZED)
    {
      assert(0);
    }

    DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_RIGHT);
    DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_RIGHT);
    if (DMADRV_return_status == ECODE_EMDRV_DMADRV_OK)
    {
      debug__printf_to_buf_append_time(0,"DEINITIALIZED RIGHT DMA CHANNEL: %u\n", (unsigned int)LDMA_CHANNEL_RIGHT);
    }
    else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_ALREADY_FREED)
    {
      debug__printf_to_buf_append_time(0,"RIGHT DMA CHANNEL: %u ALREADY FREED\n", (unsigned int)LDMA_CHANNEL_RIGHT);
    }
    else if (DMADRV_return_status == ECODE_EMDRV_DMADRV_NOT_INITIALIZED)
    {
      assert(0);
    }
  }

  // DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_LEFT);
  // if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  // {
  //   printf_to_buf_append_time(0,"DEINITIALIZED LEFT DMA: %u\n", (unsigned int)DMADRV_return_status);
  // }
  // else
  // {
  // }
  // DMADRV_return_status = DMADRV_StopTransfer(LDMA_CHANNEL_LEFT);
  // if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  // {
  //   printf_to_buf_append_time(0,"DEINITIALIZED RIGHT DMA: %u\n", (unsigned int)DMADRV_return_status);
  // }
  // else
  // {
  // }

  // DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_LEFT);
  // if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  // {
  //   printf_to_buf_append_time(0,"DEINITIALIZED RIGHT DMA: %u\n", (unsigned int)DMADRV_return_status);
  // }
  // else
  // {
  // }

  // DMADRV_return_status = DMADRV_FreeChannel(LDMA_CHANNEL_RIGHT);
  // if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  // {
  //   printf_to_buf_append_time(0,"DEINITIALIZED RIGHT DMA: %u\n", (unsigned int)DMADRV_return_status);
  // }
  // else
  // {
  // }

  //   DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_LEFT, NULL);
  // if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  //   {
  //     printf_to_buf_append_time(0,"Allocate Error: %X\n",(unsigned int)DMADRV_return_status);
  //     assert(0);
  //   }
}

bool adc__get_new_data_ready_flag(uint32_t **new_data_pointer_return)
{
  if (new_data_ready_flag == true)
  {
    new_data_ready_flag = false;
    // TODO FIX CASTING
    *new_data_pointer_return = (uint32_t *)new_data_pointer;
    return true;
  }
  return false;
}

// TODO make it not do sketchy iteration by 4.
//  void twos_complement_data(uint8_t* input, uint8_t* output, uint32_t input_size)
//  {
//    for (uint32_t i=0 ; i<(BUFFER_SIZE>>2) ; i++)
//      {
//        uint32_t* index = &((uint32_t*)input)[i];
//        uint32_t index_after;
//        if (*index & 0x80000000)
//          {
//            index_after = *index - 0x7FFFFFFF;
//          }
//        else
//          {
//            index_after = *index + 0x7FFFFFFF;
//          }

//       //      uint32_t index_after =  ~(*index) + 1;
//       int32_t index_after_signed_cast =  *((int32_t*)index);
//       *index  = index_after; //shift 32bit to 24bit
//     }
// }

void reverse_endianness_of_data(uint8_t *input, uint8_t *output, uint32_t input_size)
{
  if ((input_size & 0x3) != 0) // check if size is multiple of 4 bytes
  {
    // ERROR
  }

  for (uint32_t i = 0; i < input_size; i += 4)
  {
    output[i] = input[i + 3];
    output[i + 1] = input[i + 2];
    output[i + 2] = input[i + 1];
    output[i + 3] = input[i];
  }
}

/**
 * @brief Truncates 32-bit audio samples to 16-bit format.
 * Reduces I2S 32-bit words to 16-bit samples by dropping least significant bytes.
 * Input size must be multiple of 4 bytes. Returns false on invalid input size.
 * 
 * @param input - Source 32-bit audio buffer pointer
 * @param output - Destination 16-bit audio buffer pointer
 * @param input_size - Number of input bytes (must be multiple of 4)
 * @return true on success, false if input_size not multiple of 4
 */
bool truncate_data(uint8_t* input, uint8_t* output, uint32_t input_size)
{
  if ((input_size & 0x3) != 0) // check if size is multiple of 4 bytes
  {
    return false;
  }

  uint32_t j = 0;
  for (uint16_t i = 0; i < input_size;)
  {
    output[j + 1] = input[i];
    i++;
    j++;
    output[j - 1] = input[i];
    i++;
    j++;
    i++;
    i++;
  }
  return true;
}

/**
 * @brief Truncates 32-bit audio samples to 16-bit and writes to output buffer.
 * Similar to truncate_data() but used in callback contexts for direct buffer writes.
 * Input size must be multiple of 4 bytes.
 * 
 * @param input - Source 32-bit audio buffer pointer
 * @param output - Destination 16-bit audio buffer pointer
 * @param input_size - Number of input bytes (must be multiple of 4)
 * @return void
 */
void truncate_data_and_write_to_buffer(uint8_t *input, uint8_t *output, uint32_t input_size)
{
  if ((input_size & 0x3) != 0) // check if size is multiple of 4 bytes
  {
    // ERROR
  }

  uint32_t j = 0;
  for (uint16_t i = 0; i < input_size;)
  {
    output[j + 1] = input[i];
    i++;
    j++;
    output[j - 1] = input[i];
    i++;
    j++;
    i++;
    i++;
  }
}

/**
 * @brief Process packet placeholder (currently unimplemented).
 * Reserved for future packet processing logic.
 * 
 * @param left_or_right - Channel selector (LEFT or RIGHT)
 * @return void
 */
void process_packet(bool left_or_right)
{
  (void)left_or_right;
}

static void adc__wait_for_startup_validation(void)
{
  uint32_t start_cycles = DWT->CYCCNT;
  uint32_t timeout_cycles = CMU_ClockFreqGet(cmuClock_CORE);
  bool left_ready_logged = false;
  bool right_ready_logged = false;

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"Waiting for ADC startup buffers\n"));

  while ((startup_left_buffer_ready == false) || (startup_right_buffer_ready == false))
  {
    if ((startup_left_buffer_ready == true) && (left_ready_logged == false))
    {
      left_ready_logged = true;
      DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"ADC startup left buffer ready\n"));
    }

    if ((startup_right_buffer_ready == true) && (right_ready_logged == false))
    {
      right_ready_logged = true;
      DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"ADC startup right buffer ready\n"));
    }

    if ((uint32_t)(DWT->CYCCNT - start_cycles) > timeout_cycles)
    {
      assert(0);
      // printf("ADC startup buffers validation timeout\n");
      //custom_assert(0, __FILE__, __LINE__);
    }
  }

  DEBUG_PERIPHERALS_LOG(debug__printf_to_buf_append_time(0,"ADC startup buffers validated\n"));
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
  counters__add_to_counter(samples_received_left, RADIO_PACKET_DATA_SIZE_PER_CHANNEL >> 1); // divide by 2

  if (stereo_flag == false)
  {
    return true;
  }

  //  return true;

  if (sequenceNo & 0x1)
  {
    leftBuffer_to_process = leftBuffer_1;
  }
  else
  {
    leftBuffer_to_process = leftBuffer_2;
  }

  // uint8_t *output_buffer;
  // if (stereo_flag == false && ((sequenceNo & 0x1) != 0))
  // {
  //   output_buffer = radio__get_pointer_to_data_buffer_channel(LEFT);
  //   // TODO FIX CASTING
  //   truncate_data_and_write_to_buffer((uint8_t *)leftBuffer_to_process, output_buffer, RADIO_PACKET_DATA_SIZE);
  //   radio__indicate_data_buffer_was_filled_by_pointer(LEFT);
  // }
  // else
  // {
  //   output_buffer = radio__get_pointer_to_data_buffer_channel(RIGHT);
  //   // TODO FIX CASTING
  //   truncate_data_and_write_to_buffer((uint8_t *)leftBuffer_to_process, output_buffer, RADIO_PACKET_DATA_SIZE);
  //   radio__indicate_data_buffer_was_filled_by_pointer(RIGHT);
  // }
  // new_packet_ready_left_flag = true;
  // new_packet_left_pointer = output_buffer;

  // debug__increment_number_of_left_samples_receiver(RADIO_PACKET_DATA_SIZE_PER_CHANNEL >> 1); // divide by 2

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

  if (sequenceNo & 0x1)
  {
    rightBuffer_to_process = rightBuffer_1;
  }
  else
  {
    rightBuffer_to_process = rightBuffer_2;
  }

// CORE_DECLARE_IRQ_STATE;
//     CORE_ENTER_CRITICAL();
  
//   for (uint32_t i=0 ; i<BUFFER_SIZE ; i++)
//   {
//     rightBuffer_to_process[i] = i & 0xFF;
//   }

  uint8_t output_buffer[RADIO_PACKET_DATA_SIZE_PER_CHANNEL];
  truncate_data((uint8_t *)rightBuffer_to_process, output_buffer, RADIO_PACKET_DATA_SIZE);

  audio_buffers__add_new_data_to_right_buffer((uint8_t *)output_buffer);

  startup_right_buffer_ready = true;

// CORE_EXIT_CRITICAL();




  //uint8_t* radio__get_next_buffer(RIGHT);

  // uint8_t *output_buffer;
  // if (stereo_flag == false && ((sequenceNo & 0x1) != 0))
  // {
  //   output_buffer = radio__get_pointer_to_data_buffer_channel(LEFT);

  //   // TODO FIX CASTING
  //   truncate_data_and_write_to_buffer((uint8_t *)rightBuffer_to_process, output_buffer, RADIO_PACKET_DATA_SIZE);

  //   //  memset(output_buffer, 0,RADIO_PACKET_DATA_SIZE/2);

  //   radio__indicate_data_buffer_was_filled_by_pointer(LEFT);
  // }
  // else
  // {
  //   output_buffer = radio__get_pointer_to_data_buffer_channel(RIGHT);

  //   // TODO FIX CASTING
  //   truncate_data_and_write_to_buffer((uint8_t *)rightBuffer_to_process, output_buffer, RADIO_PACKET_DATA_SIZE);

  //   //  memset(output_buffer, 0,RADIO_PACKET_DATA_SIZE/2);

  //   radio__indicate_data_buffer_was_filled_by_pointer(RIGHT);
  // }

  // new_packet_ready_right_flag = true;
  // new_packet_right_pointer = output_buffer;

  counters__add_to_counter(samples_received_right, RADIO_PACKET_DATA_SIZE_PER_CHANNEL >> 1); // divide by 2

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
    debug__printf_to_buf_append_time(0,"DMADRV init failed: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }

  DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_LEFT, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  {
    debug__printf_to_buf_append_time(0,"Allocate Error: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }
  debug__printf_to_buf_append_time(0,"Got RX Left DMA Channel: %u\n", (unsigned int)LDMA_CHANNEL_LEFT);

  DMADRV_return_status = DMADRV_AllocateChannel(&LDMA_CHANNEL_RIGHT, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  {
    debug__printf_to_buf_append_time(0,"Allocate Error: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }
  debug__printf_to_buf_append_time(0,"Got RX Right DMA Channel: %u\n", (unsigned int)LDMA_CHANNEL_RIGHT);

  DMADRV_return_status = DMADRV_PeripheralMemoryPingPong(LDMA_CHANNEL_LEFT, dmadrvPeripheralSignal_USART0_RXDATAV, (void *)leftBuffer_1, (void *)leftBuffer_2, (void *)&(USART0->RXDATA), true, BUFFER_SIZE, SL_HAL_LDMA_CTRL_SIZE_BYTE, DMA_left_callback, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  {
    debug__printf_to_buf_append_time(0,"LDMA_CHANNEL_RX_LEFT start Error: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }
  debug__printf_to_buf_append_time(0,"Started LDMA_CHANNEL_RX_LEFT\n");

  DMADRV_return_status = DMADRV_PeripheralMemoryPingPong(LDMA_CHANNEL_RIGHT, dmadrvPeripheralSignal_USART0_RXDATAVRIGHT, (void *)rightBuffer_1, (void *)rightBuffer_2, (void *)&(USART0->RXDATA), true, BUFFER_SIZE, SL_HAL_LDMA_CTRL_SIZE_BYTE, DMA_right_callback, NULL);
  if (DMADRV_return_status != ECODE_EMDRV_DMADRV_OK)
  {
    debug__printf_to_buf_append_time(0,"LDMA_CHANNEL_RX_RIGHT start Error: %X\n", (unsigned int)DMADRV_return_status);
    assert(0);
  }
  debug__printf_to_buf_append_time(0,"Started LDMA_CHANNEL_RX_RIGHT\n");
}

// void initLDMA(void)
//{
//   // Default LDMA init
//   LDMA_Init_t init = LDMA_INIT_DEFAULT;
//   LDMA_Init(&init);
//
//   // Configure LDMA for transfer from USART to memory (left channel)
//   // LDMA will loop continuously
//   //LDMA_TransferCfg_t
//   leftCfg= (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_RXDATAV);
//
//   //Globally store and configure link descriptors for left microphone transfer
//
////  LDMA_Descriptor_t leftXfer_1 = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, leftBuffer_1, BUFFER_SIZE, 1);
////  leftDesc[0] = leftXfer_1;
////  LDMA_Descriptor_t leftXfer_2 = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, leftBuffer_1, BUFFER_SIZE, -1);
////  leftDesc[1]  = leftXfer_2;
//
//  LDMA_Descriptor_t leftXfer_1  = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, (uint8_t *)leftBuffer_1, BUFFER_SIZE, 1);
//  LDMA_Descriptor_t leftXfer_2  = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, (uint8_t *)leftBuffer_2, BUFFER_SIZE, 1);
//  LDMA_Descriptor_t leftXfer_3  = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, (uint8_t *)leftBuffer_3, BUFFER_SIZE, 0);
//  leftDesc[0] = leftXfer_1;
//  leftDesc[1] = leftXfer_2;
//  leftDesc[2] = leftXfer_3;
//
//  //leftXfer[1] = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, leftBuffer_2, BUFFER_SIZE, -1);
//  //leftDesc = leftXfer;
//  // trigger interrupt on left microphone transfer complete (buffer full)
//  leftDesc[0].xfer.doneIfs = 1;
//  leftDesc[0].xfer.ignoreSrec = 0;
//  leftDesc[0].xfer.size = ldmaCtrlSizeByte;
//  leftDesc[1].xfer.doneIfs = 1;
//  leftDesc[1].xfer.ignoreSrec = 0;
//  leftDesc[1].xfer.size = ldmaCtrlSizeByte;
//  leftDesc[2].xfer.doneIfs = 1;
//  leftDesc[2].xfer.ignoreSrec = 0;
//  leftDesc[2].xfer.size = ldmaCtrlSizeByte;
//
//  //leftDesc.xfer.dstInc = ldmaCtrlDstIncFour;
//  //leftDesc.xfer.blockSize = ldmaCtrlBlockSizeUnit1;
//
//
//
//  // Configure LDMA for transfer from USART to memory (right channel)
//  // LDMA will loop continuously and discard right channel data
//  rightCfg = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_RXDATAVRIGHT);
//  // Set up right microphone descriptor
//  //LDMA_Descriptor_t rightXfer
//  rightDesc[0] = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, rightBuffer_1, BUFFER_SIZE/2, 1);
//  rightDesc[1] =(LDMA_Descriptor_t) LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(&USART0->RXDATA, rightBuffer_3, BUFFER_SIZE, 0);
//  //rightDesc = rightXfer;
//  //rightDesc = rightXfer;
//
//  // Don't trigger interrupts on right microphone transfers
//  rightDesc[0].xfer.doneIfs = 1;
//  rightDesc[1].xfer.doneIfs = 1;
//  //rightDesc.xfer.dstInc = 0;
//  rightDesc[0].xfer.ignoreSrec = 0;
//  rightDesc[1].xfer.ignoreSrec = 0;
//  //rightDesc.xfer.structReq = 1;
//  rightDesc[0].xfer.size = ldmaCtrlSizeByte;
//  rightDesc[1].xfer.size = ldmaCtrlSizeByte;
//  //rightDesc.xfer.blockSize = ldmaCtrlBlockSizeUnit1;
//
//  LDMA_IntClear((0xFFFFFFFF));
//  LDMA_IntEnable((0x1<<TX_LDMA_CHANNEL)|(0x1<<RX_LDMA_CHANNEL));// | LDMA_IF_DONE3);
//  NVIC_EnableIRQ(LDMA_IRQn);
//
//  // Ensure destination address does not increment
//  //rightDesc.xfer.dstInc = 0;
//
//  // Start left and right transfers
//
////  uint32_t number_of_buffer_fills_per_second = (SAMPLE_FREQ / BUFFER_SIZE * 4);
////  uint32_t microseconds_to_fill_buffer = 1000000 / number_of_buffer_fills_per_second;
////  uint32_t number_of_ticks_per_microsecond = CMU_ClockFreqGet(cmuClock_SYSCLK) / 1000000;
////  uint32_t number_of_ticks_to_fill_half_buffer = number_of_ticks_per_microsecond * microseconds_to_fill_buffer / 2;
//
//  //USART_Reset(USART0);
//
//
//
//  LDMA_StartTransfer(RX_LDMA_CHANNEL, (void *)&leftCfg, (void *)&leftDesc);
//
////  uint32_t current_ticks = DWT->CYCCNT;
////  while ((DWT->CYCCNT - current_ticks) < number_of_ticks_to_fill_half_buffer){}
//
//  LDMA_StartTransfer(TX_LDMA_CHANNEL, (void *)&rightCfg, (void *)&rightDesc);
//
//  //LDMA
//
//
////  DMADRV_Init();
////  uint32_t status;
////  unsigned int dmaChannelLeft;
////  status = DMADRV_AllocateChannel(&dmaChannelLeft, 0);
////  DMADRV_MemoryPeripheral(dmaChannelLeft, dmadrvPeripheralSignal_USART0_TXBL, &USART0->TXDATA, leftBuffer, true, BUFFER_SIZE, dmadrvDataSize1, 0, 0);
////
////  unsigned int dmaChannelRight;
////  status = DMADRV_AllocateChannel(&dmaChannelRight, 0);
////  DMADRV_MemoryPeripheral(dmaChannelRight, dmadrvPeripheralSignal_USART0_TXBLRIGHT, &USART0->TXDATA, rightBuffer, true, BUFFER_SIZE, dmadrvDataSize1, 0, 0);
//
//
//}
