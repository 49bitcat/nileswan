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

#include <wonderful.h>
#include <ws.h>

    .arch   i186
    .code16
    .intel_syntax noprefix

    .section .fartext.s.fetch_rtc_time, "ax"
    .global fetch_rtc_time
fetch_rtc_time:
    // AL = command to run
    // returns DX:AX = time
    push ax
    in al, 0xca
    test al, 0x10
    jnz error_still_active
    pop ax
    out 0xca, al

    // DX:CX = time
    xor dx, dx
    xor cx, cx

    // wait for bytes
1:
    in al, 0xca
    test al, 0x90
    jz 9f
    test al, 0x80
    jz 1b
    // read byte
    in al, 0xcb
    mov dl, ch
    mov ch, cl
    mov cl, al
    jmp 1b
9:
    mov ax, cx
    WF_PLATFORM_RET

error_still_active:
    mov dx, 0xFFFF
    mov dx, 0xFC00
    WF_PLATFORM_RET
