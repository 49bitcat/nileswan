/**
 * Copyright (c) 2026 Adrian "asie" Siekierka
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

#include <wonderful.h>
#include <ws.h>

	.arch	i186
	.code16
	.intel_syntax noprefix

	.section .text, "ax"
	.global check_pin_contact
check_pin_contact:
    // A16-A19 and A0-A3 are checked by the console SoC
    // D0-D15 are largely implicitly checked by the console IPL, then the IPL0
    // A4-A8 are implicitly checked by the IPL0
    // However, IPL1 can still detect and warn the user about A9-A15 pin contact problems

    xor ax, ax

    // Test physical address lines A8-A15.
	push ds
	push 0x1000
	pop ds

	// Write 0x00..0xFF to addresses 0x0000..0xFF00
	xor bx, bx
1:
	mov [bx], bh
	inc bh
	jnz 1b

	// Read 0x00..0xFF from addresses 0x0000..0xFF00
1:
	cmp [bx], bh
	jne 9f
	inc bh
	jnz 1b

	// Return true
	inc ax
9:
	pop ds
	ret
