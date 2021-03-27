/*
 * (c) 2015 flabberast <s3+flabbergast@sdfeu.org>
 *
 * Based on the following work:
 *  - Guillaume Duc's raw hid example (MIT License)
 *    https://github.com/guiduc/usb-hid-chibios-example
 *  - PJRC Teensy examples (MIT License)
 *    https://www.pjrc.com/teensy/usb_keyboard.html
 *  - hasu's TMK keyboard code (GPL v2 and some code Modified BSD)
 *    https://github.com/tmk/tmk_keyboard/
 *  - ChibiOS demo code (Apache 2.0 License)
 *    http://www.chibios.org
 *
 * Since some GPL'd code is used, this work is licensed under
 * GPL v2 or later.
 */

#include "ch.h"
#include "hal.h"

#include "usb_main.h"

/* TMK includes */
#include "report.h"
#include "host.h"
#include "host_driver.h"
#include "keyboard.h"
#include "action.h"
#include "action_util.h"
#include "mousekey.h"
#include "led.h"
#include "sendchar.h"
#include "debug.h"
#include "printf.h"

#include "chprintf.h" // ChibiOS

#ifndef EARLY_INIT_PERFORM_BOOTLOADER_JUMP
// Change this to be TRUE once we've migrated keyboards to the new init system
#    define EARLY_INIT_PERFORM_BOOTLOADER_JUMP FALSE
#endif

#ifdef SLEEP_LED_ENABLE
#    include "sleep_led.h"
#endif
#ifdef SERIAL_LINK_ENABLE
#    include "serial_link/system/serial_link.h"
#endif
#ifdef VISUALIZER_ENABLE
#    include "visualizer/visualizer.h"
#endif
#ifdef MIDI_ENABLE
#    include "qmk_midi.h"
#endif
#ifdef STM32_EEPROM_ENABLE
#    include "eeprom_stm32.h"
#endif
#include "suspend.h"
#include "wait.h"

#define printf_debug_init()
#define printf_debug(...)

/* -------------------------
 *   TMK host driver defs
 * -------------------------
 */

/* declarations */
uint8_t keyboard_leds(void);
void    send_keyboard(report_keyboard_t *report);
void    send_mouse(report_mouse_t *report);
void    send_system(uint16_t data);
void    send_consumer(uint16_t data);

/* host struct */
host_driver_t chibios_driver = {keyboard_leds, send_keyboard, send_mouse, send_system, send_consumer};

#ifdef VIRTSER_ENABLE
void virtser_task(void);
#endif

#ifdef RAW_ENABLE
void raw_hid_task(void);
#endif

#ifdef CONSOLE_ENABLE
void console_task(void);
#endif
#ifdef MIDI_ENABLE
void midi_ep_task(void);
#endif

/* TESTING
 * Amber LED blinker thread, times are in milliseconds.
 */
/* set this variable to non-zero anywhere to blink once */
// static THD_WORKING_AREA(waThread1, 128);
// static THD_FUNCTION(Thread1, arg) {

//   (void)arg;
//   chRegSetThreadName("blinker");
//   while (true) {
//     systime_t time;

//     time = USB_DRIVER.state == USB_ACTIVE ? 250 : 500;
//     palClearLine(LINE_CAPS_LOCK);
//     chSysPolledDelayX(MS2RTC(STM32_HCLK, time));
//     palSetLine(LINE_CAPS_LOCK);
//     chSysPolledDelayX(MS2RTC(STM32_HCLK, time));
//   }
// }

/* Early initialisation
 */
__attribute__((weak)) void early_hardware_init_pre(void) {
#if EARLY_INIT_PERFORM_BOOTLOADER_JUMP
    void enter_bootloader_mode_if_requested(void);
    enter_bootloader_mode_if_requested();
#endif  // EARLY_INIT_PERFORM_BOOTLOADER_JUMP
}

__attribute__((weak)) void early_hardware_init_post(void) {}

__attribute__((weak)) void board_init(void) {}

// This overrides what's normally in ChibiOS board definitions
void __early_init(void) {
    early_hardware_init_pre();

    // This is the renamed equivalent of __early_init in the board.c file
    void __chibios_override___early_init(void);
    __chibios_override___early_init();

    early_hardware_init_post();
}

// This overrides what's normally in ChibiOS board definitions
void boardInit(void) {
    // This is the renamed equivalent of boardInit in the board.c file
    void __chibios_override_boardInit(void);
    __chibios_override_boardInit();

    board_init();
}

