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

#include <nile/hardware.h>
#include <ws.h>
#include <nile.h>
#include <ws/display.h>
#include <ws/memory.h>
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

void mem_test_qv2_init_screen(void) {
    clear_screen();
    for (int i = 0; i < 23; i++) {
        int ix = 4 + 22 - i;
        SCREEN[ix + (1 * 32)] = 'A' | 0x100;
        SCREEN[ix + (2 * 32)] = ('0' + (i / 10)) | 0x100;
        SCREEN[ix + (3 * 32)] = ('0' + (i % 10)) | 0x100;

        if (i < 16) {
            SCREEN[ix + (9 * 32)] = 'D' | 0x100;
            SCREEN[ix + (10 * 32)] = ('0' + (i / 10)) | 0x100;
            SCREEN[ix + (11 * 32)] = ('0' + (i % 10)) | 0x100;
        }
    }

    DRAW_STRING(1, 4, "PS1", 0);
    DRAW_STRING(1, 5, "PS2", 0);
    DRAW_STRING(1, 6, "SRM", 0);
    DRAW_STRING(1, 7, "FPG", 0);

    DRAW_STRING(8, 12, "PS1", 0);
    DRAW_STRING(8, 13, "PS2", 0);
    DRAW_STRING(8, 14, "SRM", 0);
    DRAW_STRING(8, 15, "FPG", 0);
}

bool mem_test_qv2_print_values(bool *array, int offset, int count, int y) {
    uint16_t *ptr = SCREEN + (y * 32) + 26 - offset;
    bool result = true;
    while (count--) {
        result &= *array != 0;
        *(ptr--) = *(array++) ? ('.' | 0x100) : ('X' | 0x100 | WS_SCREEN_ATTR_PALETTE(2));
    }
    return result;
}

void mem_test_qv2_print_question_marks(int offset, int count, int y) {
    uint16_t *ptr = SCREEN + (y * 32) + 26 - offset;
    while (count--) {
        *(ptr--) = ('?' | 0x100);
    }
}

static void mem_test_qv2_phys_address_line(bool *array, int count, uint16_t start_bank) {
    int bank_count = 0;
    memset(array, 0, count);
    if (count > 16) {
        bank_count = count - 16;
        count = 16;
    }

    outportw(WS_CART_EXTBANK_RAM_PORT, start_bank);

    // Test physical address lines A0-A15.
    for (int bit = 0; bit < count; array++, bit++) {
        // Write value to ~A0, check A0
        volatile uint8_t __far* addr1 = MK_FP(0x1000, 0);
        volatile uint8_t __far* addr2 = MK_FP(0x1000, 1 << bit);

        *addr1 = 0x00;
        *addr2 = 0xFF;
        if (*addr1 != 0x00) continue;
        *addr1 = 0xFF;
        *addr2 = 0x00;
        if (*addr1 != 0xFF) continue;

        *array = true;
    }

    // Test physical address lines A16+.
    volatile uint8_t __far* addr = MK_FP(0x1000, 0);
    for (int bank = 0; bank < bank_count; array++, bank++) {
        *addr = 0x00;
        outportw(WS_CART_EXTBANK_RAM_PORT, start_bank + (1 << bank));
        *addr = 0xFF;
        outportw(WS_CART_EXTBANK_RAM_PORT, start_bank);
        if (*addr != 0x00) continue;
        *addr = 0xFF;
        outportw(WS_CART_EXTBANK_RAM_PORT, start_bank + (1 << bank));
        *addr = 0x00;
        outportw(WS_CART_EXTBANK_RAM_PORT, start_bank);
        if (*addr != 0xFF) continue;

        *array = true;
    }
}

static void mem_test_qv2_phys_data_line_16(bool *array, uint16_t bank) {
	memset(array, 0, 16);

    outportw(WS_CART_EXTBANK_RAM_PORT, bank);
    outportw(WS_CART_EXTBANK_ROM0_PORT, bank);
    volatile uint16_t __far* addr_w = MK_FP(0x1000, 0);
    volatile uint16_t __far* addr_r = MK_FP(0x2000, 0);
    for (int bit = 0; bit < 16; array++, bit++) {
        *addr_w = 0;
        if (*addr_r & (1 << bit)) continue;
        *addr_w = (1 << bit);
        if (!(*addr_r & (1 << bit))) continue;

        *array = true;
    }
}

