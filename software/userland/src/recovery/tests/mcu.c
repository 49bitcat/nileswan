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

#include "mcu.h"
#include <nile.h>
#include <nile/mcu.h>
#include <nile/mcu/cdc.h>
#include <stdint.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include "config.h"
#include "console.h"
#include "input.h"
#include "strings.h"

static const char __far s_crlf[] = "\r\n";

bool test_mcu_usb_cdc_echo(void) {
    console_print_header(s_mcu_usb_cdc_echo);
    console_print(0, s_rebooting_mcu);

    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    if (!console_print_status(nile_mcu_reset(false))) {
        return false;
    }
    ws_delay_us(NILE_MCU_RESET_TIME_US);
    console_print_newline();

    char c[2];
    c[1] = 0;

    input_wait_clear();
    while (true) {
        if (nile_mcu_native_cdc_read_sync(c, 1) > 0) {
            if (c[0] >= 32 && c[0] <= 126) {
                nile_mcu_native_cdc_write_async_start(c, 1);
                console_print(CONSOLE_FLAG_NO_SERIAL, c);
            } else if (c[0] == 13) {
                nile_mcu_native_cdc_write_async_start(s_crlf, 2);
                console_draw_newline();
            }
            nile_mcu_native_cdc_write_async_finish();
        }

        input_update();
        if (input_pressed) break;
    }

    console_print_newline();
    return true;
}