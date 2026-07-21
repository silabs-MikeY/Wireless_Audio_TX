#include "console_tx.h"

#include "em_core.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_eusart.h"
#include "sl_clock_manager.h"
#include "sl_device_clock.h"

#include <assert.h>

#if defined(EUSART_PRESENT) || defined(EUART_PRESENT)

typedef struct {
  const uint8_t *text;
  size_t remaining;
  console_tx_complete_callback_t callback;
  void *user_context;
} console_tx_transfer_state_t;

static const size_t console_tx__ldma_max_transfer_bytes = 2048U;
static bool console_tx_initialized = false;
static volatile bool console_tx_busy_flag = false;
static unsigned int console_tx_ldma_channel = 0U;
static console_tx_transfer_state_t console_tx_transfer_state = { 0 };
static LDMA_TransferCfg_t console_tx_transfer_cfg;
static LDMA_Descriptor_t console_tx_descriptor;

// -----------------------------------------------------------------------------
//                     Console TX Hardware Helpers
// -----------------------------------------------------------------------------

static void console_tx__enable_console_pins(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(CONSOLE_TX_EUSART_TX_PORT,
                  CONSOLE_TX_EUSART_TX_PIN,
                  gpioModePushPull,
                  1);
  GPIO_PinModeSet(CONSOLE_TX_EUSART_RX_PORT,
                  CONSOLE_TX_EUSART_RX_PIN,
                  gpioModeInputPull,
                  1);

#if defined(EUSART_PRESENT)
  GPIO->EUSARTROUTE[0].ROUTEEN =
    GPIO_EUSART_ROUTEEN_TXPEN | GPIO_EUSART_ROUTEEN_RXPEN;
  GPIO->EUSARTROUTE[0].TXROUTE =
    (CONSOLE_TX_EUSART_TX_PORT << _GPIO_EUSART_TXROUTE_PORT_SHIFT)
    | (CONSOLE_TX_EUSART_TX_PIN << _GPIO_EUSART_TXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[0].RXROUTE =
    (CONSOLE_TX_EUSART_RX_PORT << _GPIO_EUSART_RXROUTE_PORT_SHIFT)
    | (CONSOLE_TX_EUSART_RX_PIN << _GPIO_EUSART_RXROUTE_PIN_SHIFT);
#elif defined(EUART_PRESENT)
  GPIO->EUARTROUTE->ROUTEEN = GPIO_EUART_ROUTEEN_TXPEN | GPIO_EUART_ROUTEEN_RXPEN;
  GPIO->EUARTROUTE->TXROUTE =
    (CONSOLE_TX_EUSART_TX_PORT << _GPIO_EUART_TXROUTE_PORT_SHIFT)
    | (CONSOLE_TX_EUSART_TX_PIN << _GPIO_EUART_TXROUTE_PIN_SHIFT);
  GPIO->EUARTROUTE->RXROUTE =
    (CONSOLE_TX_EUSART_RX_PORT << _GPIO_EUART_RXROUTE_PORT_SHIFT)
    | (CONSOLE_TX_EUSART_RX_PIN << _GPIO_EUART_RXROUTE_PIN_SHIFT);
#endif
}

static EUSART_TypeDef *console_tx__peripheral(void)
{
  return CONSOLE_TX_EUSART_PERIPHERAL;
}

#if defined(EUSART_PRESENT) && defined(LDMAXBAR_CH_REQSEL_SIGSEL_EUSART0TXFL)
#define CONSOLE_TX_LDMA_SIGNAL ldmaPeripheralSignal_EUSART0_TXFL
#elif defined(EUART_PRESENT)
#define CONSOLE_TX_LDMA_SIGNAL ldmaPeripheralSignal_USART0_TXBL
#else
#error "console_tx requires a supported EUSART/EUART LDMA TX signal"
#endif

// -----------------------------------------------------------------------------
//                     Console TX Hardware Helpers End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Console TX DMA Transfer Helpers
// -----------------------------------------------------------------------------

static void console_tx__complete_transfer(console_tx_complete_callback_t callback,
                                          void *user_context)
{
  if (callback != NULL) {
    callback(user_context);
  }
}

static void console_tx__start_next_chunk(EUSART_TypeDef *eusart)
{
  size_t chunk_length = console_tx_transfer_state.remaining;
  const void *source;

  if (chunk_length == 0U) {
    return;
  }

  if (chunk_length > console_tx__ldma_max_transfer_bytes) {
    chunk_length = console_tx__ldma_max_transfer_bytes;
  }

  source = (const void *)console_tx_transfer_state.text;
  console_tx_transfer_cfg =
    (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(CONSOLE_TX_LDMA_SIGNAL);
  console_tx_descriptor =
    (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(source, &eusart->TXDATA, chunk_length);
  console_tx_descriptor.xfer.doneIfs = 0u;

  console_tx_transfer_state.text += chunk_length;
  console_tx_transfer_state.remaining -= chunk_length;

  LDMA_StartTransfer((int)console_tx_ldma_channel, &console_tx_transfer_cfg, &console_tx_descriptor);
}

// -----------------------------------------------------------------------------
//                     Console TX DMA Transfer Helpers End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Console TX General
// -----------------------------------------------------------------------------

bool console_tx__init(unsigned int ldma_channel)
{
  sl_status_t status;
  uint32_t frequency = 0U;
  EUSART_TypeDef *eusart = console_tx__peripheral();

  if (console_tx_initialized) {
    return (console_tx_ldma_channel == ldma_channel);
  }

  console_tx_ldma_channel = ldma_channel;
  console_tx_busy_flag = false;
  console_tx_transfer_state.text = NULL;
  console_tx_transfer_state.remaining = 0U;
  console_tx_transfer_state.callback = NULL;
  console_tx_transfer_state.user_context = NULL;

  CMU_ClockSelectSet(cmuClock_EUSART0CLK, cmuSelect_EM01GRPCCLK);
  CMU_ClockEnable(cmuClock_EUSART0, true);
  CMU_ClockEnable(cmuClock_LDMA, true);

  sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_EUSART0);
  console_tx__enable_console_pins();

  status = sl_clock_manager_get_clock_branch_frequency(SL_CLOCK_BRANCH_EUSART0CLK,
                                                       &frequency);
  assert(status == SL_STATUS_OK);

  EUSART_UartInit_TypeDef uart_init = EUSART_UART_INIT_DEFAULT_HF;
  uart_init.enable = eusartEnableTx;
  uart_init.refFreq = frequency;
  uart_init.baudrate = CONSOLE_TX_EUSART_BAUDRATE;

  EUSART_Reset(eusart);
  EUSART_UartInitHf(eusart, &uart_init);

  EUSART_IntClear(eusart, EUSART_IF_TXC);
  EUSART_IntDisable(eusart, EUSART_IF_TXC);
  NVIC_ClearPendingIRQ(EUSART0_TX_IRQn);
  NVIC_EnableIRQ(EUSART0_TX_IRQn);

  console_tx_initialized = true;
  return true;
}

bool console_tx__busy(void)
{
  return console_tx_busy_flag;
}

bool console_tx__print(const char *text,
                       size_t length,
                       console_tx_complete_callback_t callback,
                       void *user_context)
{
  return console_tx__write_async(text, length, callback, user_context);
}

bool console_tx__write_async(const char *text,
                             size_t length,
                             console_tx_complete_callback_t callback,
                             void *user_context)
{
  if (text == NULL) {
    return false;
  }

  if (length == 0U) {
    console_tx__complete_transfer(callback, user_context);
    return true;
  }

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  if (console_tx_busy_flag) {
    CORE_EXIT_CRITICAL();
    return false;
  }

  console_tx_busy_flag = true;
  console_tx_transfer_state.text = (const uint8_t *)text;
  console_tx_transfer_state.remaining = length;
  console_tx_transfer_state.callback = callback;
  console_tx_transfer_state.user_context = user_context;
  CORE_EXIT_CRITICAL();

  EUSART_TypeDef *eusart = console_tx__peripheral();

  EUSART_IntClear(eusart, EUSART_IF_TXC);
  EUSART_IntEnable(eusart, EUSART_IF_TXC);

  console_tx__start_next_chunk(eusart);
  return true;
}

void console_tx__write_blocking(const char *text, size_t length)
{
  if (text == NULL) {
    return;
  }

  if (length == 0U) {
    return;
  }

  while (!console_tx__write_async(text, length, NULL, NULL)) {
    ;
  }

  while (console_tx__busy()) {
    ;
  }
}

// -----------------------------------------------------------------------------
//                     Console TX General End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Interrupt Handlers
// -----------------------------------------------------------------------------

void EUSART0_TX_IRQHandler(void)
{
  EUSART_TypeDef *eusart = console_tx__peripheral();

  if ((eusart->IF & EUSART_IF_TXC) == 0U) {
    return;
  }

  EUSART_IntClear(eusart, EUSART_IF_TXC);

  CORE_DECLARE_IRQ_STATE;
  console_tx_complete_callback_t callback;
  void *user_context;

  CORE_ENTER_CRITICAL();
  if (console_tx_transfer_state.remaining > 0U) {
    CORE_EXIT_CRITICAL();
    console_tx__start_next_chunk(eusart);
    return;
  }

  console_tx_busy_flag = false;
  EUSART_IntDisable(eusart, EUSART_IF_TXC);
  callback = console_tx_transfer_state.callback;
  user_context = console_tx_transfer_state.user_context;
  console_tx_transfer_state.text = NULL;
  console_tx_transfer_state.remaining = 0U;
  console_tx_transfer_state.callback = NULL;
  console_tx_transfer_state.user_context = NULL;
  CORE_EXIT_CRITICAL();

  console_tx__complete_transfer(callback, user_context);
}

// -----------------------------------------------------------------------------
//                     Interrupt Handlers End
// -----------------------------------------------------------------------------
#endif
