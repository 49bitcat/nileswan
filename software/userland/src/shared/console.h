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

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stdarg.h>
#include <stddef.h>
#include <wonderful.h>
#include <ws.h>

#define CONSOLE_LINE_COUNT 16

#define CONSOLE_FLAG_CENTER (1 << 0)
#define CONSOLE_FLAG_RIGHT (1 << 1)
#define CONSOLE_FLAG_HIGHLIGHT (1 << 2)
#define CONSOLE_FLAG_MONOSPACE (1 << 3)
#define CONSOLE_FLAG_NO_SERIAL (1 << 4)

void console_init(void);
void console_draw_header(const char __far* str);
void console_print_header(const char __far* str);
void console_clear(void);
void console_clear_lines(int y, int count);
void console_clear_current_line(void);
void console_draw_newline(void);
void console_print_newline(void);
int console_draw(int x, int y, uint16_t flags, const char __far* str);
int console_vdrawf(int x, int y, uint16_t flags, const char __far* format, va_list val);
int console_drawf(int x, int y, uint16_t flags, const char __far* format, ...);
void console_putc(uint16_t flags, uint16_t ch);
void console_print(uint16_t flags, const char __far* str);
void console_vprintf(uint16_t flags, const char __far* format, va_list val);
void console_printf(uint16_t flags, const char __far* format, ...);
bool console_print_status(bool status);

#endif
