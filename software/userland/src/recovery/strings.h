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

#ifndef STRINGS_H_
#define STRINGS_H_

#define DEFINE_STRING_LOCAL(name, value) static const char __far name[] = value
#ifdef STRINGS_H_IMPLEMENTATION
#define DEFINE_STRING(name, value) const char __far name[] = value
#else
#define DEFINE_STRING(name, value) extern const char __far name[]
#endif

DEFINE_STRING(s_cart_recovery, "cartridge recovery " VERSION);
DEFINE_STRING(s_nileswan_recovery, "nileswan recovery " VERSION);
DEFINE_STRING(s_nileswan_header,
    "     _ _\n"
    " _ _(_) |___ ____ __ ____ _ _ _\n"
    "| ' \\ | / -_|_-< V  V / _` | ' \\\n"
    "|_|_|_|_\\___/__/\\_/\\_/\\__,_|_|_|\n");
DEFINE_STRING(s_press_any_key_to_continue, "\nPress any button to continue...");
DEFINE_STRING(s_usb_post_restart_warning, "\nThe MCU has been restarted.\nPlease wait for the USB device to re-appear.");
DEFINE_STRING(s_error_code, "Error code %d");
DEFINE_STRING(s_format_1_u32, "%08lX");
DEFINE_STRING(s_format_1_long, "%ld");
DEFINE_STRING(s_format_4_bytes, "%02X%02X%02X%02X");

DEFINE_STRING(s_setup_mcu_boot_flags, "Configure MCU boot flags");
DEFINE_STRING(s_restarting_mcu, "Restarting MCU...");
DEFINE_STRING(s_wait_mcu_bootloader, "Waiting for bootloader...");
DEFINE_STRING(s_flash_optr, "FLASH_OPTR = ");
DEFINE_STRING(s_new_flash_optr, "New FLASH_OPTR = ");
DEFINE_STRING(s_writing_changes, "Writing changes...");

DEFINE_STRING(s_flash_mcu_firmware, "Flash MCU firmware");
DEFINE_STRING(s_writing, "Writing...");
DEFINE_STRING(s_erasing, "Erasing...");

DEFINE_STRING(s_print_cartridge_ids, "Display cartridge IDs");
DEFINE_STRING(s_print_cartridge_info, "Display cartridge information");
DEFINE_STRING(s_print_cartridge_info_usb, "Print cartridge information (-> USB)");
DEFINE_STRING(s_flash_jedec_id, "SPI flash JEDEC ID: ");
DEFINE_STRING(s_flash_uuid, "SPI flash UID: ");
DEFINE_STRING(s_mcu_uuid, "MCU UID: ");
DEFINE_STRING(s_board_rev, "Board revision ID: %02X");
DEFINE_STRING(s_version_manifest_update, "Version (update): ");
DEFINE_STRING(s_version_manifest_factory, "Version (factory): ");
DEFINE_STRING(s_version_mcu_protocol, "MCU protocol: ");
DEFINE_STRING(s_version_mcu_protocol1, "%d.%d");
DEFINE_STRING(s_version_mcu_protocol2, " +");
DEFINE_STRING(s_version_mcu_protocol3, " %02x");
DEFINE_STRING(s_version_manifest_line1, "%d.%d.%d%c(%02x%02x%02x%02x)");

DEFINE_STRING(s_print_save_info, "Display battery backup status");
DEFINE_STRING(s_save_battery_ok, "Battery status: Present");
DEFINE_STRING(s_save_battery_no, "Battery status: Not present");
DEFINE_STRING(s_save_id_ram, "Save ID (MCU RAM2): %02x%02x%02x%02x");
DEFINE_STRING(s_save_id_rtc, "Save ID (RTC/SRAM): %02x%02x%02x%02x");

DEFINE_STRING(s_tf_card_init, "Initializing storage...");
DEFINE_STRING(s_tf_card_fs_init, "Initializing filesystem...");
DEFINE_STRING(s_tf_card_type, "Medium type: %d\n");
DEFINE_STRING(s_tf_card_size, "Storage size: 512b x ");
DEFINE_STRING(s_tf_card_block_size, "Block size: 512b x ");
DEFINE_STRING(s_tf_card_formatting, "Formatting...");
DEFINE_STRING(s_tf_card_format_warning, "THIS ACTION FORMATS THE STORAGE CARD.\nALL DATA ON THE CARD WILL BE LOST!\n\n");
DEFINE_STRING(s_proceed, "Do you want to proceed? A - Yes; other - No");

