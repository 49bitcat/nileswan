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

#include "console.h"
#include "ops/ieeprom.h"
#include "ops/tf_card.h"
#include "tests/flash_fsm.h"
#include "tests/mcu.h"
#include "tests/rtc.h"
#include "tests/sram32kb.h"
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <nile.h>
#include <ws/util.h>

#define IRAM_IMPLEMENTATION
#include "iram.h"

#include "input.h"
#include "menu.h"
#include "strings.h"

#include "ops/id_print.h"
#include "ops/mcu_setup.h"

void console_press_any_key(void) {
	console_print(CONSOLE_FLAG_NO_SERIAL, s_press_any_key_to_continue);
	input_wait_any_key();
}

void main_mfg(void) {
	uint8_t board_rev = inportb(IO_NILE_BOARD_REVISION);

	console_print_header(s_caps_initialization);
	// MCU boot flag setup must run before MCU reset
	if (!op_mcu_setup_boot_flags()) return;
	if (!op_mcu_setup_flash_firmware()) return;

	console_print_header(s_caps_test_suite);
	if (!test_flash_fsm()) return;
	if (!test_rtc_clock()) return;
	if (!op_tf_card_test()) return;
	if (board_rev >= 0x01) {
		if (!test_sram_32kb()) return;
	}

	console_print_header(s_caps_information);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success0);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success1);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success2);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success1);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success0);
	console_print_newline(0);
	if (!op_id_print(0)) return;
}

bool console_warranty_disclaimer(void) {
	console_print(CONSOLE_FLAG_CENTER, s_warranty_disclaimer);
	console_print(0, s_warranty_disclaimer2);
	console_print(0, s_proceed);
	input_wait_any_key();
	console_print_newline(0);
	console_print_newline(0);
	return (input_pressed & KEY_A);
}

static const char __wf_rom* const __wf_rom menu_main[] = {
	s_internal_eeprom_recovery,
	s_tf_card_mgmt,
	s_print_cartridge_info,
	s_print_cartridge_info_usb,
	s_mcu_mgmt,
	s_cartridge_tests,
	s_run_manufacturing_test,
	s_license,
	s_exit,
	NULL
};

static const char __wf_rom* const __wf_rom menu_ieeprom[] = {
	s_disable_custom_splash,
	s_restore_tft_data,
	NULL
};

static const char __wf_rom* const __wf_rom menu_cartridge_tests[] = {
	s_mcu_accel_test,
	s_mcu_usb_cdc_echo,
	s_rtc_clock_test,
	s_rtc_stability_test,
	s_flash_fsm_test,
	s_sram_32kb_test,
	NULL
};

static const char __wf_rom* const __wf_rom menu_card_mgmt[] = {
	s_tf_card_mount,
	s_benchmark_card_read,
	s_benchmark_card_write,
	s_tf_card_format,
	NULL
};

static const char __wf_rom* const __wf_rom menu_mcu_mgmt[] = {
	s_setup_mcu_boot_flags,
	s_flash_mcu_firmware,
	NULL
};

void main(void) {
	cpu_irq_disable();
	ws_hwint_set_handler(HWINT_IDX_VBLANK, vblank_int_handler);
	ws_hwint_enable(HWINT_VBLANK);
	cpu_irq_enable();

	nile_io_unlock();
	nile_bank_unlock();

	console_init();

	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
	if (*((volatile uint16_t __far*) MK_FP(0x1000, 0x01FE)) == 0x3FA7) {
		main_mfg();
		console_press_any_key();
	}

	while (true) {
#ifdef NILESWAN_BRANDING
		console_draw_header(s_nileswan_recovery);
#else
		console_draw_header(s_cart_recovery);
#endif
		int option = menu_run(menu_main);
		int suboption;
option_loop:
		switch (option) {
		case 0:
			console_clear();
			console_draw_header(s_internal_eeprom_recovery);
			suboption = menu_run(menu_ieeprom);
			switch (suboption) {
			case 0:
				console_clear();
				op_ieeprom_disable_custom_splash();
				console_press_any_key();
				break;
			case 1:
				console_clear();
				op_ieeprom_restore_tft_data();
				console_press_any_key();
				break;
			}
			if (suboption >= 0) goto option_loop; else break;
		case 1:
			console_clear();
			console_draw_header(s_tf_card_mgmt);
			suboption = menu_run(menu_card_mgmt);
			switch (suboption) {
			case 0:
				console_clear();
				op_tf_card_init(true);
				console_press_any_key();
				break;
			case 1:
				console_clear();
				op_tf_card_benchmark_read();
				console_press_any_key();
				break;
			case 2:
				console_clear();
				op_tf_card_benchmark_write();
				console_press_any_key();
				break;
			case 3:
				console_clear();
				op_tf_card_format();
				console_press_any_key();
				break;
			}
			if (suboption >= 0) goto option_loop; else break;
		case 2:
		case 3: {
			uint16_t flags = (option == 3) ? CONSOLE_FLAG_MCU_SERIAL : 0;
			console_clear();
			if (flags & CONSOLE_FLAG_MCU_SERIAL) {
				nile_mcu_reset(false);
				ws_delay_ms(500);
				console_press_any_key();
			}
			console_print(CONSOLE_FLAG_NO_SERIAL | CONSOLE_FLAG_MONOSPACE, s_nileswan_header);
		    console_print_newline(flags);
			op_id_print(flags);
		    console_print_newline(flags);
			op_info_print(flags);
			console_press_any_key();
		} break;
		case 4:
			console_clear();
			console_draw_header(s_mcu_mgmt);
			suboption = menu_run(menu_mcu_mgmt);
			switch (suboption) {
			case 0:
				console_clear();
				op_mcu_setup_boot_flags();
				console_press_any_key();
				break;
			case 1:
				console_clear();
				op_mcu_setup_flash_firmware();
				console_press_any_key();
				break;
			}
			if (suboption >= 0) goto option_loop; else break;
		case 5:
			console_clear();
			console_draw_header(s_cartridge_tests);
			suboption = menu_run(menu_cartridge_tests);
			switch (suboption) {
			case 0:
				console_clear();
				test_mcu_accelerometer();
				console_press_any_key();
				break;
			case 1:
				console_clear();
				test_mcu_usb_cdc_echo();
				console_press_any_key();
				break;
			case 2:
				console_clear();
				test_rtc_clock();
				console_press_any_key();
				break;
			case 3:
				console_clear();
				test_rtc_stability(0);
				console_press_any_key();
				break;
			case 4:
				console_clear();
				test_flash_fsm();
				console_press_any_key();
				break;
			case 5:
				console_clear();
				test_sram_32kb();
				console_press_any_key();
				break;
			}
			if (suboption >= 0) goto option_loop; else break;
		case 6:
			console_clear();
			main_mfg();
			console_press_any_key();
			break;
		case 7:
			console_clear();
			console_print(0, s_license_header);
			console_press_any_key();
			break;
		case 8:
			nile_soft_reset();
			break;
		}
	}

	while(1);
}
