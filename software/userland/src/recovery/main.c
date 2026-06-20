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
#include "config.h"
#include "ops/id_print.h"
#include "ops/ieeprom.h"
#include "ops/mcu.h"
#include "ops/mcu_setup.h"
#include "ops/tf_card.h"
#include "tests/button.h"
#include "tests/flash_fsm.h"
#include "tests/mcu.h"
#include "tests/memory.h"
#include "tests/rtc.h"
#include "tests/sram.h"
#include <nile/hardware.h>
#include <nilefs.h>
#include <stdbool.h>
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

void console_press_any_key(void) {
	console_print(CONSOLE_FLAG_NO_SERIAL, s_press_any_key_to_continue);
	input_wait_any_key();
}

bool run_mfg_tests(void) {
	uint8_t board_rev = inportb(IO_NILE_BOARD_REVISION);

	console_print_header(s_caps_initialization);
	// MCU boot flag setup must run before MCU reset
	if (!op_mcu_setup_boot_flags()) return false;
	if (!op_mcu_setup_flash_firmware()) return false;

	// Automatic tests
	console_print_header(s_caps_test_suite);
	if (!test_flash_fsm()) return false;
	if (!test_rtc_clock()) return false;
	if (!test_mcu_eeprom()) return false;
	if (!test_mcu_save_id()) return false;
	if (!op_tf_card_test()) return false;
	if (board_rev >= 0x01) {
		if (!test_sram_32kb()) return false;
	}

	// Manual tests
	alert_mode_set(ALERT_ALERT);
	console_print_header(s_caps_test_suite_manual);
	console_print(CONSOLE_FLAG_MONOSPACE | CONSOLE_FLAG_NO_SERIAL, s_mfg_test_success0);
	console_print(CONSOLE_FLAG_MONOSPACE | CONSOLE_FLAG_NO_SERIAL, s_mfg_test_success1);
	console_print(CONSOLE_FLAG_MONOSPACE | CONSOLE_FLAG_NO_SERIAL, s_mfg_test_alert2);
	console_print(CONSOLE_FLAG_MONOSPACE | CONSOLE_FLAG_NO_SERIAL, s_mfg_test_alert3);
	console_print(CONSOLE_FLAG_MONOSPACE | CONSOLE_FLAG_NO_SERIAL, s_mfg_test_success1);
	console_print(CONSOLE_FLAG_MONOSPACE | CONSOLE_FLAG_NO_SERIAL, s_mfg_test_success0);
	input_wait_any_key();
	alert_mode_set(ALERT_NONE);
	if (!test_mcu_accelerometer()) return false;
	if (!test_button()) return false;

	console_print_header(s_caps_information);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success0);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success1);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success2);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success1);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success0);
	console_print_newline(0);
	if (!op_id_print(0)) return false;

	return true;
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
	s_console_recovery,
	s_tf_card_mgmt,
	s_cartridge_diagnostics,
	s_cartridge_recovery,
	s_run_manufacturing_test,
	s_license,
	s_exit,
#ifdef CONFIG_ENABLE_DEV_FEATURES
	s_dev_features,
#endif
	NULL
};

static void do_mfg_tests(void) {
	bool pass = run_mfg_tests();
	alert_mode_set(pass ? ALERT_PASS : ALERT_FAIL);
	if (!pass) {
		console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_fail0);
		console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_fail1);
		console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_fail2);
	}
 	console_press_any_key();
	alert_mode_set(ALERT_NONE);
}

#ifndef PROGRAM_factory
__attribute__((section(".data")))
void fpga_core_reboot(void) {
	volatile uint8_t roml_bank = inportb(WS_CART_BANK_ROML_PORT);
	volatile uint16_t rom0_bank = inportw(WS_CART_EXTBANK_ROM0_PORT);
	volatile uint16_t rom1_bank = inportw(WS_CART_EXTBANK_ROM1_PORT);
	volatile uint16_t ram_bank = inportw(WS_CART_EXTBANK_RAM_PORT);
	volatile uint16_t bank_mask = inportw(NILE_SEG_MASK_PORT);

	// Start loading warmboot image
	outportb(IO_NILE_WARMBOOT_CNT, 0);
	// Configure HBlank timer to count down until 50ms have passed
	outportw(WS_TIMER_HBL_RELOAD_PORT, 601);
	outportw(WS_TIMER_CTRL_PORT, WS_TIMER_CTRL_HBL_ONESHOT);

	// Wait for FPGA init to finish
	while(inportw(WS_TIMER_HBL_COUNTER_PORT));

	// Restore some I/O ports
	outportb(NILE_POW_CNT_PORT, 0xDD);
	outportb(WS_CART_BANK_FLASH_PORT, 0);
	outportw(WS_CART_EXTBANK_ROM0_PORT, rom0_bank);
	outportw(WS_CART_EXTBANK_ROM1_PORT, rom1_bank);
	outportw(WS_CART_EXTBANK_RAM_PORT, ram_bank);
	outportb(WS_CART_BANK_ROML_PORT, roml_bank);
	outportw(NILE_SEG_MASK_PORT, bank_mask);
}
#endif

