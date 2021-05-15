#ifndef STAPELBERG_CONFIG_H
#define STAPELBERG_CONFIG_H

#include "../config.h"
#include "config_common.h"

/* USB Device descriptor parameter */
#define PRODUCT_ID 0x6060
#define DEVICE_VER 0x0002

/* key matrix size */
#define MATRIX_ROWS 15
#define MATRIX_COLS 7

/*
 * Keyboard Matrix Assignments
 *
 * Change this to how you wired your keyboard
 * COLS: AVR pins used for columns, left to right
 * ROWS: AVR pins used for rows, top to bottom
 * DIODE_DIRECTION: COL2ROW = COL = Anode (+), ROW = Cathode (-, marked on diode)
 *                  ROW2COL = ROW = Anode (+), COL = Cathode (-, marked on diode)
 *
 */
// clang-format off
#define MATRIX_ROW_PINS {				\
			 D0, /* tastenlinks1g, row_eql */	\
			 D1, /* links1f, row_1 */		\
			 D2, /* links1e, row_2 */		\
			 D3, /* links1c, row_3 */		\
			 D4, /* links1b, row_4 */		\
			 D5, /* links1a, row_5 */		\
			 D6, /* rechts1m, row_6 */		\
			 D7, /* rechts1b, row_7 */		\
			 C0, /* rechts1a, row_8 */		\
			 C1, /* rechts1k, row_9 */		\
			 C2, /* rechts1l, row_0 */		\
			 C3, /* rechts1e, row_min */		\
			 C4, /* rechts1h / links1a, row_esc */	\
			 C5, /* links1l, links1e, links1b, rechts1l, rechts1e, rechts1b, row_f1 */		\
			 C6, /* links1j, links1f, links1c, rechts1k, rechts1f, rechts1c, row_f2 */		\
  }
#define MATRIX_COL_PINS { \
			 B0, /* links1m, col0 */	\
			 B1, /* links1h, col1 */	\
			 B2, /*links1j, col2 */	\
			 B3, /* rechts1g, col3 */	\
			 B4, /* links1l, col4 */	\
			 B5, /* rechts1j, rechts1l, col5 */	\
			 B6, /* col6 */		\
  }
#define UNUSED_PINS
// clang-format on

/* COL2ROW or ROW2COL */
#define DIODE_DIRECTION COL2ROW

#define LED_NUM_LOCK_PIN F2
#define LED_CAPS_LOCK_PIN F3
#define LED_SCROLL_LOCK_PIN F1
#define LED_COMPOSE_PIN F0
#define LED_PIN_ON_STATE 0

/* Debounce reduces chatter (unintended double-presses) - set 0 if debouncing is not needed */
#define DEBOUNCE 5

/* don't know if this should be defined at the board or top level. Assuming board
#define MOUSEKEY_DELAY          100
#define MOUSEKEY_INTERVAL       20
#define MOUSEKEY_MAX_SPEED      3
#define MOUSEKEY_TIME_TO_MAX    10
*/

#define IGNORE_MOD_TAP_INTERRUPT

#endif
