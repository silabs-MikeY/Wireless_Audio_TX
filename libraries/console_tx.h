#ifndef CONSOLE_TX_H
#define CONSOLE_TX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_TX_EUSART_PERIPHERAL EUSART0
#define CONSOLE_TX_EUSART_TX_PORT SL_GPIO_PORT_A
#define CONSOLE_TX_EUSART_TX_PIN 7U
#define CONSOLE_TX_EUSART_RX_PORT SL_GPIO_PORT_A
#define CONSOLE_TX_EUSART_RX_PIN 8U
#define CONSOLE_TX_EUSART_BAUDRATE 3000000U

typedef void (*console_tx_complete_callback_t)(void *user_context);

bool console_tx__init(unsigned int ldma_channel);
bool console_tx__print(const char *text,
                       size_t length,
                       console_tx_complete_callback_t callback,
                       void *user_context);
bool console_tx__write_async(const char *text,
                             size_t length,
                             console_tx_complete_callback_t callback,
                             void *user_context);
void console_tx__write_blocking(const char *text, size_t length);
bool console_tx__busy(void);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_TX_H */
