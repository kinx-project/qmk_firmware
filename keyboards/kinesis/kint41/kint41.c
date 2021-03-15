#include "kint41.h"

volatile measurement_t measurement;

// called once when the firmware starts up
void matrix_init_kb(void) {
    // matrix_init_user();
    led_init_ports();
}

#if 0
volatile uint32_t last_scan;
volatile uint32_t freq;

// runs every cycle
void matrix_scan_kb(void) {
    // Measure the scan frequency. Printed when doing latency measurements.
    uint32_t scan = DWT->CYCCNT;
    freq          = (scan - last_scan);
    last_scan     = scan;

    matrix_scan_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    measurement.start = DWT->CYCCNT;
    return process_record_user(keycode, record);
}
#endif

#define ledTurnOff palSetLine
#define ledTurnOn palClearLine

#define LED_KEYPAD LINE_PIN24
#define LED_SCROLL_LOCK LINE_PIN25
#define LED_NUM_LOCK LINE_PIN26
#define LED_CAPS_LOCK LINE_PIN12

void led_init_ports() {
    palSetLineMode(LED_KEYPAD, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(LED_SCROLL_LOCK, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(LED_NUM_LOCK, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(LED_CAPS_LOCK, PAL_MODE_OUTPUT_PUSHPULL);

    ledTurnOff(LED_KEYPAD);
    ledTurnOff(LED_SCROLL_LOCK);
    ledTurnOff(LED_NUM_LOCK);
    ledTurnOff(LED_CAPS_LOCK);

// Turn on the Teensy 3.6 Power LED:
#define LED_POWER LINE_PIN13
    palSetLineMode(LED_POWER, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLine(LED_POWER);
}

void led_set_kb(uint8_t usb_led) {
    // measurement.report = DWT->CYCCNT;

#if 0
    const uint32_t freq_us = (freq * 5.55) / 1000;
    {
        const uint32_t diff_ns      = (measurement.report - measurement.start) * 5.55;
        const uint32_t diff_us      = diff_ns / 1000;
        const uint32_t diff_prev_ns = (measurement.report - measurement.sof) * 5.55;
        const uint32_t diff_prev_us = diff_prev_ns / 1000;
        dprintf("press-to-report=%d us, press-to-usbsof=%d us, matrix-scan-freq=%d us\r\n", diff_us, diff_prev_us, freq_us);
    }
#endif

    if (usb_led & (1 << USB_LED_COMPOSE)) {
        ledTurnOn(LED_KEYPAD);
    } else {
        ledTurnOff(LED_KEYPAD);
    }

    if (usb_led & (1 << USB_LED_SCROLL_LOCK)) {
        ledTurnOn(LED_SCROLL_LOCK);
    } else {
        ledTurnOff(LED_SCROLL_LOCK);
    }

    if (usb_led & (1 << USB_LED_NUM_LOCK)) {
        ledTurnOn(LED_NUM_LOCK);
    } else {
        ledTurnOff(LED_NUM_LOCK);
    }

    if (usb_led & (1 << USB_LED_CAPS_LOCK)) {
        ledTurnOn(LED_CAPS_LOCK);
    } else {
        ledTurnOff(LED_CAPS_LOCK);
    }
}
