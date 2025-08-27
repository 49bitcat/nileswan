#include "mcu_setup.h"
#include <string.h>
#include <ws.h>
#include <nile.h>
#include "console.h"
#include "strings.h"
#include "cbin/recovery/firmware_bin.h"

#define MCU_FLASH_OPTR_ADDR 0x40022020U

static bool mcu_enter_bootloader_mode(void) {
    // A configured U0 requires a hard reset to boot into bootloader mode.
    console_print_newline();
    console_print(0, s_restarting_mcu);
    if (console_print_status(nile_mcu_reset(true))) {
        console_print_newline();
        return true;
    }

    return false;
}

static bool mcu_enter_bootloader_mode_maybe_unconfigured(void) {
    console_print_newline();
    console_print(0, s_wait_mcu_bootloader);

    // An unconfigured U0 will boot into bootloader mode by default.
    nile_spi_xch(NILE_MCU_BOOT_START);
    if (console_print_status(nile_mcu_boot_wait_ack())) {
        console_print_newline();
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
            console_print_newline();

            flash_optr[3] &= ~0x01; // Unset NBOOT_SEL
            flash_optr[2] |=  0x80; // Set BKPSRAM_HW_ERASE_DISABLE

            console_print(0, s_new_flash_optr);
            console_printf(0, s_format_4_bytes, flash_optr[3], flash_optr[2], flash_optr[1], flash_optr[0]);
            console_print_newline();

            console_print(0, s_writing_changes);
            if (console_print_status(nile_mcu_boot_write_memory(MCU_FLASH_OPTR_ADDR, flash_optr, sizeof(flash_optr)))) {
                result = true;
            }
        }
    }

    outportw(IO_NILE_SPI_CNT, prev_spi_cnt);
    console_print_newline();
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
            console_print_newline();
            console_print(0, s_writing);

            start_address += NILE_MCU_FLASH_START;
            end_address += NILE_MCU_FLASH_START;

            result = true;

            uint32_t i = 0;
            while (start_address < end_address) {
                uint32_t len = end_address - start_address;
                if (len > sizeof(verify_buffer)) len = sizeof(verify_buffer);

                if (!nile_mcu_boot_write_memory(start_address, firmware + i, len)) {
                    result = false;
                    break;
                }
                if (!nile_mcu_boot_read_memory(start_address, verify_buffer, len)) {
                    result = false;
                    break;
                }
                if (memcmp(verify_buffer, firmware + i, len)) {
                    result = false;
                    break;
                }

                start_address += len;
                i += len;
            }

            console_print_status(result);
        }
    }

    outportw(IO_NILE_SPI_CNT, prev_spi_cnt);
    console_print_newline();
    return result;
}