static inline void IOMUXC_SetPinMux(uint32_t muxRegister, uint32_t muxMode, uint32_t inputRegister, uint32_t inputDaisy, uint32_t configRegister, uint32_t inputOnfield) {
    printf_debug("val = %x\n", IOMUXC_SW_MUX_CTL_PAD_MUX_MODE(muxMode) | IOMUXC_SW_MUX_CTL_PAD_SION(inputOnfield));
    *((volatile uint32_t *)muxRegister) = IOMUXC_SW_MUX_CTL_PAD_MUX_MODE(muxMode) | IOMUXC_SW_MUX_CTL_PAD_SION(inputOnfield);

    if (inputRegister) {
        *((volatile uint32_t *)inputRegister) = inputDaisy;
    }
}

extern uint8_t bit_by_index[];
extern uint8_t SW_MUX_CTL_PAD_by_index[];
extern uint8_t SW_PAD_CTL_PAD_by_index[];

void my_setpadmode(ioportid_t port, uint8_t pad, iomode_t mode) {
    // 42000000 IMXRT_GPIO6
    printf_debug("my_setpadmode port=%x, pad=%d, mode=%d)\n", port, pad, mode);
    osalDbgAssert(pad < PADS_PER_PORT, "pal_lld_setpadmode() #1, invalid pad");

    // see IMXRT1060RM Section 12.4.3 GPIO programming

    // Interrupt Mask Register (IMR)
    port->IMR &= ~((uint32_t)1 << bit_by_index[pad]);

    // All GPIOs are on mode ALT5 as per Chapter 10, External Signals and Pin Multiplexing, Table 10-1
    const int altMode = 5;

    // 0000 10B0 = 1000010110000b
    //                         ^ slew rate
    //                       ^^ reserved
    //                    ^^^ drive strength = DSE_6
    //                  ^^ speed = medium
    //               ^^^ reserved
    //              ^ open drain disabled
    //             ^ pull/keep enable
    printf_debug("padctl = %x\n", IOMUXC->SW_PAD_CTL_PAD[SW_PAD_CTL_PAD_by_index[pad]]);

    switch (mode) {
        case PAL_MODE_RESET:
        case PAL_MODE_INPUT:
        case PAL_MODE_OUTPUT_PUSHPULL:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(altMode);
            // need to always set PAD_CTL in case the pin was configured as input before
            IOMUXC->SW_PAD_CTL_PAD[SW_PAD_CTL_PAD_by_index[pad]] = IOMUXC_SW_PAD_CTL_PAD_DSE(6);
            break;

        case PAL_MODE_OUTPUT_OPENDRAIN:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(altMode);
            // IOMUXC->SW_PAD_CTL_PAD[SW_PAD_CTL_PAD_by_index[pad]] =
            //  IOMUXC_SW_PAD_CTL_PAD_ODE(1); /* Open Drain Enable */
            break;

        case PAL_MODE_INPUT_PULLUP:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(altMode);

            // IOMUXC->SW_PAD_CTL_PAD[SW_PAD_CTL_PAD_by_index[pad]] =
            //  IOMUXC_SW_PAD_CTL_PAD_PKE(1) | /* Pull/Keep Enable */
            //  IOMUXC_SW_PAD_CTL_PAD_PUS(1) | /* Pull Up/Down Config: 47k pull up */
            //  IOMUXC_SW_PAD_CTL_PAD_PUE(1); /* Pull/Keep Select: pull */

            break;

        case PAL_MODE_INPUT_PULLDOWN:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(altMode);

            IOMUXC->SW_PAD_CTL_PAD[SW_PAD_CTL_PAD_by_index[pad]] = IOMUXC_SW_PAD_CTL_PAD_PKE(1) | /* Pull/Keep Enable */
                                                                   IOMUXC_SW_PAD_CTL_PAD_PUS(0) | /* Pull Up/Down Config: 100k pull dn */
                                                                   IOMUXC_SW_PAD_CTL_PAD_PUE(1);  /* Pull/Keep Select: pull */
            break;

        case PAL_MODE_UNCONNECTED:
        case PAL_MODE_INPUT_ANALOG:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(altMode);
            break;

        case PAL_MODE_ALTERNATIVE_1:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(1);
            break;

        case PAL_MODE_ALTERNATIVE_2:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(2);
            break;

        case PAL_MODE_ALTERNATIVE_3:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(3);
            break;

        case PAL_MODE_ALTERNATIVE_4:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(4);
            break;

        case PAL_MODE_ALTERNATIVE_5:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(5);
            break;

        case PAL_MODE_ALTERNATIVE_6:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(6);
            break;

        case PAL_MODE_ALTERNATIVE_7:
            IOMUXC->SW_MUX_CTL_PAD[SW_MUX_CTL_PAD_by_index[pad]] = PIN_MUX_ALTERNATIVE(7);
            break;
    }

    // GPIO direction register (GDIR)
    if (mode == PAL_MODE_OUTPUT_PUSHPULL)
        port->GDIR |= ((uint32_t)1 << bit_by_index[pad]);
    else
        port->GDIR &= ~((uint32_t)1 << bit_by_index[pad]);
}

