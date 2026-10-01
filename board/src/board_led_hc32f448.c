#include "board_led.h"

#include "hc32f448.h"

/* The original APP's idle/menu red path RMWs PORRC and PORRH; its motor-mode
 * green path RMWs PORRC and POSRH.  Keep those register choices exact even
 * though PC13 and PH2 drive opposite sides of the two-colour LED.
 */
#define RED_LED_MASK   (1U << 13U)
#define GREEN_LED_MASK (1U << 2U)

void board_led_init(void)
{
    /* GPIO protected-register write enable.  Both pins are push-pull digital
     * outputs, initially low so the transition cannot briefly light both. */
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PCRC13 = GPIO_PCR_POUTE;
    CM_GPIO->PCRH2 = GPIO_PCR_POUTE;
    CM_GPIO->PWPR = 0xA500U;

    board_led_set(BOARD_LED_OFF);
}

void board_led_set(BoardLedColor color)
{
#if defined(DAMIAO_DM4310)
    /* main motor-mode/menu exits use two halfword read-modify-writes,
     * not a three-write off-then-on sequence or a trailing barrier. */
    if (color == BOARD_LED_GREEN) {
        CM_GPIO->PORRC |= RED_LED_MASK;
        CM_GPIO->POSRH |= GREEN_LED_MASK;
        return;
    }
    if (color == BOARD_LED_RED) {
        CM_GPIO->PORRC |= RED_LED_MASK;
        CM_GPIO->PORRH |= GREEN_LED_MASK;
        return;
    }
#endif
    /* Switch the currently selected color off before enabling the other one. */
    CM_GPIO->PORRC = RED_LED_MASK;
    CM_GPIO->PORRH = GREEN_LED_MASK;

    if (color == BOARD_LED_GREEN) {
        CM_GPIO->POSRH = GREEN_LED_MASK;
    } else if (color == BOARD_LED_RED) {
        CM_GPIO->POSRC = RED_LED_MASK;
    }
    __DSB();
}

void board_led_toggle_power_stage_fault_indicator(void)
{
#if defined(DAMIAO_DM4310)
    /* MOSFET diagnostic 0x25740..0x2574e toggles PC13 and sets PH2. */
    CM_GPIO->POTRC |= RED_LED_MASK;
    CM_GPIO->POSRH |= GREEN_LED_MASK;
#else
    board_led_set(BOARD_LED_RED);
#endif
}

void board_led_set_factory_ready_state(void)
{
#if defined(DAMIAO_DM4310)
    /* load_and_validate_calibration@0x22814 performs exactly these two
     * halfword read-modify-writes on its valid-table exit. */
    CM_GPIO->POSRC |= RED_LED_MASK;
    CM_GPIO->PORRH |= GREEN_LED_MASK;
#else
    /* PC13 and PH2 drive opposite sides of the two-colour LED.  Driving both
     * pins to the same level leaves no voltage across the LED, which is the
     * observed post-boot dark state.  Keep the DM8009 idle/menu indication on
     * the normal red status path after calibration succeeds. */
    board_led_set(BOARD_LED_RED);
#endif
}

void board_led_toggle_fault_indicator(void)
{
    /* main@0x254b8 runs this every 251 outer-loop ticks while fault > 1:
     * PC13 is reset and PH2 is toggled.  POTRH preserves the existing phase
     * instead of inventing a source-side blink state. */
#if defined(DAMIAO_DM4310)
    CM_GPIO->PORRC |= RED_LED_MASK;
    CM_GPIO->POTRH |= GREEN_LED_MASK;
#else
    CM_GPIO->POSRC = RED_LED_MASK;
    CM_GPIO->POTRH = GREEN_LED_MASK;
    __DSB();
#endif
}
