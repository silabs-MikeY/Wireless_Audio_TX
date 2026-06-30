#ifndef ADC_H_
#define ADC_H_

#include "hardware_config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// #include "radio.h"


bool adc__get_new_data_ready_flag(uint32_t** new_data_pointer_return);
void adc__init(void);
void adc__deinit(void);
bool adc__get_audio_stereo_flag(void);
bool adc__get_new_packet_ready_for_processing(uint8_t** data_pointer);

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

#define BUFFER_SIZE       RADIO_PACKET_DATA_SIZE
#define BAUD_RATE         (SAMPLE_FREQ * 64)

#endif /* ADC_H_ */