static void mem_test_qv2_phys_data_line_8(bool *array) {
	memset(array, 0, 8);

	volatile uint8_t __far* addr = MK_FP(0x1000, 0);
    for (int bit = 0; bit < 8; array++, bit++) {
        *addr = 0;
        if (*addr & (1 << bit)) continue;
        *addr = (1 << bit);
        if (!(*addr & (1 << bit))) continue;

        *array = true;
    }
}

static void mem_test_qv2_psram_a16_a19(bool *array) {
	memset(array, 0, 4);

	ws_bank_with_roml(0, {
		outportw(WS_CART_EXTBANK_RAM_PORT, 15);

		// Test virtual address lines A16-A19.
		// This is done by borrowing PSRAM1.
		for (int line = 0; line < 4; array++, line++) {
			volatile uint8_t __far* addr_w = MK_FP(0x1000, 0);
			volatile uint8_t __far* addr_r = MK_FP(0xF000 ^ (0x1000 << line), 0);

			outportw(WS_CART_EXTBANK_RAM_PORT, 15 ^ (1 << line));
			*addr_w = 0x00;
			outportw(WS_CART_EXTBANK_RAM_PORT, 15);
	        *addr_w = 0xFF;
	        if (*addr_r != 0x00) continue;
	        outportw(WS_CART_EXTBANK_RAM_PORT, 15 ^ (1 << line));
	        *addr_w = 0xFF;
	        outportw(WS_CART_EXTBANK_RAM_PORT, 15);
	        *addr_w = 0x00;
	        if (*addr_r != 0xFF) continue;

	        *array = true;
		}

		outportw(WS_CART_EXTBANK_RAM_PORT, 0);
	});
}

bool mem_test_run_quick_v2(void) {
    bool array[24];
    bool psram1_dead = false;
    bool result = true;

    mem_test_qv2_init_screen();
    // PSRAM1 address
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
    mem_test_qv2_phys_address_line(array, 23, 0x00);
    bool psram1_addr = mem_test_qv2_print_values(array, 0, 23, 4);;
    result &= psram1_addr;
    psram1_dead |= !psram1_addr;
    // PSRAM2 address
    mem_test_qv2_phys_address_line(array, 23, 0x80);
    result &= mem_test_qv2_print_values(array, 0, 23, 5);
    // SRAM address
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
    mem_test_qv2_phys_address_line(array, 19, 0x00);
    result &= mem_test_qv2_print_values(array, 0, 19, 6);
    // IPC address
    mem_test_qv2_phys_address_line(array, 9, NILE_SEG_RAM_IPC);
    result &= mem_test_qv2_print_values(array, 0, 9, 7);
    // PSRAM1 data
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
    mem_test_qv2_phys_data_line_16(array, 0);
    bool psram1_data = mem_test_qv2_print_values(array, 0, 16, 12);
    result &= psram1_data;
    psram1_dead |= !psram1_data;
    // Cart A16-A19
    if (!psram1_dead) {
    	mem_test_qv2_psram_a16_a19(array + 16);
    	result &= mem_test_qv2_print_values(array + 16, 16, 4, 7);
    } else {
   		mem_test_qv2_print_question_marks(16, 4, 7);
    }
    // PSRAM2 data
    mem_test_qv2_phys_data_line_16(array, 0x80);
    result &= mem_test_qv2_print_values(array, 0, 16, 13);
    // SRAM data
    outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
    outportw(WS_CART_EXTBANK_RAM_PORT, 0);
    mem_test_qv2_phys_data_line_8(array);
    result &= mem_test_qv2_print_values(array, 0, 8, 14);
    // IPC data
    outportw(WS_CART_EXTBANK_RAM_PORT, NILE_SEG_RAM_IPC);
    mem_test_qv2_phys_data_line_8(array);
    result &= mem_test_qv2_print_values(array, 0, 8, 15);

    return result;
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
