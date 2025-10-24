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

#ifndef VWF8_H_
#define VWF8_H_

#include <stddef.h>
#include <wonderful.h>
#include <ws.h>

// 8-high variable width font system
int vwf8_get_char_width(uint8_t chr);
int vwf8_get_string_width(const char __wf_rom* s);
int vwf8_draw_char(uint8_t __wf_iram* tile, uint8_t chr, int x);
int vwf8_draw_string(uint8_t __wf_iram* tile, const char __wf_rom* s, int x);

#endif
