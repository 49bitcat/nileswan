/**
 * Copyright (c) 2024 Adrian "asie" Siekierka
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

#define INDICATOR_ADDR 0x3FB6

	.arch	i186
	.code16
	.intel_syntax noprefix

	.section .text, "ax"
    // ax - bank count

    // uses ax, cx
.macro xorshift_ax_cx
    // x ^= x << 7
    mov cx, ax
    shl cx, 7
    xor ax, cx

    // x ^= x >> 9
    mov cl, ah
    shr cl, 1
    xor al, cl

    // x ^= x << 8
    xor ah, al
.endm

    // ax = pointer to result structure (of size dx bytes)
    // dx = number of banks to test
	.global mem_test_deep
mem_test_deep:
    push ds
    push es
    push si
    push di
    mov bx, ax
    mov ax, 0x1000
    mov ds, ax
    mov es, ax

    cld
    call mem_test_deep_perform

    pop di
    pop si
    pop es
    pop ds
    IA16_RET

mem_test_deep_perform:
    push dx
    push dx

    xor di, di

    // test read only?
    ss cmp byte ptr [mem_test_deep_mode], 1
    je mem_test_deep_read_start

mem_test_deep_write_outer_loop:
    // dx = dx - 1, bank = dx
    mov ax, dx
    dec ax
    out WS_CART_EXTBANK_RAM_PORT, ax
    mov dx, ax
    // initialize random value
    add ax, 12345
mem_test_deep_write_loop:
.rept 4
    // store random word to memory
    stosw
    xorshift_ax_cx
.endr
    // have we finished the page?
    test di, di
    jnz mem_test_deep_write_loop

    // increment indicator
    ss mov cx, word ptr [INDICATOR_ADDR]
    inc cx
    or cx, 0x140
    and cx, 0x17F
    ss mov word ptr [INDICATOR_ADDR], cx

    // have we finished all pages?
    test dx, dx
    jnz mem_test_deep_write_outer_loop

mem_test_deep_read_start:
    // restore bank counter
    pop dx
mem_test_deep_read_outer_loop:
    // dx = dx - 1, bank = dx
    mov ax, dx
    dec ax
    out WS_CART_EXTBANK_RAM_PORT, ax
    mov dx, ax
    // initialize random value
    add ax, 12345
mem_test_deep_read_loop:
.rept 4
    // compare memory with random word
    scasw
    // is there a difference?
    jnz mem_test_deep_read_found
4:
    // advance PRNG
    xorshift_ax_cx
.endr
    // have we finished the page?
    test di, di
    jnz mem_test_deep_read_loop
mem_test_deep_read_page_done:
    // write "no error" result
    // ... unless test mode inhibits
    ss cmp byte ptr [mem_test_deep_mode], 254
    jae mem_test_deep_read_page_done_error
    // ... unless error already printed
    cmp word ptr ss:[bx], 0x0121
    je mem_test_deep_read_page_done_error
    mov word ptr ss:[bx], 0x012E
mem_test_deep_read_page_done_error:
    call mem_test_deep_incr_bx

    ss mov cx, word ptr [INDICATOR_ADDR]
    inc cx
    or cx, 0x140
    and cx, 0x17F
    ss mov word ptr [INDICATOR_ADDR], cx

    // have we finished all pages?
    test dx, dx
    jnz mem_test_deep_read_outer_loop
    // restore bank counter
    pop dx
    ret

mem_test_deep_read_found_skip:
    ss mov byte ptr [mem_test_deep_mode], 255
    pop dx
    ret

mem_test_deep_read_found:
    // write "error" result
    // ... unless test mode inhibits
    ss cmp byte ptr [mem_test_deep_mode], 254
    jae mem_test_deep_read_found_skip
    mov word ptr ss:[bx], 0x0121
    // write "error" location
    pusha
    // dx = bank
    mov word ptr ss:[0x3F90], 0x013F
    mov ax, 0x3F80
    call print_hex_number
    // di = offset + 2
    mov dx, di
    dec dx
    dec dx
    call print_hex_number

    // wait for keypress
mem_test_deep_read_found_keypress:
    call ws_keypad_scan
    and ax, 0x0DDD
    jz mem_test_deep_read_found_keypress
    push ax
mem_test_deep_read_found_keypress2:
    call ws_keypad_scan
    and ax, 0x0DDD
    jnz mem_test_deep_read_found_keypress2
    pop ax
    mov word ptr ss:[0x3F90], 0x0120
    test ah, 0x0F
    jnz mem_test_deep_read_clear_bank_only
    popa
    // clear pointer, read next page
    xor di, di
    jmp mem_test_deep_read_page_done_error
mem_test_deep_read_clear_bank_only:
    popa
    jmp 4b

mem_test_deep_incr_bx:
    mov cx, bx
    add bx, 2
    xor cx, bx
    and cx, 0x20
    jz mem_test_deep_incr_bx_end
    add bx, 32
mem_test_deep_incr_bx_end:
    ret

    .section .data, "a"
    // 0 (default) - print tiles, stop on every read
    // 1 - only do read test
    // 254 - set test mode to 255 on failure
    .global mem_test_deep_mode
mem_test_deep_mode:
    .byte 0
