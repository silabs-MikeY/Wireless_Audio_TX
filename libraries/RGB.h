#ifndef RGB_H_
#define RGB_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color_t;

static const color_t red = {
    0xFF,
    0x00,
    0x00
};

static const color_t green = {
    0x00,
    0xFF,
    0x00
};

static const color_t blue = {
    0x00,
    0x00,
    0xFF
};

static const color_t yellow = {
    0xFF,
    0xFF,
    0x00
};

static const color_t pink = {
    0xFF,
    0x00,
    0xFF
};

static const color_t teal = {
    0x00,
    0xFF,
    0xFF
};

static const color_t orange = {
    0xFF,
    0x80,
    0x00
};

static const color_t purple = {
    0x7F,
    0x00,
    0xFF
};

static const color_t white = {
    0xFF,
    0xFF,
    0xFF
};

static const color_t black = {
    0x00,
    0x00,
    0x00
};

void rgb__set_rgb_out(uint8_t RGB_number, color_t color, bool clear_other_colors, uint8_t intensity);
bool rgb__timer_init(void);
bool rgb__init(void);
void rgb__deinit(void);
void rgb__start_radio_status_blink(uint32_t current_tick);
void rgb__start_radio_status_blink_force(void);
void rgb__run_radio_status_blink(uint32_t current_tick);
void rgb__radio_blink_turn_on(void);

#endif /* RGB_H_ */
