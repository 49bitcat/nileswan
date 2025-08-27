/**
 * Copyright (c) 2024, 2025 Adrian Siekierka
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

#include <stddef.h>
#include <string.h>
#include <ws.h>
#include <nile.h>
#include <wsx/zx0.h>
#include "assets/tiles.h"
#include "ipc.h"
#include "util.h"

typedef enum {
	MENU_OPTION_MANUFACTURING_TEST,
	MENU_OPTION_BOOT_RECOVERY_CURRENT,
	MENU_OPTION_QUICK_TEST,
	MENU_OPTION_CONFIG,
	MENU_OPTION_ADVANCED,
	MENU_OPTIONS_COUNT
} menu_option_t;

typedef enum {
	MENU_CFG_OPTION_RAM_SIZE,
	MENU_CFG_OPTION_SRAM_SPEED,
	MENU_CFG_OPTION_EXIT,
	MENU_CFG_OPTIONS_COUNT
} menu_cfg_option_t;

typedef enum {
	MENU_ADV_OPTION_MEMORY_TEST,
	MENU_ADV_OPTION_BOOT_RECOVERY_FACTORY,
	MENU_ADV_OPTION_RETENTION,
	MENU_ADV_OPTION_EXIT,
	MENU_ADV_OPTIONS_COUNT
} menu_adv_option_t;

#define SCREEN ((uint16_t*) (0x3800 + (13 * 32 * 2)))

uint8_t psram_max_bank = 0xFF;
#define SRAM_MAX_BANK 7

/* === Test code in external files === */

// tests_asm.s
extern void ram_fault_test(void *results, uint16_t bank_count);
#define TEST_MODE_DEFAULT 0
#define TEST_MODE_ONLY_READ 1
#define TEST_MODE_BOOL_PASS 254
#define TEST_MODE_BOOL_FAIL 255
extern uint8_t ram_fault_test_mode;
bool ram_fault_test_bool(uint16_t bank_count) {
	ram_fault_test_mode = TEST_MODE_BOOL_PASS;
	ram_fault_test(NULL, bank_count);
	bool result = ram_fault_test_mode == TEST_MODE_BOOL_PASS;
	ram_fault_test_mode = TEST_MODE_DEFAULT;
	return result;
}

/* === Utility functions === */

static void clear_screen(void) {
	ws_screen_fill_tiles(SCREEN, 0x120, 0, 0, 28, 18);
}

#define DRAW_STRING(x, y, s, pal) mem_expand_8_16(SCREEN + ((y) * 32) + (x), (s), sizeof(s) - 1, 0x100 | pal);
#define DRAW_STRING_CENTERED(y, s, pal) DRAW_STRING(((29 - sizeof(s)) >> 1), y, s, pal)
#define DRAW_STRING_DYNAMIC(x, y, s, pal) mem_expand_8_16(SCREEN + ((y) * 32) + (x), (s), strlen(s), 0x100 | pal);
#define DRAW_STRING_CENTERED_DYNAMIC(y, s, pal) DRAW_STRING_DYNAMIC(((30 - strlen(s)) >> 1), y, s, pal)

/* === Memory test === */

static void draw_pass_fail(uint8_t y, bool result) {
	mem_expand_8_16(SCREEN + ((y * 32)) + 22, result ? "PASS" : "FAIL", 4, WS_SCREEN_ATTR_PALETTE(result ? 3 : 2) | 0x100);
}

/* static void draw_result_byte(uint8_t y, uint8_t value, bool result) {
	uint16_t* dst = SCREEN + ((y * 32)) + 22;
	if (result)
		print_hex_number(dst, value);
	else
		mem_expand_8_16(dst, "FAIL", 4, WS_SCREEN_ATTR_PALETTE(result ? 3 : 2) | 0x100);
} */

static void wait_for_button(void) {
	DRAW_STRING_CENTERED(17, "press any button", 0);
	while(!ws_keypad_scan());
	while(ws_keypad_scan());
}

static bool ipc_buf_test() {
	bool result = true;

	outportw(WS_CART_EXTBANK_RAM_PORT, 14);
	__far uint16_t* ipc_buf = MK_FP(0x1000, 0);

	for (uint16_t i = 0; i < sizeof(nile_ipc_t); i+=2)
		*(ipc_buf++) = i ^ (i >> 8);

	// IPC buffer should mirror
	for (uint16_t i = 0; i < sizeof(nile_ipc_t); i+=2) {
		if (*(ipc_buf++) != (i ^ (i >> 8))) {
			result = false;
			break;
		}
	}

	// de-initialize IPC buffer
	memset(ipc_buf, 0, sizeof(nile_ipc_t));

	return result;
}