DEFINE_STRING(s_manual_shutdown, "The console may now be powered off.");

DEFINE_STRING(s_model_unsupported, "This action is not supported on this console.\n");
DEFINE_STRING(s_warranty_disclaimer, "This action modifies sensitive data.\nYou do so at your own risk.\n\n");
DEFINE_STRING(s_warranty_disclaimer2, "By choosing to continue, you agree that the developers of this software will not be held responsible for any damage or loss resulting from the use of this action.\n\n");

DEFINE_STRING(s_console_recovery, "Console recovery >");
DEFINE_STRING(s_ieeprom_writing, "Writing to internal EEPROM...");
DEFINE_STRING(s_disable_custom_splash, "Disable custom boot splash");
DEFINE_STRING(s_custom_splash_already_disabled, "Custom boot splash already disabled!");
DEFINE_STRING(s_restore_tft_data, "Write new TFT panel timing data");

DEFINE_STRING(s_cartridge_diagnostics, "Cartridge diagnostics >");
DEFINE_STRING(s_cartridge_recovery, "Cartridge recovery >");
DEFINE_STRING(s_retention_tests, "Retention tests >");
DEFINE_STRING(s_endurance_tests, "Endurance tests >");
DEFINE_STRING(s_mcu_tests, "MCU behaviour tests >");
DEFINE_STRING(s_fpga_tests, "FPGA behaviour tests >");
DEFINE_STRING(s_dev_features, "Development >");

DEFINE_STRING(s_flash_fsm_test, "Flash emulation test");
DEFINE_STRING(s_flash_fsm_test_no, "Flash emulation test #%d");
DEFINE_STRING(s_flash_fsm_last_byte, "\nLast byte = %02X\n");

DEFINE_STRING(s_hold_b_to_abort, " (hold B to abort)");
DEFINE_STRING(s_rtc_clock_test, "RTC clock test");
DEFINE_STRING(s_rtc_stability_test, "RTC stability test");
DEFINE_STRING(s_rebooting_mcu, "Rebooting MCU...");
DEFINE_STRING(s_switching_rtc, "Switching to RTC mode...");
DEFINE_STRING(s_resetting_rtc, "Resetting RTC...");
DEFINE_STRING(s_setting_rtc_time, "Setting RTC date/time...");
DEFINE_STRING(s_verifying_time_change, "Verifying time change...");
DEFINE_STRING(s_rtc_stability_read_failed, " read failed @ %d (error %d)");
DEFINE_STRING(s_rtc_stability_value_mismatch, " mismatch @ %d: %08lX then %08lX");
DEFINE_STRING(s_rtc_stability_value_invalid, " invalid value @ %d: %08lX");
DEFINE_STRING(s_out_of_range, " out of range");

DEFINE_STRING(s_button_test, "Onboard button test");
DEFINE_STRING(s_button_press, "[!] Please press onboard button");
DEFINE_STRING(s_button_release, "[!] Please release onboard button");
DEFINE_STRING(s_press_b_to_abort, "Press B to abort");

DEFINE_STRING(s_sram_32kb_test, "SRAM mirroring emulation test");
DEFINE_STRING(s_sram_no_mirroring, "No mirroring");
DEFINE_STRING(s_sram_mirroring, "Mirroring");
DEFINE_STRING(s_sram_psram_stability_test, "SRAM<->PSRAM stability test");

DEFINE_STRING(s_dump_mcu_flash, "Dump MCU flash");
DEFINE_STRING(s_dump_mcu_flash_path, "/NILE_MCU.BIN");
DEFINE_STRING(s_dump_spi_flash, "Dump SPI flash");
DEFINE_STRING(s_dump_spi_flash_path, "/NILE_SPI.BIN");
DEFINE_STRING(s_initializing_spi_flash, "Initializing SPI flash...");
DEFINE_STRING(s_reading, "Reading...");

DEFINE_STRING(s_mcu_usb_cdc_echo, "MCU USB CDC port test (echo)");
DEFINE_STRING(s_mcu_accel_test, "MCU accelerometer poll test");
DEFINE_STRING(s_mcu_accel_axis, " %c = %d");

DEFINE_STRING(s_mcu_status_query, "Status information query");
DEFINE_STRING(s_mcu_status_query_caps, "capabilities = %02X");
DEFINE_STRING(s_mcu_status_query_status, "status = %02X");
DEFINE_STRING(s_mcu_status_query_voltage, "raw battery voltage = %04X");

