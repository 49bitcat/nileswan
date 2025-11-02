/**
 * Copyright (c) 2024, 2025 Adrian "asie" Siekierka
 *
 * Nileswan Userland is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan Userland is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan Userland. If not, see <https://www.gnu.org/licenses/>.
 */

#include "rtc.h"
#include <nile.h>
#include <nile/mcu.h>
#include <stdint.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include "config.h"
#include "console.h"
#include "input.h"
#include "strings.h"

int32_t fetch_rtc_time(uint8_t cmd);

static bool test_rtc_stability_run(void) {
    int32_t first_time = fetch_rtc_time(0x17);
    if (first_time < 0) {
        console_printf(0, s_rtc_stability_read_failed, -1, (int16_t) first_time);
        console_print_status(false);
        console_print_newline(0);
        return false;
    }
    if (first_time & 0x408080) {
        console_printf(0, s_rtc_stability_value_invalid, -1, first_time);
        console_print_status(false);
        console_print_newline(0);
        return false;
    }
    for (uint16_t i = 0; i < 3456; i++) {
        // Fetch RTC time
        uint8_t cmd = (i & 1) ? 0x15 : 0x17;
        int32_t next_time = fetch_rtc_time(cmd);
        if (next_time < 0) {
            console_printf(0, s_rtc_stability_read_failed, i, (int16_t) next_time);
            console_print_status(false);
            console_print_newline(0);
            return false;
        }
        if (next_time & 0xFF408080) {
            console_printf(0, s_rtc_stability_value_invalid, i, next_time);
            console_print_status(false);
            console_print_newline(0);
            return false;
        }
        if (next_time < first_time && next_time) {
            console_printf(0, s_rtc_stability_value_mismatch, i, first_time, next_time);
            console_print_status(false);
            console_print_newline(0);
            return false;
        }
        first_time = next_time;
    }
    return true;
}

/**
 * Reboot the MCU and initialize a clean RTC slate.
 */
static bool rtc_reset_mcu_init(void) {
    console_print(0, s_rebooting_mcu);

    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    if (!console_print_status(nile_mcu_reset(false))) {
        return false;
    }
    ws_delay_us(NILE_MCU_RESET_TIME_US);
    console_print_newline(0);

    console_print(0, s_switching_rtc);

    // LSE clock may require up to a second to initialize
    ws_delay_ms(1000);

    // FIXME: An MCU code bug requires at least one other command to be sent before the "set mode" command.
    if (nile_mcu_native_cdc_available_sync() < 0) {
        return console_print_status(false);
    }
    // END

    if (!console_print_status(nile_mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x01, 0x0002), NULL, 0) >= 0)) {
        return false;
    }
    ws_delay_us(NILE_MCU_MODESWITCH_TIME_US);
    console_print_newline(0);

    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    console_print(0, s_resetting_rtc);
    if (!ws_cart_rtc_reset()) {
        return console_print_status(false);
    }
    if (!ws_cart_rtc_wait_ready()) {
        return console_print_status(false);
    }

    return console_print_status(true);
}

bool test_rtc_stability(uint32_t runs) {
    console_print_header(s_rtc_stability_test);
    if (!rtc_reset_mcu_init()) return false;
    console_print_newline(0);

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
    console_print_newline(0);
    return result;
}

static bool wait_tick(ws_cart_rtc_time_t *time) {
    ws_cart_rtc_time_t compared;
    int timeout = 0;
    while (--timeout) {
        if (!ws_cart_rtc_read_time(&compared)) {
            break;
        }

/*
        console_print_newline(0);
        console_printf(0, "read %02X:%02X:%02X", compared.hour, compared.minute, compared.second);
*/

        if (memcmp(time, &compared, sizeof(ws_cart_rtc_time_t))) {
            memcpy(time, &compared, sizeof(ws_cart_rtc_time_t));
            return true;
        }
    }
    return false;
}

#define RTC_TICKS_EXPECTED 12000
#define RTC_TICKS_MIN ((uint32_t)(RTC_TICKS_EXPECTED) * 100 / CONFIG_RTC_TOLERANCE)
#define RTC_TICKS_MAX ((uint32_t)(RTC_TICKS_EXPECTED) * CONFIG_RTC_TOLERANCE / 100)

bool test_rtc_clock(void) {
    console_print_header(s_rtc_clock_test);
    if (!rtc_reset_mcu_init()) return false;
    console_print_newline(0);

    // Test RTC clock reliability
    console_print(0, s_verifying_time_change);
    ws_cart_rtc_time_t initial;
    if (!ws_cart_rtc_read_time(&initial)) {
        return console_print_status(false);
    }

    if (!wait_tick(&initial)) {
        return console_print_status(false);
    }
    ws_timer_hblank_start_once(65535);
    if (!wait_tick(&initial)) {
        return console_print_status(false);
    }
    uint16_t ticks = inportw(WS_TIMER_HBL_COUNTER_PORT) ^ 65535;
    ws_timer_hblank_disable();

    if (ticks >= RTC_TICKS_MIN && ticks <= RTC_TICKS_MAX) {
        console_printf(CONSOLE_FLAG_RIGHT | CONSOLE_FLAG_HIGHLIGHT, s_d, ticks);
        console_print_newline(0);
    } else {
        console_print(0, s_out_of_range);
        console_printf(CONSOLE_FLAG_RIGHT | CONSOLE_FLAG_HIGHLIGHT, s_d, ticks);
        return false;
    }

    // Test standard RTC read/write
    ws_cart_rtc_datetime_t dt, dt_read;
    dt.date.year = 0x23;
    dt.date.month = 0x08;
    dt.date.day = 0x05;
    dt.date.wday = 0x06;
    dt.time.hour = 0x11 | WS_CART_RTC_HOUR_PM;
    dt.time.minute = 0x45;
    dt.time.second = 0x52;

    console_print(0, s_setting_rtc_time);
    if (!console_print_status(ws_cart_rtc_write_datetime(&dt))) {
        return false;
    }
    console_print_newline(0);
    console_print(0, s_verifying_time_change);
    if (!ws_cart_rtc_read_datetime(&dt_read)) {
        return console_print_status(false);
    }
    if (memcmp(&dt, &dt_read, sizeof(ws_cart_rtc_datetime_t))) {
        return console_print_status(false);
    }
    console_print_status(true);
    console_print_newline(0);

    // TODO: Test date/time edge cases
    
    return true;
}
