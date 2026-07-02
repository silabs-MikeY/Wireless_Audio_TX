#ifndef HARDWARE_CONFIG_H_
#define HARDWARE_CONFIG_H_


// ADC
#define I2S_PORT                              gpioPortC
#define I2S_BCLK_PIN                          4
#define I2S_BCLK_PORT                         gpioPortC
#define I2S_LRCLK_PIN                         5
#define I2S_LRCLK_PORT                        gpioPortC
#define I2S_DATA_PIN                          3
#define I2S_DATA_PORT                         gpioPortC
//#define I2S_DUMMY_TX_DEBUG_PIN                6
//#define I2S_DUMMY_TX_DEBUG_PORT               gpioPortC
#define I2S_DUMMY_TX_DEBUG_PIN                1
#define I2S_DUMMY_TX_DEBUG_PORT               gpioPortB

#define I2S_USART_INSTANCE                    USART0

#define SAMPLE_FREQ       (48000)
// ADC END


// RGB LEDs
#define RGB1_GREEN_PORT                       gpioPortB
#define RGB1_GREEN_PIN                        0

#define RGB1_BLUE_PORT                        gpioPortB
#define RGB1_BLUE_PIN                         1

#define RGB1_RED_PORT                         gpioPortA
#define RGB1_RED_PIN                          0

#define RGB1_TIMER                            (TIMER2)

#define RGB2_GREEN_PORT                       gpioPortA
#define RGB2_GREEN_PIN                        4

#define RGB2_BLUE_PORT                        gpioPortA
#define RGB2_BLUE_PIN                         5

#define RGB2_RED_PORT                         gpioPortA
#define RGB2_RED_PIN                          6

#define RGB2_TIMER                            (TIMER3)
// RGB LEDs END

// Scheduler
#define MICROS_TICK_TIMER                     (TIMER0)
#define MICROS_COUNT_TIMER                    (TIMER1)
// Scheduler END

// VDAC
#define VDAC_PORT                            RGB1_GREEN_PORT
#define VDAC_PIN                             RGB1_GREEN_PIN
// VDAC END

// BUTTON
#define BUTTON_PORT                          gpioPortA
#define BUTTON_PIN                           9
// BUTTON END

// VDAC
#define VDAC_PORT                            RGB1_GREEN_PORT
#define VDAC_PIN                             RGB1_GREEN_PIN
// VDAC END

// RADIO
#define RADIO_PAYLOAD_SIZE                   260 //Same as radio.config payload size
#define RADIO_PACKET_HEADER_SIZE             4
#define RADIO_PACKET_DATA_SIZE               (RADIO_PAYLOAD_SIZE - RADIO_PACKET_HEADER_SIZE)
#define RADIO_PACKET_DATA_SIZE_PER_CHANNEL   (RADIO_PACKET_DATA_SIZE/2)
#define RADIO_FIFO_SIZE                      512 //Must be power of 2 and greater than RADIO_PAYLOAD_SIZE
#define NUMBER_OF_CHANNELS                   20
// RADIO END

typedef enum {
    LEFT,
    RIGHT
} audio_channel_side_t;

// #define LEFT                  (false)
// #define RIGHT                 (true)

#endif /* HARDWARE_CONFIG_H_ */