void run_quick_test(void) {
	clear_screen();
	DRAW_STRING_CENTERED(0, "quick test in progress", 0);

	DRAW_STRING(2, 2, "PSRAM write/read", 0);
	outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
	draw_pass_fail(2, ram_fault_test_bool(psram_max_bank));
	DRAW_STRING(2, 3, "SRAM write/read", 0);
	outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
	draw_pass_fail(3, ram_fault_test_bool(SRAM_MAX_BANK + 1));

	DRAW_STRING(2, 4, "IPC buf write/read", 0);
	draw_pass_fail(4, ipc_buf_test());

	ws_screen_fill_tiles(SCREEN, 0x120, 0, 0, 28, 1);
	DRAW_STRING_CENTERED(0, "quick test complete", 0);
	wait_for_button();
}

void run_full_memory_test(bool loop) {
	clear_screen();
	DRAW_STRING(3, 0, "PSRAM  Mem. Test  SRAM", 0);

	do {
		outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
		ram_fault_test(SCREEN + (1 * 32), psram_max_bank + 1);
		outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
		ram_fault_test(SCREEN + (1 * 32) + 19, SRAM_MAX_BANK + 1);
	} while(loop);
}

void run_read_memory_test(void) {
	clear_screen();
	DRAW_STRING_CENTERED(0, "testing SRAM read", 0);
	DRAW_STRING(19, 2, "76543210", 0);

	while (true) {
		outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
		ram_fault_test_mode = TEST_MODE_ONLY_READ;
		ram_fault_test(SCREEN + (3 * 32) + 19, SRAM_MAX_BANK + 1);
		ram_fault_test_mode = TEST_MODE_DEFAULT;
	}
	wait_for_button();
}

