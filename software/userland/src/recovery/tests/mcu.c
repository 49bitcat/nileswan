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
#include <nile/hardware.h>
#include <nile/mcu.h>
#include <nile/mcu/cdc.h>
#include <nile/mcu/eeprom.h>
#include <nile/mcu/protocol.h>
#include <stdint.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <ws/eeprom.h>
#include "config.h"
#include "console.h"
#include "input.h"
#include "strings.h"

static const char __far s_crlf[] = "\r\n";

bool test_mcu_begin(void) {
    console_print(0, s_rebooting_mcu);

    nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    if (!console_print_status(nile_mcu_reset(false))) {
        return false;
    }
    ws_delay_us(NILE_MCU_NATIVE_RESET_TIME_US);
    console_print_newline(0);
    return true;
}

bool test_mcu_usb_cdc_echo(void) {
    console_print_header(s_mcu_usb_cdc_echo);
    if (!test_mcu_begin()) return false;

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

    console_print_newline(0);
    return true;
}

bool test_mcu_accelerometer(void) {
    console_print_header(s_mcu_accel_test);
    if (!test_mcu_begin()) return false;

    wait_for_vblank();
    console_clear();

    // enable 100 Hz polling
    nile_mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x50, 100), NULL, 0);
    nile_mcu_native_recv_cmd(NULL, 0);

    input_wait_clear();
    uint16_t data[3];

    while (true) {
        nile_mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x51, 0), NULL, 0);
        nile_mcu_native_recv_cmd(&data, 6);
        wait_for_vblank();

        console_clear_lines(6, 3);
        console_drawf(0, 6, CONSOLE_FLAG_NO_SERIAL, s_mcu_accel_axis, 'X', data[0]);
        console_drawf(0, 7, CONSOLE_FLAG_NO_SERIAL, s_mcu_accel_axis, 'Y', data[1]);
        console_drawf(0, 8, CONSOLE_FLAG_NO_SERIAL, s_mcu_accel_axis, 'Z', data[2]);

        input_update();
        if (input_pressed) break;
    }

    // disable polling
    nile_mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x50, 0), NULL, 0);
    nile_mcu_native_recv_cmd(NULL, 0);

    input_wait_clear();
    console_clear();

    return true;
}

static bool test_mcu_wait_for_irq(uint16_t mask, const char __far *cmd) {
    wait_for_vblank();
    input_update();

    console_print(0, cmd);
    nile_mcu_native_mcu_reg_read_sync(NILE_MCU_NATIVE_REG_IRQ_STATUS_AUTOACK);
    while (!(nile_mcu_native_mcu_reg_read_sync(NILE_MCU_NATIVE_REG_IRQ_STATUS) & mask)) {
        wait_for_vblank();
        input_update();

        if (input_pressed & WS_KEY_B) {
            console_print_status(false);
            console_print_newline(0);
            return false;
        }
    }

    console_print_status(true);
    console_print_newline(0);
    return true;
}

bool test_mcu_tf_insert_remove(void) {
    console_print_header(s_mcu_accel_test);
    if (!test_mcu_begin()) return false;

    console_print(0, s_press_b_to_abort);
    console_print_newline(0);

    nile_mcu_native_mcu_reg_read_sync(NILE_MCU_NATIVE_REG_IRQ_STATUS_AUTOACK);
    if (!test_mcu_wait_for_irq(NILE_MCU_NATIVE_IRQ_TF_REMOVE, s_tf_card_remove)) return false;
    if (!test_mcu_wait_for_irq(NILE_MCU_NATIVE_IRQ_TF_INSERT, s_tf_card_insert)) return false;

    return true;
}

