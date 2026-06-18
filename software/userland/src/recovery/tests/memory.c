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

#include "input.h"
#include <nile.h>
#include <nile/mcu/protocol.h>
#include <stdint.h>
#include <wonderful.h>
#include <ws.h>
#include <ws/eeprom.h>
#include "console.h"
#include "strings.h"
#include "util/rand.h"

bool test_mcu_begin(void);

static bool test_memory_sram_write(void) {
	uint16_t value = 1;

	for (uint16_t bank = 0; bank < CONFIG_SRAM_BANKS; bank++) {
		outportw(WS_CART_EXTBANK_RAM_PORT, bank);
		uint16_t offset = 0;
		do {
			value = xorshift_fill_128b(value, MK_FP(0x1000, offset));
			offset += 0x80;
		} while (offset);
		console_putc(0, '.');
	}
	return true;
}

static bool test_memory_sram_read(void) {
	uint8_t buffer[128];
	uint16_t value = 1;
	bool ok = true;

	for (uint16_t bank = 0; bank < CONFIG_SRAM_BANKS; bank++) {
		outportw(WS_CART_EXTBANK_RAM_PORT, bank);
		uint16_t offset = 0;
		do {
			uint8_t __far* cmp_buffer = MK_FP(0x1000, offset);
			value = xorshift_fill_128b(value, buffer);
			for (int i = 0; i < 128; i++) {
				if (buffer[i] != cmp_buffer[i]) {
					console_printf(0, s_sram_read_error, bank, offset + i, buffer[i], cmp_buffer[i]);
					input_wait_any_key();
					ok = false;
					if (input_held & (KEY_Y1|KEY_Y2|KEY_Y3|KEY_Y4)) break;
				}
			}
			offset += 0x80;
		} while (offset);
		console_putc(0, '.');
	}

	return ok;
}

static bool test_memory_eeprom_write(ws_eeprom_handle_t handle) {
	uint8_t buffer[128];
	uint16_t value = 1;

	for (uint16_t pos = 0; pos < 2048; pos += 128) {
		value = xorshift_fill_128b(value, buffer);
		if (nile_mcu_native_eeprom_write_sync(buffer, pos >> 1, 64) < 0) {
			return false;
		}
	}
	return true;
}

static bool test_memory_eeprom_read(ws_eeprom_handle_t handle) {
	uint8_t buffer[128];
	uint8_t cmp_buffer[128];
	uint16_t value = 1;
	bool ok = true;

	for (uint16_t pos = 0; pos < 2048; pos += 128) {
		value = xorshift_fill_128b(value, buffer);
		if (nile_mcu_native_eeprom_read_sync(cmp_buffer, pos >> 1, 64) < 0) {
			console_print(0, s_mcu_communication_error);
			input_wait_any_key();
			ok = false;
		} else {
			for (int i = 0; i < 128; i++) {
				if (buffer[i] != cmp_buffer[i]) {
					console_printf(0, s_eeprom_read_error, pos + i, buffer[i], cmp_buffer[i]);
					input_wait_any_key();
					ok = false;
					if (input_held & (KEY_Y1|KEY_Y2|KEY_Y3|KEY_Y4)) break;
				}
			}
		}
	}
	return ok;
}

void test_sram_retention(bool first) {
	if (first) {
		console_print(0, s_retention_test_writing);
		if (!console_print_status(test_memory_sram_write())) {
			return;
		}
		console_print_newline(0);
	}
	console_print(0, s_retention_test_reading);
	if (test_memory_sram_read()) {
		console_print_status(true);
	}
	console_print_newline(0);
}

void test_eeprom_retention(bool first) {
	if (!test_mcu_begin()) return;

    console_print(0, s_switching_eeprom);
    outportb(IO_NILE_EMU_CNT, (inportb(IO_NILE_EMU_CNT & ~NILE_EMU_EEPROM_MASK)) | NILE_EMU_EEPROM_2KB);
    if (!nile_mcu_native_eeprom_set_mode_sync(NILE_MCU_EEPROM_MODE_M93LC86)) {
        console_print_status(false);
        return;
    }
    if (first) {
    	uint32_t save_id = 0xAA551234;
	    int16_t result;
	    if ((result = nile_mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(NILE_MCU_NATIVE_CMD_SET_SAVE_ID, 0x01), &save_id, 4)) < 0) {
	        console_print_status(false);
	        return;
	    }
	    if ((result = nile_mcu_native_recv_cmd(NULL, 0)) < 0) {
	        console_print_status(false);
	        return;
	    }
    }
    console_print_newline(0);

	ws_eeprom_handle_t handle = ws_eeprom_handle_cartridge(10);
	if (first) {
		console_print(0, s_retention_test_writing);
		if (!console_print_status(test_memory_eeprom_write(handle))) {
			return;
		}
		console_print_newline(0);
	}
	console_print(0, s_retention_test_reading);
	if (test_memory_eeprom_read(handle)) {
		console_print_status(true);
	}
	console_print_newline(0);
}