#if 0
// from the linker
extern unsigned long _stextload;
extern unsigned long _stext;
extern unsigned long _etext;
extern unsigned long _sdataload;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _flexram_bank_config;
extern unsigned long _estack;
extern unsigned long _heap_start;
extern unsigned long _heap_end;

__attribute__((optimize("O0"))) static void verify_memory_regions(void) {
    /* ITCM (minimal, nur 1 bank, d.h. nur 32 KB): 0x0000000 bis 32KB */
    uint8_t *itcm = 0x00000000 + 32 /* MPU to trap NULL pointer */;
    uint8_t  csum = 0;
    for (uint8_t *w = itcm; w < itcm + (32 * 1024) - 32; w++) {
        csum += *w;
    }
    printf_debug("itcm csum = %x\n", csum);

    // TODO: need to turn off the stack overflow MPU trap for this:
    /* DTCM (rest, d.h. 512 KB - 32 KB): 0x20000000 bis 0x20000000 + (512 KB - 32 KB) */
    uint8_t *dtcm = 0x20000000;
    csum          = 0;
    for (uint8_t *w = dtcm; w < dtcm + (512 * 1024) - (32 * 1024); w++) {
        csum += *w;
    }
    printf_debug("dtcm csum = %x\n", csum);

    /* RAM: 0x20200000 bis 0x20200000 + 512 KB */
    uint8_t *ram = 0x20200000;
    csum         = 0;
    for (uint8_t *w = ram; w < ram + (512 * 1024); w++) {
        csum += *w;
    }
    printf_debug("ram csum = %x\n", csum);
}
#endif

extern void delay(const uint32_t cycles);

char buf[1024];

#include "fsl_lpuart.h"
#undef printf_debug
void printf_debug(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int n = chvsnprintf(buf, sizeof(buf), format, args);
  // Directly write to serial instead of using SD4 BaseSequentialStream, because
  // the latter does not work from within a locked section (e.g. usbStart grabs
  // a lock).
  buf[n] = '\r';
  buf[n+1] = '\n';
  buf[n+2] = '\0';
  LPUART_WriteBlocking(LPUART1, (unsigned char*)buf, n+2);
  va_end(args);
}

/* Main thread
 */
