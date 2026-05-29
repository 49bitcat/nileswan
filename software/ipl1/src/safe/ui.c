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
#include "ui.h"

bool full_redraw = true;

void clear_screen(void) {
	ws_screen_fill_tiles(SCREEN, 0x120, 0, 0, WS_DISPLAY_WIDTH_TILES, WS_DISPLAY_HEIGHT_TILES);
	full_redraw = true;
}

void wait_for_button(void) {
	DRAW_STRING_CENTERED(17, "press any button", 0);
	while(!ws_keypad_scan());
	while(ws_keypad_scan());
}
