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

#include "id_print.h"
#include <nile.h>
#include <nile/mcu.h>
#include "console.h"
#include "main.h"
#include "strings.h"

#define MCU_UID_BASE 0x1FFF6E50
#define MCU_UID_SIZE 12

bool op_id_print(uint16_t flags) {
    uint8_t buf[16];
    bool flash_wake = false;
    bool result = true;

    console_print_header(s_print_cartridge_ids);

    console_printf(flags, s_board_rev, inportb(IO_NILE_BOARD_REVISION));
    console_print_newline(flags);

    console_print(flags, s_flash_jedec_id);
    flash_wake = console_print_status(nile_flash_wake());
    if (flash_wake) {
        uint32_t flash_jedec_id = nile_flash_read_id();
        console_printf(flags, s_format_1_u32, flash_jedec_id);
    } else {
        result = false;
    }
    console_print_newline(flags);

    console_print(flags, s_flash_uuid);
    if (console_print_status(flash_wake && nile_flash_read_uuid(buf))) {
        console_printf(flags, s_format_4_bytes, buf[0], buf[1], buf[2], buf[3]);
        console_printf(flags, s_format_4_bytes, buf[4], buf[5], buf[6], buf[7]);
    } else {
        result = false;
    }
    console_print_newline(flags);

    if (flags & CONSOLE_FLAG_MCU_SERIAL) {
        console_print(flags, s_mcu_uuid);
        if (!console_print_status(nile_mcu_native_mcu_get_uuid_sync(buf, 12) >= 12)) {
            result = false;
        }
    } else {
        console_print(flags, s_restarting_mcu);
        if (console_print_status(nile_mcu_reset(true))) {
            console_clear_current_line(flags);
            console_print(flags, s_mcu_uuid);
            if (!console_print_status(nile_mcu_boot_read_memory(MCU_UID_BASE, buf, MCU_UID_SIZE))) {
                result = false;
            }
        } else {
            result = false;
        }
    }

    if (result) {
        console_printf(flags, s_format_4_bytes, buf[11], buf[10], buf[9], buf[8]);
        console_printf(flags, s_format_4_bytes, buf[7], buf[6], buf[5], buf[4]);
        console_printf(flags, s_format_4_bytes, buf[3], buf[2], buf[1], buf[0]);
    }

    console_print_newline(flags);
    return result;
}

static bool op_info_print_manifest(uint16_t flags, uint32_t addr) {
	nile_flash_manifest_t manifest;
    bool result = false;

    if (nile_flash_wake()) {
        if (nile_flash_read(&manifest, addr, sizeof(manifest)) && manifest.version.id == NILE_FLASH_MANIFEST_ID) {
            result = true;
            
			console_printf(flags, s_version_manifest_line1,
				manifest.version.major,
				manifest.version.minor,
				manifest.version.patch,
				manifest.version.partial_install ? '!' : ' ',
				(int) manifest.commit_id[0],
				(int) manifest.commit_id[1],
				(int) manifest.commit_id[2],
				(int) manifest.commit_id[3]
			);
        }
    }

    if (!result) {
        console_print_status(false);
    }
    console_print_newline(flags);

    return result;
}

static bool op_info_print_mcu_version(uint16_t flags) {
	uint8_t buffer[64];
    bool result = false;

    if ((flags & CONSOLE_FLAG_MCU_SERIAL) || nile_mcu_reset(false)) {
        if (!nile_mcu_native_send_cmd(NILE_MCU_NATIVE_CMD(0x0F, 0), NULL, 0)) {
            int16_t bytes = nile_mcu_native_recv_cmd(buffer, sizeof(buffer));
            if (bytes >= 4) {
                result = true;
                
                console_printf(flags, s_version_mcu_protocol1,
                    ((uint16_t*) buffer)[0],
                    ((uint16_t*) buffer)[1]);
                if (bytes > 4) {
                    console_print(flags, s_version_mcu_protocol2);
                    for (int i = 4; i < bytes; i++) {
                        console_printf(flags, s_version_mcu_protocol3, (int) buffer[i]);
                    }
                }
            }
        }
    }

    if (!result) {
        console_print_status(false);
    }
    console_print_newline(flags);

    return result;
}

bool op_info_print(uint16_t flags) {
    console_print_header(s_print_cartridge_info);

    console_print(flags, s_version_manifest_update);
    op_info_print_manifest(flags, NILE_FLASH_LAYOUT_MANIFEST_ADDR);

    console_print(flags, s_version_manifest_factory);
    op_info_print_manifest(flags, NILE_FLASH_LAYOUT_MANIFEST_FACTORY_ADDR);

    console_print(flags, s_version_mcu_protocol);
    op_info_print_mcu_version(flags);

    return true;
}

bool op_id_info_print_manual(uint16_t flags) {
    if (flags & CONSOLE_FLAG_MCU_SERIAL) {
        console_print(flags & ~CONSOLE_FLAG_MCU_SERIAL, s_restarting_mcu);
    
        nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);
        if (!nile_mcu_reset(false)) {
            console_print_status(false);
            return false;
        }
        console_putc(flags & ~CONSOLE_FLAG_MCU_SERIAL, '.');
        ws_delay_ms(1000);
        console_print_status(true);

        console_print(flags & ~CONSOLE_FLAG_MCU_SERIAL, s_usb_post_restart_warning);
        console_press_any_key();
        
        console_print_newline(flags);
    }
    console_print(CONSOLE_FLAG_NO_SERIAL | CONSOLE_FLAG_MONOSPACE, s_nileswan_header);
    console_print_newline(flags);
    op_id_print(flags);
    console_print_newline(flags);
    op_info_print(flags);
    return true;
}