__attribute__((optimize("-O0")))
bool test_mcu_eeprom(void) {
    int i;
    console_print_header(s_mcu_eeprom_test);
    if (!test_mcu_begin()) return false;

    console_print(0, s_switching_eeprom);
    outportb(IO_NILE_EMU_CNT, (inportb(IO_NILE_EMU_CNT & ~NILE_EMU_EEPROM_MASK)) | NILE_EMU_EEPROM_2KB);
    if (!nile_mcu_native_eeprom_set_mode_sync(NILE_MCU_EEPROM_MODE_M93LC86))
        return console_print_status(false);
    if (!console_print_status(nile_mcu_native_mcu_switch_mode(NILE_MCU_NATIVE_MODE_EEPROM) >= 0))
        return false;
    ws_delay_us(NILE_MCU_NATIVE_MODESWITCH_TIME_US);
    console_print_newline(0);

    console_print(0, s_eeprom_test_write);

    ws_eeprom_handle_t handle = ws_eeprom_handle_cartridge(10);
    ws_eeprom_write_unlock(handle);

    uint16_t expected, actual;
    for (i = 0; i < 1024; i++) {
        expected = (i * 0x5753) ^ 0xFFFF;
        if (!ws_eeprom_write_word(handle, i * 2, expected)) break;
        actual = ws_eeprom_read_word(handle, i * 2);
        if (actual != expected) break;
    }
    if (i < 1024) {
        console_printf(0, s_eeprom_test_failed, 1, i * 2, actual, expected);
        console_print_status(false);
        return false;
    }

    for (i = 0; i < 1024; i++) {
        expected = i * 0x5753;
        if (!ws_eeprom_write_word(handle, i * 2, expected)) break;
        actual = ws_eeprom_read_word(handle, i * 2);
        if (actual != expected) break;
    }
    if (i < 1024) {
        console_printf(0, s_eeprom_test_failed, 2, i * 2, actual, expected);
        console_print_status(false);
        return false;
    }
    console_print_status(true);
    console_print_newline(0);

    if (!test_mcu_begin()) return false;
    console_print(0, s_eeprom_test_check);
    if (!console_print_status(nile_mcu_native_eeprom_get_mode_sync() == NILE_MCU_EEPROM_MODE_M93LC86))
        return false;
    console_print_newline(0);

    console_print(0, s_eeprom_test_read);
    for (i = 0; i < 1024; i++) {
        expected = i * 0x5753;
        if (!nile_mcu_native_eeprom_read_sync(&actual, i, 2)) break;
        if (expected != actual) break;
    }
    if (i < 1024) {
        console_printf(0, s_eeprom_test_failed, 3, i * 2, actual, expected);
        console_print_status(false);
        return false;
    }
    console_print_status(true);
    console_print_newline(0);

    return true;
}

bool test_mcu_save_id(void) {
    if (!test_mcu_begin()) return false;

    uint32_t save_id_expected = 0x12AA5578;
    uint32_t save_id_actual;

    console_print(0, s_save_id_save);
    if (!console_print_status(
        (nile_mcu_native_mcu_set_save_id_sync(NILE_MCU_NATIVE_SAVE_ID_DOMAIN_SRAM2 | NILE_MCU_NATIVE_SAVE_ID_DOMAIN_RTC, save_id_expected) >= 0)
        && (nile_mcu_native_eeprom_set_mode_sync(NILE_MCU_EEPROM_MODE_M93LC86) >= 0)
    )) return false;
    console_print_newline(0);

    if (!test_mcu_begin()) return false;

    console_print(0, s_save_id_load_ram);
    if (!console_print_status(nile_mcu_native_mcu_get_save_id_sync(NILE_MCU_NATIVE_SAVE_ID_DOMAIN_SRAM2, &save_id_actual) >= 0 && save_id_actual == save_id_expected)) return false;
    console_print_newline(0);

    console_print(0, s_save_id_load_rtc);
    if (!console_print_status(nile_mcu_native_mcu_get_save_id_sync(NILE_MCU_NATIVE_SAVE_ID_DOMAIN_RTC, &save_id_actual) >= 0 && save_id_actual == save_id_expected)) return false;
    console_print_newline(0);

    return true;
}
