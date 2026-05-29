/**
 * Copyright (c) 2024, 2025, 2026 Adrian "asie" Siekierka
 *
 * Nileswan IPL1 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan IPL1 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan IPL1. If not, see <https://www.gnu.org/licenses/>.
 */

#include <ws.h>
#include <nile.h>
#include "shared.h"
#include "ui.h"

// Quick memory test

// Deep memory test

extern void mem_test_deep(void *results, uint16_t bank_count);

#define TEST_MODE_DEFAULT 0
#define TEST_MODE_ONLY_READ 1
#define TEST_MODE_BOOL_PASS 254
#define TEST_MODE_BOOL_FAIL 255
extern uint8_t mem_test_deep_mode;
static bool mem_test_deep_bool(uint16_t bank_count) {
	mem_test_deep_mode = TEST_MODE_BOOL_PASS;
	mem_test_deep(NULL, bank_count);
	bool result = mem_test_deep_mode == TEST_MODE_BOOL_PASS;
	mem_test_deep_mode = TEST_MODE_DEFAULT;
	return result;
}

static void draw_pass_fail(uint8_t y, bool result) {
	mem_expand_8_16(SCREEN + ((y * 32)) + 22, result ? "PASS" : "FAIL", 4, WS_SCREEN_ATTR_PALETTE(result ? 3 : 2) | 0x100);
}

bool mem_test_run_ipc() {
	bool result = true;

	outportw(WS_CART_EXTBANK_RAM_PORT, 14);
	__far uint16_t* ipc_buf = MK_FP(0x1000, 0);

	for (uint16_t i = 0; i < sizeof(nile_ipc_t); i+=2)
		*(ipc_buf++) = (i * 0x101) + 1;

	// IPC buffer should mirror
	for (uint16_t i = 0; i < sizeof(nile_ipc_t); i+=2) {
		if (*(ipc_buf++) != (i * 0x101) + 1) {
			result = false;
			break;
		}
	}

	// de-initialize IPC buffer
	memset(ipc_buf, 0, sizeof(nile_ipc_t));

	return result;
}

void mem_test_run_quick(void) {
	clear_screen();
	DRAW_STRING_CENTERED(0, "quick test in progress", 0);

	DRAW_STRING(2, 2, "PSRAM write/read", 0);
	outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
	draw_pass_fail(2, mem_test_deep_bool(PSRAM_MAX_BANK));
	DRAW_STRING(2, 3, "SRAM write/read", 0);
	outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
	draw_pass_fail(3, mem_test_deep_bool(SRAM_MAX_BANK + 1));

	DRAW_STRING(2, 4, "IPC buf write/read", 0);
	draw_pass_fail(4, mem_test_run_ipc());

	ws_screen_fill_tiles(SCREEN, 0x120, 0, 0, WS_DISPLAY_WIDTH_TILES, 1);
	DRAW_STRING_CENTERED(0, "quick test complete", 0);
	wait_for_button();
}

void mem_test_run_deep(bool loop) {
	clear_screen();
	DRAW_STRING(3, 0, "PSRAM  Mem. Test  SRAM", 0);

	do {
		outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
		mem_test_deep(SCREEN + (1 * 32), PSRAM_MAX_BANK + 1);
		outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
		mem_test_deep(SCREEN + (1 * 32) + 19, SRAM_MAX_BANK + 1);
	} while(loop);
}

void mem_test_run_sram_readout(void) {
	clear_screen();
	DRAW_STRING_CENTERED(0, "testing SRAM read", 0);
	DRAW_STRING(19, 2, "76543210", 0);

	while (true) {
		outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
		mem_test_deep_mode = TEST_MODE_ONLY_READ;
		mem_test_deep(SCREEN + (3 * 32) + 19, SRAM_MAX_BANK + 1);
		mem_test_deep_mode = TEST_MODE_DEFAULT;
	}
	wait_for_button();
}
