#include "rtc.h"
#include <nile.h>
#include <nile/mcu.h>
#include <stdint.h>
#include <wonderful.h>
#include <ws/hardware.h>
#include <ws/util.h>
#include "console.h"
#include "input.h"
#include "strings.h"

int32_t fetch_rtc_time(uint8_t cmd);

static bool test_rtc_stability_run(void) {
    int32_t first_time = fetch_rtc_time(0x17);
    if (first_time < 0) {
        console_printf(0, s_rtc_stability_read_failed, -1, (int16_t) first_time);
        console_print_status(false);
        console_print_newline();
        return false;
    }
    if (first_time & 0x408080) {
        console_printf(0, s_rtc_stability_value_invalid, -1, first_time);
        console_print_status(false);
        console_print_newline();
        return false;
    }
    for (uint16_t i = 0; i < 3456; i++) {
        // Fetch RTC time
        uint8_t cmd = (i & 1) ? 0x15 : 0x17;
        int32_t next_time = fetch_rtc_time(cmd);
        if (next_time < 0) {
            console_printf(0, s_rtc_stability_read_failed, i, (int16_t) next_time);
            console_print_status(false);
            console_print_newline();
            return false;
        }
        if (next_time & 0xFF408080) {
            console_printf(0, s_rtc_stability_value_invalid, i, next_time);
            console_print_status(false);
            console_print_newline();
            return false;
        }
        if (next_time < first_time && next_time) {
            console_printf(0, s_rtc_stability_value_mismatch, i, first_time, next_time);
            console_print_status(false);
            console_print_newline();
            return false;
        }
        first_time = next_time;
    }
    return true;
}

bool test_rtc_stability(uint32_t runs) {
    console_print_header(s_rtc_stability_test);
    console_print(0, s_rebooting_mcu);
    
    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);
    
    if (!nile_mcu_reset(false)) {
        return console_print_status(false);
    }

    ws_busywait(20000);
    if (!nile_mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x01, 0x0002), NULL, 0)) {
        return console_print_status(false);
    }
    console_print_status(true);
    console_print_newline();

    console_print(0, s_resetting_rtc);
    ws_busywait(20000);
    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);
    outportb(IO_CART_RTC_CTRL, 0x10);
    while (inportb(IO_CART_RTC_CTRL) & 0x10);
    console_print_status(true);
    console_print_newline();

    bool result = true;
    console_print(0, s_rtc_stability_test);
    if (!runs) {
        console_print(0, s_hold_b_to_abort);
        while (!(input_pressed & KEY_B)) {
            input_update();
            if (!test_rtc_stability_run()) return false;
            console_putc(0, '.');
        }
        console_putc(0, '.');
    } else {
        while (runs--) {
            if (!test_rtc_stability_run()) return false;
            console_putc(0, '.');
        }
    }

    console_print_status(result);
    console_print_newline();
    return result;
}