static const char __wf_rom* const __wf_rom menu_console_recovery[] = {
	s_disable_custom_splash,
	s_restore_tft_data,
	NULL
};

static void do_menu_console_recovery() {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_console_recovery);
		option = menu_run(menu_console_recovery, option);
		switch (option) {
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
		default:
			return;
		}
	}
}

static const char __wf_rom* const __wf_rom menu_card_mgmt[] = {
	s_tf_card_mount,
	s_benchmark_card_read_iram,
	s_benchmark_card_read_sram,
	s_benchmark_card_write,
	s_tf_card_format,
	NULL
};

static void do_menu_tf_card_mgmt(void) {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_tf_card_mgmt);
		option = menu_run(menu_card_mgmt, option);
		switch (option) {
		case 0:
			console_clear();
			op_tf_card_init(true, true);
			console_press_any_key();
			break;
		case 1:
			console_clear();
			op_tf_card_benchmark_read(TF_BENCH_BUFFER_IRAM);
			console_press_any_key();
			break;
		case 2:
			console_clear();
			op_tf_card_benchmark_read(TF_BENCH_BUFFER_PSRAM);
			console_press_any_key();
			break;
		case 3:
			console_clear();
			op_tf_card_benchmark_write(TF_BENCH_BUFFER_IRAM);
			console_press_any_key();
			break;
		case 4:
			console_clear();
			op_tf_card_format();
			console_press_any_key();
			break;
		default:
			return;
		}
	}
}

static const char __wf_rom* const __wf_rom menu_retention_tests[] = {
	s_sram_retention_test1,
	s_eeprom_retention_test1,
	s_sram_retention_test2,
	s_eeprom_retention_test2,
	NULL
};

static void do_menu_retention_tests() {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_retention_tests);
		option = menu_run(menu_retention_tests, option);
		switch (option) {
		case 0:
			console_clear();
			test_sram_retention(true);
			console_press_any_key();
			break;
		case 1:
			console_clear();
			test_eeprom_retention(true);
			console_press_any_key();
			break;
		case 2:
			console_clear();
			test_sram_retention(false);
			console_press_any_key();
			break;
		case 3:
			console_clear();
			test_eeprom_retention(false);
			console_press_any_key();
			break;
		default:
			return;
		}
	}
}

static const char __wf_rom* const __wf_rom menu_endurance_tests[] = {
	s_rtc_stability_test,
	s_sram_psram_stability_test,
	s_tf_card_stability_test,
	NULL
};

static void do_menu_endurance_tests() {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_endurance_tests);
		option = menu_run(menu_endurance_tests, option);
		switch (option) {
		case 0:
			console_clear();
			test_rtc_stability(0);
			console_press_any_key();
			break;
		case 1:
			console_clear();
			test_sram_psram_stability(0);
			console_press_any_key();
			break;
		case 2:
			console_clear();
			test_tf_card_stability(0);
			console_press_any_key();
			break;
		default:
			return;
		}
	}
}

static const char __wf_rom* const __wf_rom menu_fpga_tests[] = {
	s_flash_fsm_test,
	s_sram_32kb_test,
	NULL
};

static void do_menu_fpga_tests() {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_fpga_tests);
		option = menu_run(menu_fpga_tests, option);
		switch (option) {
		case 0:
			console_clear();
			test_flash_fsm();
			console_press_any_key();
			break;
		case 1:
			console_clear();
			test_sram_32kb();
			console_press_any_key();
			break;
		default:
			return;
		}
	}
}

static const char __wf_rom* const __wf_rom menu_mcu_tests[] = {
	s_mcu_accel_test,
	s_mcu_eeprom_test,
	s_rtc_clock_test,
	s_save_id_test,
	s_mcu_status_query,
	s_mcu_usb_cdc_echo,
	NULL
};

static void do_menu_mcu_tests() {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_mcu_tests);
		option = menu_run(menu_mcu_tests, option);
		switch (option) {
		case 0:
			console_clear();
			test_mcu_accelerometer();
			console_press_any_key();
			break;
		case 1:
			console_clear();
			test_mcu_eeprom();
			console_press_any_key();
			break;
		case 2:
			console_clear();
			test_rtc_clock();
			console_press_any_key();
			break;
		case 3:
			console_clear();
			test_mcu_save_id();
			console_press_any_key();
			break;
		case 4:
			console_clear();
			op_mcu_status_query();
			console_press_any_key();
			break;
		case 5:
			console_clear();
			test_mcu_usb_cdc_echo();
			console_press_any_key();
			break;
		default:
			return;
		}
	}
}

