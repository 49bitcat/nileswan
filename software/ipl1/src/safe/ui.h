/**
 * Copyright (c) 2024, 2025, 2026 Adrian "asie" Siekierka
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

#include <ws.h>
#include "util.h"

extern bool full_redraw;
void clear_screen(void);
void wait_for_button(void);

#define SCREEN ((uint16_t*) (0x3800 + (13 * 32 * 2)))

#define DRAW_STRING(x, y, s, pal) mem_expand_8_16(SCREEN + ((y) * 32) + (x), (s), sizeof(s) - 1, 0x100 | pal);
#define DRAW_STRING_CENTERED(y, s, pal) DRAW_STRING(((29 - sizeof(s)) >> 1), y, s, pal)
#define DRAW_STRING_DYNAMIC(x, y, s, pal) mem_expand_8_16(SCREEN + ((y) * 32) + (x), (s), strlen(s), 0x100 | pal);
#define DRAW_STRING_CENTERED_DYNAMIC(y, s, pal) DRAW_STRING_DYNAMIC(((30 - strlen(s)) >> 1), y, s, pal)
