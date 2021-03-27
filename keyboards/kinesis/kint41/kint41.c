/* Copyright 2020 QMK
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kint41.h"

void matrix_init_kb(void) {
    led_init_ports();
    matrix_init_user();
}

#define ledTurnOff writePinHigh
#define ledTurnOn writePinLow

// TODO: change back to LINE_PIN24
#define LED_KEYPAD LINE_PIN25
#define LED_SCROLL_LOCK LINE_PIN25
#define LED_NUM_LOCK LINE_PIN26
#define LED_CAPS_LOCK LINE_PIN12

void led_init_ports() {
    setPinOutput(LED_KEYPAD);
    setPinOutput(LED_SCROLL_LOCK);
    setPinOutput(LED_NUM_LOCK);
    setPinOutput(LED_CAPS_LOCK);

    ledTurnOff(LED_KEYPAD);
    ledTurnOff(LED_SCROLL_LOCK);
    ledTurnOff(LED_NUM_LOCK);
    ledTurnOff(LED_CAPS_LOCK);

// Turn on the Teensy 3.6 Power LED:
#define LED_POWER LINE_PIN13
    setPinOutput(LED_POWER);
    writePinHigh(LED_POWER);
}

bool led_update_kb(led_t led_state) {
    bool res = led_update_user(led_state);
    if (res) {
        if (led_state.compose) {
            ledTurnOn(LED_KEYPAD);
        } else {
            ledTurnOff(LED_KEYPAD);
        }

        if (led_state.scroll_lock) {
            ledTurnOn(LED_SCROLL_LOCK);
        } else {
            ledTurnOff(LED_SCROLL_LOCK);
        }

        if (led_state.num_lock) {
            ledTurnOn(LED_NUM_LOCK);
        } else {
            ledTurnOff(LED_NUM_LOCK);
        }

        if (led_state.caps_lock) {
            ledTurnOn(LED_CAPS_LOCK);
        } else {
            ledTurnOff(LED_CAPS_LOCK);
        }
    }
    return res;
}