DEFINE_STRING(s_mcu_eeprom_test, "EEPROM emulation test");
DEFINE_STRING(s_switching_eeprom, "Switching to EEPROM mode...");
DEFINE_STRING(s_eeprom_test_write, "Writing data to EEPROM...");
DEFINE_STRING(s_eeprom_test_check, "Checking MCU state...");
DEFINE_STRING(s_eeprom_test_read, "Reading data from MCU...");
DEFINE_STRING(s_eeprom_test_failed, " failed @ stage %d, offset %d [%04X, %04X]");

DEFINE_STRING(s_caps_initialization, "INITIALIZATION");
DEFINE_STRING(s_caps_test_suite, "AUTOMATED TEST SUITE");
DEFINE_STRING(s_caps_test_suite_manual, "MANUAL TEST SUITE");
DEFINE_STRING(s_caps_information, "INFORMATION");
DEFINE_STRING(s_mfg_test_success0, "*********************\n");
DEFINE_STRING(s_mfg_test_success1, "*                   *\n");
DEFINE_STRING(s_mfg_test_success2, "* All tests passed! *\n");
DEFINE_STRING(s_mfg_test_alert2,   "* Hold the console, *\n");
DEFINE_STRING(s_mfg_test_alert3,   "* press any button. *\n");
DEFINE_STRING(s_run_manufacturing_test, "Run manufacturing tests");

DEFINE_STRING(s_mfg_fail0, " _  _  . .  .\n");
DEFINE_STRING(s_mfg_fail1, "|_ |_| | |  |\n");
DEFINE_STRING(s_mfg_fail2, "|  | | | |_ .");

DEFINE_STRING(s_tf_card_mgmt, "Storage card management >");
DEFINE_STRING(s_tf_card_mount, "Mount storage card");
DEFINE_STRING(s_tf_card_format, "Format storage card");
DEFINE_STRING(s_tf_card_stability_test, "Storage card stability test");
DEFINE_STRING(s_benchmark_card_read, "Benchmark card read");
DEFINE_STRING(s_benchmark_card_read_iram, "Benchmark card read (-> IRAM)");
DEFINE_STRING(s_benchmark_card_read_sram, "Benchmark card read (-> SRAM)");
DEFINE_STRING(s_benchmark_card_write, "Benchmark card write");
DEFINE_STRING(s_benchmark_preparing_test_file, "Preparing test file... ");
DEFINE_STRING(s_benchmark_reading_bytes, "Reading %d bytes... ");
DEFINE_STRING(s_benchmark_writing_bytes, "Writing %d bytes... ");
DEFINE_STRING(s_benchmark_data_read_mismatch, "Data read mismatch");
DEFINE_STRING(s_benchmark_hblanks, "%d hbl (%d KB/s)");
DEFINE_STRING(s_d, "%d");
DEFINE_STRING(s_tf_card_mcu_insert_remove_test, "Storage card insert/removal detection test");
DEFINE_STRING(s_tf_card_remove, "[!] Please remove TF card");
DEFINE_STRING(s_tf_card_insert, "[!] Please insert TF card");

DEFINE_STRING(s_sram_retention_test1, "SRAM retention test (first boot)");
DEFINE_STRING(s_sram_retention_test2, "SRAM retention test (second boot)");
DEFINE_STRING(s_eeprom_retention_test1, "EEPROM retention test (first boot)");
DEFINE_STRING(s_eeprom_retention_test2, "EEPROM retention test (second boot)");
DEFINE_STRING(s_sram_read_error, "\nError @ %01X%04X: expected %02X, actual %02X");
DEFINE_STRING(s_eeprom_read_error, "\nError @ %03X: expected %02X, actual %02X");
DEFINE_STRING(s_mcu_communication_error, "\nMCU communication error");
DEFINE_STRING(s_retention_test_writing, "Writing data for readback...");
DEFINE_STRING(s_retention_test_reading, "Reading back data...");
DEFINE_STRING(s_retention_test_passed, "Readback test passed");

DEFINE_STRING(s_exit, "Exit");

DEFINE_STRING(s_license, "About");
DEFINE_STRING(s_license_header, "This program is free software: you can\nredistribute it and/or modify it under the terms\nof the GNU General Public License as published by\nthe Free Software Foundation, either version 3\nof the License, or (at your option) any later\nversion.\n\nThis program is distributed in the hope that it\nwill be useful, but WITHOUT ANY WARRANTY;\nwithout even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR\nPURPOSE. See the GNU General Public License for\nmore details.\n");

#endif /* STRINGS_H_ */