int main(void) {
    /* ChibiOS/RT init */
    halInit();
    chSysInit();

    const SerialConfig sc = {
      .sc_speed = 115200,
    };
    sdStart(&SD1, &sc);
    sdStart(&SD3, &sc);
    chprintf((BaseSequentialStream*)&SD1, "hello from chprintf on the SD1 serial!\r\n");
    chprintf((BaseSequentialStream*)&SD3, "hello from chprintf on the SD4 console!\r\n");
    
    printf_debug_init();
    printf_debug("printf_debug\r\n");
    printf("printf\r\n");

#ifdef STM32_EEPROM_ENABLE
    EEPROM_Init();
#endif

#if 0 /* Button test program */
    // set row pin as output
    // write row pin low

    ioportid_t col_line = LINE_PIN21;
    palSetLineMode(col_line, PAL_MODE_INPUT_PULLUP); // setPinInputHigh
    for (;;) {
        printf_debug("readLine = %d\n", palReadLine(col_line));    // high
        delay(600000000);          // 1s
    }

#endif

    // ROW_F2
    // #define MATRIX_ROW_PINS {LINE_PIN21}
    // COL_3
    // #define MATRIX_COL_PINS {LINE_PIN20}

#if 0  /* GPIO test program */
    ioportid_t line = LINE_PIN21;
    palSetLineMode(line, PAL_MODE_OUTPUT_PUSHPULL);
    for (;;) {
        palSetLine(line);    // high
        delay(600000000);          // 1s
        palClearLine(line);  // low
        delay(600000000);          // 1s
    }
#endif /* LED_HACK */

#if 0 /* GPIO connection test program */
    ioportid_t write_line = LINE_PIN21;
    ioportid_t read_line = LINE_PIN20;
    palSetLineMode(write_line, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(read_line, PAL_MODE_INPUT_PULLUP);
    for (;;) {
        printf_debug("\n\nsetting high\n");
        palSetLine(write_line);    // high
        printf_debug("readLine = %d\n", palReadLine(read_line));
        delay(600000000);          // 1s
        printf_debug("readLine = %d\n", palReadLine(read_line));
        printf_debug("setting low\n");
        palClearLine(write_line);  // low
        printf_debug("readLine = %d\n", palReadLine(read_line));
        delay(600000000);          // 1s
        printf_debug("readLine = %d\n", palReadLine(read_line));
    }
#endif

#if 0 /* matrix scan test program */
    ioportid_t write_line = LINE_PIN21;
    ioportid_t read_line = LINE_PIN20;
    palSetLineMode(write_line, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(read_line, PAL_MODE_INPUT_PULLUP);
    for (;;) {
        printf_debug("\n\nselect row\n");
        palSetLineMode(write_line, PAL_MODE_OUTPUT_PUSHPULL);
        palClearLine(write_line);
        printf_debug("readLine1 = %d\n", palReadLine(read_line));
        printf_debug("readLine2 = %d\n", palReadLine(read_line));
        palSetLine(write_line);
        //my_setpadmode(PAL_PORT(write_line), PAL_PAD(write_line), PAL_MODE_INPUT_PULLUP);

        palSetLineMode(write_line, PAL_MODE_INPUT_PULLUP);
        delay(600000000);          // 1s
    }
#endif

    // TESTING
    // chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    keyboard_setup();

    /* Init USB */
    printf_debug("init_usb_driver() before, addr = %x\r\n", &USB_DRIVER);
    init_usb_driver(&USB_DRIVER);
    printf_debug("init_usb_driver() after\r\n");

#ifdef MIDI_ENABLE
    setup_midi();
#endif

#ifdef SERIAL_LINK_ENABLE
    init_serial_link();
#endif

#ifdef VISUALIZER_ENABLE
    visualizer_init();
#endif

    host_driver_t *driver = NULL;

    /* Wait until the USB or serial link is active */
    while (true) {
#if defined(WAIT_FOR_USB) || defined(SERIAL_LINK_ENABLE)
        if (USB_DRIVER.state == USB_ACTIVE) {
            driver = &chibios_driver;
            break;
        }
#else
        driver = &chibios_driver;
        break;
#endif
#ifdef SERIAL_LINK_ENABLE
        if (is_serial_link_connected()) {
            driver = get_serial_link_driver();
            break;
        }
        serial_link_update();
#endif
        wait_ms(50);
    }

    /* Do need to wait here!
     * Otherwise the next print might start a transfer on console EP
     * before the USB is completely ready, which sometimes causes
     * HardFaults.
     */
    wait_ms(50);

    print("USB configured.\n");

    /* init TMK modules */
    keyboard_init();
    host_set_driver(driver);

#ifdef SLEEP_LED_ENABLE
    sleep_led_init();
#endif

    print("Keyboard start.\n");

    /* Main loop */
    while (true) {
#if !defined(NO_USB_STARTUP_CHECK)
        if (USB_DRIVER.state == USB_SUSPENDED) {
            print("[s]");
#    ifdef VISUALIZER_ENABLE
            visualizer_suspend();
#    endif
            while (USB_DRIVER.state == USB_SUSPENDED) {
                /* Do this in the suspended state */
#    ifdef SERIAL_LINK_ENABLE
                serial_link_update();
#    endif
                suspend_power_down();  // on AVR this deep sleeps for 15ms
                /* Remote wakeup */
                if (suspend_wakeup_condition()) {
                    usbWakeupHost(&USB_DRIVER);
                }
            }
            /* Woken up */
            // variables has been already cleared by the wakeup hook
            send_keyboard_report();
#    ifdef MOUSEKEY_ENABLE
            mousekey_send();
#    endif /* MOUSEKEY_ENABLE */

#    ifdef VISUALIZER_ENABLE
            visualizer_resume();
#    endif
        }
#endif

        keyboard_task();
        // delay(600000000);          // 1s

#ifdef CONSOLE_ENABLE
        console_task();
#endif
#ifdef MIDI_ENABLE
        midi_ep_task();
#endif
#ifdef VIRTSER_ENABLE
        virtser_task();
#endif
#ifdef RAW_ENABLE
        raw_hid_task();
#endif
    }
}
