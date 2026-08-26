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

#include "mcu_setup.h"
#include <nile/flash.h>
#include <nile/mcu.h>
#include <nilefs/ff.h>
#include <string.h>
#include <ws.h>
#include <nile.h>
#include "console.h"
#include "strings.h"
#include "cbin/recovery/firmware_bin.h"
#include "tf_card.h"

#define MCU_FLASH_OPTR_ADDR 0x40022020U

static bool mcu_enter_bootloader_mode(void) {
    // A configured U0 requires a hard reset to boot into bootloader mode.
    console_print_newline(0);
    console_print(0, s_restarting_mcu);
    if (console_print_status(nile_mcu_reset(true))) {
        console_print_newline(0);
        return true;
    }

    return false;
}

static bool mcu_enter_bootloader_mode_maybe_unconfigured(void) {
    console_print_newline(0);
    console_print(0, s_wait_mcu_bootloader);

    // An unconfigured U0 will boot into bootloader mode by default.
    nile_spi_xch(NILE_MCU_BOOT_START);
    if (console_print_status(nile_mcu_boot_wait_ack())) {
        console_print_newline(0);
        return true;
    }

    return mcu_enter_bootloader_mode();
}

bool op_mcu_setup_boot_flags(void) {
    bool result = false;
    uint16_t prev_spi_cnt = inportw(IO_NILE_SPI_CNT);
    outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    console_print_header(s_setup_mcu_boot_flags);

    if (mcu_enter_bootloader_mode_maybe_unconfigured()) {
        uint8_t flash_optr[4];

        console_print(0, s_flash_optr);
        if (console_print_status(nile_mcu_boot_read_memory(MCU_FLASH_OPTR_ADDR, flash_optr, sizeof(flash_optr)))) {
            console_printf(0, s_format_4_bytes, flash_optr[3], flash_optr[2], flash_optr[1], flash_optr[0]);
            console_print_newline(0);

            flash_optr[3] &= ~0x01; // Unset NBOOT_SEL
            flash_optr[2] |=  0x80; // Set BKPSRAM_HW_ERASE_DISABLE

            flash_optr[1] &= ~0x07; // Disable BOR

            console_print(0, s_new_flash_optr);
            console_printf(0, s_format_4_bytes, flash_optr[3], flash_optr[2], flash_optr[1], flash_optr[0]);
            console_print_newline(0);

            console_print(0, s_writing_changes);
            if (console_print_status(nile_mcu_boot_write_memory(MCU_FLASH_OPTR_ADDR, flash_optr, sizeof(flash_optr)))) {
                result = true;
            }
        }
    }

    outportw(IO_NILE_SPI_CNT, prev_spi_cnt);
    console_print_newline(0);
    return result;
}

bool op_mcu_setup_flash_firmware(void) {
    uint8_t verify_buffer[128];

    bool result = false;
    uint16_t prev_spi_cnt = inportw(IO_NILE_SPI_CNT);
    outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    console_print_header(s_flash_mcu_firmware);

    if (mcu_enter_bootloader_mode()) {
        console_print(0, s_erasing);

        uint32_t start_address = 0;
        uint32_t end_address = start_address + firmware_size;
        uint16_t page_start = start_address / NILE_MCU_FLASH_PAGE_SIZE;
        uint16_t page_count = (end_address + NILE_MCU_FLASH_PAGE_SIZE - 1 - start_address) / NILE_MCU_FLASH_PAGE_SIZE;

        if (console_print_status(nile_mcu_boot_erase_memory(page_start, page_count))) {
            console_print_newline(0);
            console_print(0, s_writing);

            start_address += NILE_MCU_FLASH_START;
            end_address += NILE_MCU_FLASH_START;

            result = true;

            const uint8_t __far *ptr = firmware;
            while (start_address < end_address) {
                uint32_t len = end_address - start_address;
                if (len > sizeof(verify_buffer)) len = sizeof(verify_buffer);

                if (!nile_mcu_boot_write_memory(start_address, ptr, len)) {
                    result = false;
                    break;
                }
                if (!nile_mcu_boot_read_memory(start_address, verify_buffer, len)) {
                    result = false;
                    break;
                }
                if (memcmp(verify_buffer, ptr, len)) {
                    result = false;
                    break;
                }

                start_address += len;
                ptr = MK_FP(FP_SEG(ptr) + (len >> 4), FP_OFF(ptr));
            }

            console_print_status(result);
        }
    }

    outportw(IO_NILE_SPI_CNT, prev_spi_cnt);
    console_print_newline(0);
    return result;
}

bool op_mcu_setup_dump_flash(void) {
    uint8_t verify_buffer[128];
    FIL fp;

    bool result = false;
    uint16_t prev_spi_cnt = inportw(IO_NILE_SPI_CNT);
    outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    console_print_header(s_dump_mcu_flash);

    if (op_tf_card_init(false, true)) {
        if (mcu_enter_bootloader_mode()) {
            console_print(0, s_reading);

            uint32_t start_address = NILE_MCU_FLASH_START;
            uint32_t end_address = NILE_MCU_FLASH_START + (256L*1024L);

            result = true;
            strcpy(verify_buffer, s_dump_mcu_flash_path);
            if (f_open(&fp, verify_buffer, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
                result = false;
            } else {
                int i = 0;

                while (start_address < end_address) {
                    uint32_t len = end_address - start_address;
                    uint16_t bw;
                    if (len > sizeof(verify_buffer)) len = sizeof(verify_buffer);

                    if (!nile_mcu_boot_read_memory(start_address, verify_buffer, len)) {
                        result = false;
                        break;
                    }
                    if (f_write(&fp, verify_buffer, len, &bw) != FR_OK) {
                        result = false;
                        break;
                    }

                    start_address += len;

                    // 8 steps per KB
                    if (!((++i) & (128 - 1))) console_putc(0, '.');
                }
                if (f_close(&fp) != FR_OK) {
                    result = false;
                }
            }

            console_print_status(result);
        }
    }

    outportw(IO_NILE_SPI_CNT, prev_spi_cnt);
    console_print_newline(0);
    return result;
}

bool op_mcu_setup_dump_spi_flash(void) {
    uint8_t verify_buffer[128];
    FIL fp;

    bool result = false;
    uint16_t prev_spi_cnt = inportw(IO_NILE_SPI_CNT);
    outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    console_print_header(s_dump_spi_flash);

    console_print(0, s_initializing_spi_flash);
    if (console_print_status(nile_flash_wake())) {
        console_print_newline(0);
        console_print(0, s_reading);

        uint32_t start_address = 0;
        uint32_t end_address = 2048L*1024L;

        result = true;
        strcpy(verify_buffer, s_dump_spi_flash_path);
        if (f_open(&fp, verify_buffer, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
            result = false;
        } else {
            int i = 0;

            while (start_address < end_address) {
                uint32_t len = end_address - start_address;
                uint16_t bw;
                if (len > sizeof(verify_buffer)) len = sizeof(verify_buffer);

                if (!nile_flash_read(verify_buffer, start_address, len)) {
                    result = false;
                    break;
                }
                if (f_write(&fp, verify_buffer, len, &bw) != FR_OK) {
                    result = false;
                    break;
                }

                start_address += len;

                // 8 steps per KB
                if (!((++i) & (1024 - 1))) console_putc(0, '.');
            }
            if (f_close(&fp) != FR_OK) {
                result = false;
            }
        }

        console_print_status(result);
    }

    outportw(IO_NILE_SPI_CNT, prev_spi_cnt);
    console_print_newline(0);
    return result;
}
