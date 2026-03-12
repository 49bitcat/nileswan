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
#include <nile/mcu/protocol.h>
#include <stdint.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include "config.h"
#include "console.h"
#include "input.h"
#include "strings.h"

bool test_mcu_begin(void);

bool op_mcu_status_query(void) {
    console_print_header(s_mcu_status_query);
    if (!test_mcu_begin()) return false;

    wait_for_vblank();
    console_clear();
    input_wait_clear();

    while (true) {
        nile_mcu_native_info_t info;
        
        nile_mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(NILE_MCU_NATIVE_CMD_INFO, 0), NULL, 0);
        nile_mcu_native_recv_cmd(&info, sizeof(info));
        wait_for_vblank();

        console_clear_lines(6, 3);
        console_drawf(0, 6, CONSOLE_FLAG_NO_SERIAL, s_mcu_status_query_caps, info.caps);
        console_drawf(0, 7, CONSOLE_FLAG_NO_SERIAL, s_mcu_status_query_status, info.status);
        console_drawf(0, 8, CONSOLE_FLAG_NO_SERIAL, s_mcu_status_query_voltage, info.bat_voltage);

        for (int i = 0; i < 8; i++)
            wait_for_vblank();

        input_update();
        if (input_pressed) break;
    }

    input_wait_clear();
    console_clear();

    return true;
}
