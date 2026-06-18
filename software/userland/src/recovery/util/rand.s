/**
 * Copyright (c) 2026 Adrian "asie" Siekierka
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

#include <wonderful.h>
#include <ws.h>

    .arch   i186
    .code16
    .intel_syntax noprefix

    // uses ax, bx
.macro xorshift_ax_bx
.endm

	// AX = buffer value
	// DX:CX = buffer address
    .section .fartext.s.xorshift_fill_128b, "ax"
    .global xorshift_fill_128b
xorshift_fill_128b:
	push es
	push di
	push cx
	pop es

	mov di, dx
	mov cx, 64

1:
    // x ^= x << 7
    mov bx, ax
    shl bx, 7
    xor ax, bx

    // x ^= x >> 9
    mov bl, ah
    shr bl, 1
    xor al, bl

    // x ^= x << 8
    xor ah, al

    stosw

    loop 1b

	pop di
	pop es
	WF_PLATFORM_RET
