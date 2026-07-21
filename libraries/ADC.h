#ifndef ADC_H_
#define ADC_H_

#include "hardware_config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// -----------------------------------------------------------------------------
//                     ADC General
// -----------------------------------------------------------------------------

bool adc__init(bool is_stereo, unsigned int ldma_channel_left,
               unsigned int ldma_channel_right);
void adc__deinit(void);
bool adc__get_audio_stereo_flag(void);
uint32_t adc__get_microsecond_ticks(void);
uint32_t adc__get_sample_size_bytes(void);
bool adc__is_sample_little_endian(void);
bool adc__is_sample_left_justified(void);
void adc__printf(bool add_timestamp, const char *format, ...);

// -----------------------------------------------------------------------------
//                     ADC General End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     DMA Buffer Access
// -----------------------------------------------------------------------------

bool adc__get_oldest_left_dma_buffer(uint8_t **buffer, uint32_t *buffer_index,
                                     uint32_t *buffer_size);
bool adc__get_oldest_right_dma_buffer(uint8_t **buffer, uint32_t *buffer_index,
                                      uint32_t *buffer_size);
void adc__mark_left_dma_buffer_stale(uint32_t buffer_index);
void adc__mark_right_dma_buffer_stale(uint32_t buffer_index);

// -----------------------------------------------------------------------------
//                     DMA Buffer Access End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Exported Counters
// -----------------------------------------------------------------------------

uint32_t adc__get_number_of_counters(void);
const char* adc__get_counter_name(uint32_t counter_index);
volatile uint32_t* adc__get_counter_address(uint32_t counter_index);
void adc__reset_counters(void);

// -----------------------------------------------------------------------------
//                     Exported Counters End
// -----------------------------------------------------------------------------

#define I2S_CLK_PIN       I2S_BCLK_PIN
#define I2S_CLK_PORT      I2S_BCLK_PORT
#define I2S_CS_PIN        I2S_LRCLK_PIN
#define I2S_CS_PORT       I2S_LRCLK_PORT
#define I2S_DATAIN_PIN    I2S_DATA_PIN
#define I2S_DATAIN_PORT   I2S_DATA_PORT
#define I2S_DATAOUT_PIN   I2S_DUMMY_TX_DEBUG_PIN
#define I2S_DATAOUT_PORT  I2S_DUMMY_TX_DEBUG_PORT

#define I2S_MODE_SLAVE    0
#define I2S_MODE_MASTER   1
#define I2S_MODE          I2S_MODE_SLAVE

// #define BUFFER_SIZE       RADIO_PACKET_DATA_SIZE
#define ADC_BUFFER_SIZE       128
#if ((ADC_BUFFER_SIZE % 4)!=0)
#error "ADC_BUFFER_SIZE must be a multiple of 4!"
#endif

#define ADC_DMA_BUFFER_COUNT 10

#define BAUD_RATE         (SAMPLE_FREQ * 64)

#endif /* ADC_H_ */