bool load_spi_flash(uint32_t _offset, uint16_t banks) {
	uint8_t buffer[256];
	uint32_t offset;

	outportb(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
	nile_flash_wake();

	offset = _offset;
	// Quickly load all data from SPI flash to PSRAM
	DRAW_STRING(1, 1, "writing bank ", 0);
	for (uint16_t bank = psram_max_bank + 1 - banks; bank <= psram_max_bank; bank++) {
		outportw(WS_CART_EXTBANK_RAM_PORT, bank);
		print_hex_number(SCREEN + (1 * 32) + 14, bank);

		for (int i = 0; i < 2; i++) {
			if (!nile_flash_read(MK_FP(0x1000, i << 15), offset, 0x8000)) {
				DRAW_STRING(1, 2, "flash read error", 0);
				print_hex_number(SCREEN + (3 * 32) + 1, offset >> 16);
				print_hex_number(SCREEN + (3 * 32) + 5, offset);
				return false;
			}
			offset += 0x8000;
		}
	}

	offset = _offset;
	// Verify data was loaded correctly (in case PSRAM is damaged)
	DRAW_STRING(1, 2, "verifying bank ", 0);
	for (uint16_t bank = psram_max_bank + 1 - banks; bank <= psram_max_bank; bank++) {
		outportw(WS_CART_EXTBANK_RAM_PORT, bank);
		print_hex_number(SCREEN + (2 * 32) + 16, bank);

		for (int i = 0; i < (65536 / sizeof(buffer)); i++) {
			if (!nile_flash_read(buffer, offset, sizeof(buffer))) {
				DRAW_STRING(1, 3, "flash read error", 0);
				print_hex_number(SCREEN + (4 * 32) + 1, offset >> 16);
				print_hex_number(SCREEN + (4 * 32) + 5, offset);
				return false;
			}
			if (memcmp(MK_FP(0x1000, i * sizeof(buffer)), buffer, sizeof(buffer))) {
				DRAW_STRING(1, 3, "PSRAM write error", 0);
				print_hex_number(SCREEN + (4 * 32) + 1, offset >> 16);
				print_hex_number(SCREEN + (4 * 32) + 5, offset);
				return false;
			}
			offset += sizeof(buffer);
		}
	}

	return true;
}

void try_boot_rom(void) {
	outportb(WS_CART_BANK_FLASH_PORT, 0);
	outportw(WS_CART_EXTBANK_ROM0_PORT, psram_max_bank);
	outportw(WS_CART_EXTBANK_ROM1_PORT, psram_max_bank - 12);
	outportw(WS_CART_EXTBANK_RAM_PORT, 0);
	outportw(WS_CART_BANK_ROML_PORT, psram_max_bank >> 4);

	uint8_t __far* header = MK_FP(0xFFFF, 0x0000);
	if (header[0] != 0xEA) return;

	outportb(WS_DISPLAY_CTRL_PORT, 0);
	outportb(WS_SCR1_SCRL_Y_PORT, 0);
	asm volatile("ljmp $0x2FFF, $0x0000");
}

void run_manufacturing_test(void) {
	if (!ipc_buf_test()) {
		DRAW_STRING(1, 1, "IPC buffer error", 0);
		wait_for_button();
	} else {
		// set magic byte
		*((volatile uint16_t __far*) MK_FP(0x1000, 0x01FE)) = 0x3FA7;
		// run factory recovery
		if (load_spi_flash(0x10000, 3)) {
			nile_flash_sleep();
			try_boot_rom();
		} else {
			nile_flash_sleep();
			wait_for_button();
		}
	}
}

const char *menu_items[16];
int menu_pos = 0;
uint16_t keys_pressed = 0;
uint16_t keys_held = 0;

static int run_menu(void) {
	ws_screen_fill_tiles(SCREEN, 0x120, 0, 1, 28, 16);

	int menu_count = 0;
	while (menu_items[menu_count] != NULL) menu_count++;
	int menu_y = (18 - menu_count) >> 1;

	for (int i = 0; i < menu_count; i++) {
		DRAW_STRING_CENTERED_DYNAMIC(menu_y + i, menu_items[i], 0);
	}

	if (menu_pos >= menu_count) menu_pos = 0;

	bool is_pcv2 = ws_system_get_model() == WS_MODEL_PCV2;
	int last_menu_pos = -1;

	while(1) {
		if (last_menu_pos != menu_pos) {
			if (last_menu_pos >= 0) {
				ws_screen_modify_tiles(SCREEN, 0x1FF, WS_SCREEN_ATTR_PALETTE(0), 0, menu_y + last_menu_pos, 28, 1);
			}
			ws_screen_modify_tiles(SCREEN, 0x1FF, WS_SCREEN_ATTR_PALETTE(1), 0, menu_y + menu_pos, 28, 1);
			last_menu_pos = menu_pos;
		}

		ia16_halt();

		uint16_t keys = ws_keypad_scan();
		keys_pressed = keys & ~keys_held;
		keys_held = keys;

		if (keys_pressed & (is_pcv2 ? WS_KEY_PCV2_UP : WS_KEY_X1)) {
			menu_pos--;
			if (menu_pos < 0) menu_pos = menu_count - 1;
		}
		if (keys_pressed & (is_pcv2 ? WS_KEY_PCV2_DOWN : WS_KEY_X3)) {
			menu_pos++;
			if (menu_pos >= menu_count) menu_pos = 0;
		}
		if (keys_pressed & (is_pcv2 ? WS_KEY_PCV2_CIRCLE : WS_KEY_A)) {
			while(ws_keypad_scan());
			return menu_pos;
		}
		if (keys_pressed & (is_pcv2 ? WS_KEY_PCV2_CLEAR : WS_KEY_B)) {
			while(ws_keypad_scan());
			return -1;
		}
	}
}

void main(void) {
	ia16_disable_irq();
	ws_int_set_enabled(WS_INT_ENABLE_VBLANK);

	// Bootstrap mode boots without the display turned on, so initialize it here
	outportw(WS_LCD_SHADE_01_PORT, WS_DISPLAY_SHADE_LUT_DEFAULT & 0xFFFF);
	outportw(WS_LCD_SHADE_45_PORT, WS_DISPLAY_SHADE_LUT_DEFAULT >> 16);
	outportb(WS_LCD_CTRL_PORT, inportb(WS_LCD_CTRL_PORT) | WS_LCD_CTRL_DISPLAY_ENABLE);

	outportw(WS_CART_EXTBANK_RAM_PORT, NILE_SEG_RAM_IPC);
	ipc_init(MEM_NILE_IPC);

	bool sram_io_speed_limit = true;

	if (ws_system_is_color_model()) {
		outportb(WS_SYSTEM_CTRL_COLOR_PORT, WS_MODE_COLOR);
		sram_io_speed_limit = false;
		WS_DISPLAY_COLOR_MEM(0)[0] = 0xFFF;
		WS_DISPLAY_COLOR_MEM(0)[1] = 0x000;
		WS_DISPLAY_COLOR_MEM(1)[0] = 0x000;
		WS_DISPLAY_COLOR_MEM(1)[1] = 0xFFF;
		WS_DISPLAY_COLOR_MEM(2)[0] = 0xFFF;
		WS_DISPLAY_COLOR_MEM(2)[1] = WS_RGB(11, 0, 0);
		WS_DISPLAY_COLOR_MEM(3)[0] = 0xFFF;
		WS_DISPLAY_COLOR_MEM(3)[1] = WS_RGB(0, 12, 0);
	} else {
		outportw(WS_SCR_PAL_0_PORT, 0x0070);
		outportw(WS_SCR_PAL_1_PORT, 0x0007);
		outportw(WS_SCR_PAL_2_PORT, 0x0060);
		outportw(WS_SCR_PAL_3_PORT, 0x0040);
	}
	outportb(WS_SCR_BASE_PORT, WS_SCR_BASE_ADDR1(SCREEN));
	wsx_zx0_decompress((uint16_t*) 0x3200, gfx_tiles);
	outportw(WS_SCR1_SCRL_X_PORT, (13 * 8) << 8);

	clear_screen();
	outportb(WS_DISPLAY_CTRL_PORT, WS_DISPLAY_CTRL_SCR1_ENABLE);

#ifdef NILESWAN_BRANDING
	DRAW_STRING_CENTERED(0, "nileswan safe ipl1 v" VERSION, 0);
#else
	DRAW_STRING_CENTERED(0, "cart safe ipl1 v" VERSION, 0);
#endif
	DRAW_STRING_CENTERED(17, "copyright (c) 2024-2025", 0);

	while (true) {
		menu_pos = 0;
		menu_items[MENU_OPTION_MANUFACTURING_TEST] = "manufacturing test";
		menu_items[MENU_OPTION_BOOT_RECOVERY_CURRENT] = "launch recovery";
		menu_items[MENU_OPTION_QUICK_TEST] = "memory test";
		menu_items[MENU_OPTION_CONFIG] = "settings >";
		menu_items[MENU_OPTION_ADVANCED] = "advanced >";
		menu_items[MENU_OPTIONS_COUNT] = NULL;

		switch (run_menu()) {
			case MENU_OPTION_MANUFACTURING_TEST:
				clear_screen();
				run_full_memory_test(false);
				clear_screen();
				run_manufacturing_test();
				break;
			case MENU_OPTION_BOOT_RECOVERY_CURRENT:
				clear_screen();
				if (load_spi_flash(0x40000, 4)) {
					nile_flash_sleep();
					try_boot_rom();
				} else {
					nile_flash_sleep();
					wait_for_button();
				}
				break;
			case MENU_OPTION_QUICK_TEST:
				run_quick_test();
				break;
			case MENU_OPTION_CONFIG:
				menu_pos = 0;
menu_config_run:
				menu_items[MENU_CFG_OPTION_RAM_SIZE] = (psram_max_bank == 0xFF) ? "PSRAM amount: 16 MiB" : "PSRAM amount: 8 MiB";
				menu_items[MENU_CFG_OPTION_SRAM_SPEED] = sram_io_speed_limit ? "I/O speed: slow" : "I/O speed: fast";
				menu_items[MENU_CFG_OPTION_EXIT] = "exit";
				menu_items[MENU_CFG_OPTIONS_COUNT] = NULL;

				switch (run_menu()) {
					case MENU_CFG_OPTION_RAM_SIZE:
						psram_max_bank ^= 0x80;
						goto menu_config_run;
					case MENU_CFG_OPTION_SRAM_SPEED:
						if (ws_system_is_color_active()) {
							sram_io_speed_limit = !sram_io_speed_limit;
							if (sram_io_speed_limit) {
								outportb(WS_SYSTEM_CTRL_COLOR_PORT, WS_MODE_COLOR | WS_SYSTEM_CTRL_COLOR_SRAM_WAIT | WS_SYSTEM_CTRL_COLOR_IO_WAIT);
							} else {
								outportb(WS_SYSTEM_CTRL_COLOR_PORT, WS_MODE_COLOR);
							}
						}
						goto menu_config_run;
				}
				break;
			case MENU_OPTION_ADVANCED:
				menu_pos = 0;
menu_advanced_run:
				menu_items[MENU_ADV_OPTION_MEMORY_TEST] = "extended memory test";
				menu_items[MENU_ADV_OPTION_BOOT_RECOVERY_FACTORY] = "launch factory recovery";
				menu_items[MENU_ADV_OPTION_RETENTION] = "test SRAM after reboot";
				menu_items[MENU_ADV_OPTION_EXIT] = "exit";
				menu_items[MENU_ADV_OPTIONS_COUNT] = NULL;

				switch (run_menu()) {
					case MENU_ADV_OPTION_MEMORY_TEST:
						run_full_memory_test(true);
						goto menu_advanced_run;
					case MENU_ADV_OPTION_BOOT_RECOVERY_FACTORY:
						clear_screen();
						if (load_spi_flash(0x10000, 3)) {
							nile_flash_sleep();
							try_boot_rom();
						} else {
							nile_flash_sleep();
							wait_for_button();
						}
						goto menu_advanced_run;
					case MENU_ADV_OPTION_RETENTION:
						run_read_memory_test();
						goto menu_advanced_run;
				}
				break;
		}
	}
}
