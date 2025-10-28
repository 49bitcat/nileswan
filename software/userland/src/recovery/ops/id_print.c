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
#include "console.h"
#include "strings.h"

#define MCU_UID_BASE 0x1FFF6E50
#define MCU_UID_SIZE 12

bool op_id_print(void) {
    uint8_t buf[16];
    bool flash_wake = false;
    bool result = true;

    console_print_header(s_print_cartridge_ids);

    console_printf(0, s_board_rev, inportb(IO_NILE_BOARD_REVISION));
    console_print_newline();

    console_print(0, s_flash_jedec_id);
    flash_wake = console_print_status(nile_flash_wake());
    if (flash_wake) {
        uint32_t flash_jedec_id = nile_flash_read_id();
        console_printf(0, s_format_1_u32, flash_jedec_id);
    } else {
        result = false;
    }
    console_print_newline();

    console_print(0, s_flash_uuid);
    if (console_print_status(flash_wake && nile_flash_read_uuid(buf))) {
        console_printf(0, s_format_4_bytes, buf[0], buf[1], buf[2], buf[3]);
        console_printf(0, s_format_4_bytes, buf[4], buf[5], buf[6], buf[7]);
    } else {
        result = false;
    }
    console_print_newline();

    console_print(0, s_restarting_mcu);

    if (console_print_status(nile_mcu_reset(true))) {
        console_print_newline();
        console_print(0, s_mcu_uuid);
        if (console_print_status(nile_mcu_boot_read_memory(MCU_UID_BASE, buf, MCU_UID_SIZE))) {
            console_printf(0, s_format_4_bytes, buf[11], buf[10], buf[9], buf[8]);
            console_printf(0, s_format_4_bytes, buf[7], buf[6], buf[5], buf[4]);
            console_printf(0, s_format_4_bytes, buf[3], buf[2], buf[1], buf[0]);
        } else {
            result = false;
        }
    } else {
        result = false;
    }

    console_print_newline();
    return result;
}

static bool op_info_print_manifest(uint32_t addr) {
	nile_flash_manifest_t manifest;
    bool result = false;

    if (nile_flash_wake()) {
        if (nile_flash_read(&manifest, addr, sizeof(manifest)) && manifest.id == NILE_FLASH_MANIFEST_ID) {
            result = true;
            
			console_printf(0, s_version_manifest_line1,
				manifest.major,
				manifest.minor,
				manifest.patch,
				manifest.partial_install ? '!' : ' ',
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
    console_print_newline();

    return result;
}

bool op_info_print(void) {
    console_print_header(s_print_cartridge_info);

    console_print(0, s_version_manifest_update);
    op_info_print_manifest(NILE_FLASH_LAYOUT_MANIFEST_ADDR);

    console_print(0, s_version_manifest_factory);
    op_info_print_manifest(NILE_FLASH_LAYOUT_MANIFEST_FACTORY_ADDR);

    return true;
}
