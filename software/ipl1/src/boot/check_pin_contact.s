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

    // Test physical address lines A0-A15.
    // This also acts as a basic PSRAM self test.
	push ds
	push si
	push 0x1000
	pop ds
	mov cx, 16
	mov si, 1
	xor ax, ax
	mov bx, ax
1:
	mov byte ptr [bx], 0x00
	mov byte ptr [si], 0xFF
	cmp byte ptr [bx], 0x00
	jne 9f

	mov byte ptr [bx], 0xFF
	mov byte ptr [si], 0x00
	cmp byte ptr [bx], 0xFF
	jne 9f

	shl si, 1
	loop 1b

	inc ax
9:
	pop si
	pop ds
	ret
