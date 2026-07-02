#ifndef ADC_H_
#define ADC_H_

#include "hardware_config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// #include "radio.h"

bool adc__get_new_data_ready_flag(uint32_t** new_data_pointer_return);
void adc__init(bool is_stereo);
void adc__deinit(void);
bool adc__get_audio_stereo_flag(void);
bool adc__get_new_packet_ready_for_processing(uint8_t** data_pointer);
uint32_t adc__get_sample_size_bytes(void);
bool adc__is_sample_little_endian(void);
bool adc__is_sample_left_justified(void);

// User should implement these functions to process the left and right audio buffers as needed.
void adc__process_left_buffer(uint8_t *buffer);
void adc__process_right_buffer(uint8_t *buffer);

// Optional debug data functions for counters and logging.
uint32_t adc__get_number_of_counters(void);
const char* adc__get_counter_name(uint32_t counter_index);
volatile uint32_t* adc__get_counter_address(uint32_t counter_index);
void adc__reset_counters(void);

void adc__printf(bool add_timestamp, const char *format, ...);

// End of functions for user to implement

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
#define BUFFER_SIZE       64
#if ((BUFFER_SIZE % 4)!=0)
#error "BUFFER_SIZE must be a multiple of 4!"
#endif

#define BAUD_RATE         (SAMPLE_FREQ * 64)

#endif /* ADC_H_ */