static const char __wf_rom* const __wf_rom menu_cartridge_diagnostics[] = {
	s_print_cartridge_info,
	s_print_cartridge_info_usb,
	s_print_save_info,
	s_retention_tests,
	s_endurance_tests,
	s_fpga_tests,
	s_mcu_tests,
	s_button_test,
	s_tf_card_mcu_insert_remove_test,
	NULL
};

static void do_menu_cartridge_diagnostics() {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_cartridge_diagnostics);
		option = menu_run(menu_cartridge_diagnostics, option);
		switch (option) {
		case 0:
		case 1: {
			uint16_t flags = (option == 1) ? (CONSOLE_FLAG_NO_SERIAL | CONSOLE_FLAG_MCU_SERIAL) : 0;
			console_clear();
			op_id_info_print_manual(flags);
			console_press_any_key();
		} break;
		case 2:
		    console_clear();
			op_id_print_save_info(0);
			console_press_any_key();
			break;
		case 3:
			do_menu_retention_tests();
			break;
		case 4:
			do_menu_endurance_tests();
			break;
		case 5:
			do_menu_fpga_tests();
			break;
		case 6:
			do_menu_mcu_tests();
			break;
		case 7:
			console_clear();
			test_button();
			console_press_any_key();
			break;
		case 8:
			console_clear();
			test_mcu_tf_insert_remove();
			console_press_any_key();
			break;
		default:
			return;
		}
	}
}

static const char __wf_rom* const __wf_rom menu_cartridge_recovery[] = {
	s_setup_mcu_boot_flags,
	s_flash_mcu_firmware,
	NULL
};

static void do_menu_cartridge_recovery() {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_cartridge_recovery);
		option = menu_run(menu_cartridge_recovery, option);
		switch (option) {
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
		default:
			return;
		}
	}
}

#ifdef CONFIG_ENABLE_DEV_FEATURES
static const char __wf_rom* const __wf_rom menu_dev_features[] = {
	s_dump_mcu_flash,
	s_dump_spi_flash,
	NULL
};

static void do_menu_dev_features() {
	int option = 0;
	while (true) {
		console_clear();
		console_draw_header(s_dev_features);
		option = menu_run(menu_dev_features, option);
		switch (option) {
		case 0:
			console_clear();
			op_mcu_setup_dump_flash();
			console_press_any_key();
			break;
		case 1:
			console_clear();
			op_mcu_setup_dump_spi_flash();
			console_press_any_key();
			break;
		default:
			return;
		}
	}
}
#endif

static void do_menu_main(void) {
	int option = 0;
	while (true) {
		console_clear();
	#ifdef NILESWAN_BRANDING
		console_draw_header(s_nileswan_recovery);
	#else
		console_draw_header(s_cart_recovery);
	#endif
		option = menu_run(menu_main, option);
		switch (option) {
		case 0:
			do_menu_console_recovery();
			break;
		case 1:
			do_menu_tf_card_mgmt();
			break;
		case 2:
			do_menu_cartridge_diagnostics();
			break;
		case 3:
			do_menu_cartridge_recovery();
			break;
		case 4:
			console_clear();
			do_mfg_tests();
			break;
		case 5:
			console_clear();
			console_print(0, s_license_header);
			console_press_any_key();
			break;
		case 6:
			nilefs_eject();
			ws_delay_ms(250);
			nile_soft_reset();
			break;
#ifdef CONFIG_ENABLE_DEV_FEATURES
		case 7:
			do_menu_dev_features();
			break;
#endif
		}
	}
}

void main(void) {
	cpu_irq_disable();

	outportb(WS_SYSTEM_CTRL_COLOR_PORT, 0x00);
	outportw(WS_DISPLAY_CTRL_PORT, 0);

#if 0
#ifndef PROGRAM_factory
	// HACK: I forgot to make ipl1/safe reboot the FPGA core to the updated version
	// before jumping to recovery. This chunk of code is a form of atonement.

	// Deinitialize TF card
	nilefs_eject();

	// Copy IPC -> RAM (it will be lost on FPGA reboot)
	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
	memcpy((void*) 0x2000, MK_FP(0x1000, 0x0000), 0x200);

	// Wake the flash before rebooting the FPGA core
	nile_flash_wake();

	// Save/restore I/O ports and reboot FPGA core
	fpga_core_reboot();

	// Restore state
	nile_flash_sleep();
	memcpy(MK_FP(0x1000, 0x0000), (void*) 0x2000, 0x200);
#endif
#endif

	nile_io_unlock();
	nile_bank_unlock();

	ws_hwint_set_handler(HWINT_IDX_VBLANK, vblank_int_handler);
	ws_hwint_enable(HWINT_VBLANK);
	cpu_irq_enable();
	console_init();

	fs.fs_type = 0;

	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
#ifdef PROGRAM_factory
	if (*((volatile uint16_t __far*) MK_FP(0x1000, 0x01FE)) == 0x3FA7) {
		do_mfg_tests();
	}
#endif

	do_menu_main();

	while(1);
}
