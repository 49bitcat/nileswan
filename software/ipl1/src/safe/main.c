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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ws.h>
#include <nile.h>
#include <ws/display.h>
#include <wsx/zx0.h>
#include "assets/tiles.h"
#include "ipc.h"
#include "mem_test.h"
#include "shared.h"
#include "ui.h"
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
	MENU_CFG_OPTION_SPI_SPEED,
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

/* === Utility functions === */

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
	outportb(WS_CART_BANK_ROML_PORT, psram_max_bank >> 4);

	uint8_t __far* header = MK_FP(0xFFFF, 0x0000);
	if (header[0] != 0xEA) return;

	outportb(WS_DISPLAY_CTRL_PORT, 0);
	outportb(WS_SCR1_SCRL_Y_PORT, 0);
	asm volatile("ljmp $0x2FFF, $0x0000");
}

void run_manufacturing_test(void) {
	if (!mem_test_run_ipc()) {
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

#define MENU_REGION_HEIGHT 10

static int run_menu(void) {
	ws_screen_fill_tiles(SCREEN, 0x120, 0, (WS_DISPLAY_HEIGHT_TILES - MENU_REGION_HEIGHT) >> 1, WS_DISPLAY_WIDTH_TILES, MENU_REGION_HEIGHT);

	int menu_count = 0;
	while (menu_items[menu_count] != NULL) menu_count++;
	int menu_y = (WS_DISPLAY_HEIGHT_TILES - menu_count) >> 1;

	for (int i = 0; i < menu_count; i++) {
		DRAW_STRING_CENTERED_DYNAMIC(menu_y + i, menu_items[i], 0);
	}

	if (menu_pos >= menu_count) menu_pos = 0;

	bool is_pcv2 = ws_system_get_model() == WS_MODEL_PCV2;
	int last_menu_pos = -1;

	while(1) {
		if (last_menu_pos != menu_pos) {
			if (last_menu_pos >= 0) {
				ws_screen_modify_tiles(SCREEN, 0x1FF, WS_SCREEN_ATTR_PALETTE(0), 0, menu_y + last_menu_pos, WS_DISPLAY_WIDTH_TILES, 1);
			}
			ws_screen_modify_tiles(SCREEN, 0x1FF, WS_SCREEN_ATTR_PALETTE(1), 0, menu_y + menu_pos, WS_DISPLAY_WIDTH_TILES, 1);
			last_menu_pos = menu_pos;
		}

		ws_int_ack(WS_INT_ACK_VBLANK);
		while (!(inportb(WS_INT_STATUS_PORT) & WS_INT_STATUS_VBLANK)) ia16_halt();

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

static void draw_flash_info(void) {
	nile_flash_manifest_t manifest;
	char text[29];

	nile_flash_wake();

	// print flash UUID
	uint8_t result = nile_flash_read_uuid(manifest.digest);
	if (result) {
		for (int i = 0; i < 4; i++) {
			print_hex_number(SCREEN + (16 * 32) + 6 + 4 * i, __builtin_bswap16(((uint16_t*) manifest.digest)[i]));
		}
	}

	// print version data
	// "fw vM.m.P"
	if (nile_flash_read(&manifest, NILE_FLASH_LAYOUT_MANIFEST_ADDR, sizeof(manifest))) {
		if (manifest.version.id == NILE_FLASH_MANIFEST_ID) {
			snprintf(text, 28, "fw %d.%d.%d%c(%02x%02x%02x%02x)",
				manifest.version.major,
				manifest.version.minor,
				manifest.version.patch,
				manifest.version.partial_install ? '!' : ' ',
				(int) manifest.commit_id[0],
				(int) manifest.commit_id[1],
				(int) manifest.commit_id[2],
				(int) manifest.commit_id[3]
			);
			text[28] = 0;
			DRAW_STRING_CENTERED_DYNAMIC(1, text, 0);
		}
	}


	nile_flash_sleep();
}

void main(void) {
	ia16_disable_irq();
	ws_int_set_enabled(WS_INT_ENABLE_VBLANK);
	ws_int_ack(0xFF);

	// Bootstrap mode boots without the display turned on, so initialize it here
	outportw(WS_LCD_SHADE_01_PORT, WS_DISPLAY_SHADE_LUT_DEFAULT & 0xFFFF);
	outportw(WS_LCD_SHADE_45_PORT, WS_DISPLAY_SHADE_LUT_DEFAULT >> 16);
	outportb(WS_LCD_CTRL_PORT, inportb(WS_LCD_CTRL_PORT) | WS_LCD_CTRL_DISPLAY_ENABLE);

	outportw(WS_CART_EXTBANK_RAM_PORT, NILE_SEG_RAM_IPC);
	ipc_init(MEM_NILE_IPC);

	bool sram_io_speed_limit = true;
	bool spi_speed_limit = false;

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
		WS_DISPLAY_COLOR_MEM(8)[0] = 0xFFF;
		WS_DISPLAY_COLOR_MEM(8)[1] = 0x888;
	} else {
		outportw(WS_SCR_PAL_0_PORT, 0x0070); // normal
		outportw(WS_SCR_PAL_1_PORT, 0x0007); // inverted
		outportw(WS_SCR_PAL_2_PORT, 0x0060); // fail
		outportw(WS_SCR_PAL_3_PORT, 0x0040); // pass
		outportw(WS_SCR_PAL_8_PORT, 0x0040); // grey
	}
	outportb(WS_SCR_BASE_PORT, WS_SCR_BASE_ADDR1(SCREEN));
	wsx_zx0_decompress((uint16_t*) 0x3200, gfx_tiles);
	outportw(WS_SCR1_SCRL_X_PORT, (13 * 8) << 8);

	outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_FAST);

	while (true) {
		if (full_redraw) {
			clear_screen();
			outportb(WS_DISPLAY_CTRL_PORT, WS_DISPLAY_CTRL_SCR1_ENABLE);

	#ifdef NILESWAN_BRANDING
			DRAW_STRING_CENTERED(0, "nileswan ipl1/safe " VERSION, 0);
	#else
			DRAW_STRING_CENTERED(0, "cart ipl1/safe " VERSION, 0);
	#endif
			DRAW_STRING_CENTERED(17, "copyright (c) 2024-2026", WS_SCREEN_ATTR_PALETTE(8));
			draw_flash_info();

			full_redraw = false;
		}

		menu_pos = 0;
		menu_items[MENU_OPTION_MANUFACTURING_TEST] = "manufacturing test";
		menu_items[MENU_OPTION_BOOT_RECOVERY_CURRENT] = "launch recovery";
		menu_items[MENU_OPTION_QUICK_TEST] = "board check";
		menu_items[MENU_OPTION_CONFIG] = "settings >";
		menu_items[MENU_OPTION_ADVANCED] = "advanced >";
		menu_items[MENU_OPTIONS_COUNT] = NULL;

		switch (run_menu()) {
			case MENU_OPTION_MANUFACTURING_TEST:
				clear_screen();
				if (!mem_test_run_quick_v2()) {
				    wait_for_button();
				} else {
				    DRAW_STRING_CENTERED(17, "please wait", 0);
				    ws_delay_ms(1000);
				}
				mem_test_run_deep(false);
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
				mem_test_run_quick_v2();
				wait_for_button();
				break;
			case MENU_OPTION_CONFIG:
				menu_pos = 0;
menu_config_run:
				menu_items[MENU_CFG_OPTION_RAM_SIZE] = (psram_max_bank == 0xFF) ? "PSRAM amount: 16 MiB" : "PSRAM amount: 8 MiB";
				menu_items[MENU_CFG_OPTION_SRAM_SPEED] = sram_io_speed_limit ? "I/O speed: slow" : "I/O speed: fast";
				menu_items[MENU_CFG_OPTION_SPI_SPEED] = spi_speed_limit ? "SPI speed: 384 KHz" : "SPI speed: 24 MHz";
				menu_items[MENU_CFG_OPTION_EXIT] = "< back";
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
					case MENU_CFG_OPTION_SPI_SPEED:
						spi_speed_limit = !spi_speed_limit;
						outportw(IO_NILE_SPI_CNT, spi_speed_limit ? NILE_SPI_CLOCK_CART : NILE_SPI_CLOCK_FAST);
						goto menu_config_run;
				}
				break;
			case MENU_OPTION_ADVANCED:
				menu_pos = 0;
menu_advanced_run:
				menu_items[MENU_ADV_OPTION_MEMORY_TEST] = "extended memory test";
				menu_items[MENU_ADV_OPTION_BOOT_RECOVERY_FACTORY] = "launch factory recovery";
				menu_items[MENU_ADV_OPTION_RETENTION] = "test SRAM after reboot";
				menu_items[MENU_ADV_OPTION_EXIT] = "< back";
				menu_items[MENU_ADV_OPTIONS_COUNT] = NULL;

				switch (run_menu()) {
					case MENU_ADV_OPTION_MEMORY_TEST:
						mem_test_run_deep(true);
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
						mem_test_run_sram_readout();
						goto menu_advanced_run;
				}
				break;
		}
	}
}
