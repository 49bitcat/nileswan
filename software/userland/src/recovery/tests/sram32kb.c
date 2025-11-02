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

#include "sram32kb.h"
#include <nile.h>
#include <stdint.h>
#include <wonderful.h>
#include <ws.h>
#include "console.h"
#include "strings.h"

#define SRAM_FIRST_HALF ((volatile uint8_t __far*) MK_FP(0x1000, 0x0000))
#define SRAM_SECOND_HALF ((volatile uint8_t __far*) MK_FP(0x1000, 0x8000))

bool test_sram_32kb(void) {
    console_print_header(s_sram_32kb_test);

    outportw(IO_BANK_2003_RAM, 0x0);

    *SRAM_FIRST_HALF = 0x33;
    *SRAM_SECOND_HALF = 0x44;

    console_print(0, s_sram_no_mirroring);
    if (!console_print_status(*SRAM_FIRST_HALF == 0x33
        && *SRAM_SECOND_HALF == 0x44)) goto error;
    console_print_newline(0);

    outportb(IO_NILE_EMU_CNT, inportb(IO_NILE_EMU_CNT) | NILE_EMU_SRAM_32KB);

    *SRAM_FIRST_HALF = 0xAA;
    *SRAM_SECOND_HALF = 0xBB;

    console_print(0, s_sram_mirroring);
    if (!console_print_status(*SRAM_FIRST_HALF == 0xBB &&
        *SRAM_SECOND_HALF == 0xBB)) goto error;
    console_print_newline(0);

    outportb(IO_NILE_EMU_CNT, inportb(IO_NILE_EMU_CNT) & ~NILE_EMU_SRAM_32KB);

    return true;
error:
    return false;
}