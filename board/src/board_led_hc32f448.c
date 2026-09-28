#include "board_led.h"

#include "hc32f448.h"

/* The original APP's idle/menu path sets PC13 and resets PH2.  Target
 * observation then confirmed that this produces red, while the former source
 * mapping (which set PH2 for RED) physically produced green:
 *   red:   PC13, active high
 *   green: PH2,  active high
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

#if defined(DAMIAO_DM8009)
void board_led_set_factory_ready_state(void)
{
    /* PC13 and PH2 drive opposite sides of the two-colour LED.  Driving both
     * pins to the same level leaves no voltage across the LED, which is the
     * observed post-boot dark state.  Keep the DM8009 idle/menu indication on
     * the normal red status path after calibration succeeds. */
    board_led_set(BOARD_LED_RED);
}
#endif

void board_led_toggle_fault_indicator(void)
{
    /* main@0x254b8 runs this every 251 outer-loop ticks while fault > 1:
     * red is forced on and green is toggled.  POTRH preserves the phase that
     * the GPIO already has instead of inventing a source-side blink state. */
    CM_GPIO->POSRC = RED_LED_MASK;
    CM_GPIO->POTRH = GREEN_LED_MASK;
    __DSB();
}
