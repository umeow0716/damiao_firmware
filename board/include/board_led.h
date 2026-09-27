#ifndef DM4310_BOARD_LED_H
#define DM4310_BOARD_LED_H

typedef enum {
    BOARD_LED_OFF = 0,
    BOARD_LED_RED,
    BOARD_LED_GREEN,
} BoardLedColor;

void board_led_init(void);
void board_led_set(BoardLedColor color);
void board_led_toggle_fault_indicator(void);

#endif
