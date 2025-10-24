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

#include <stdint.h>
#include "util.h"

const uint8_t hexchars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

uint8_t hex_to_int(uint8_t c) {
	if (c >= '0' && c <= '9') {
		return c-48;
	} else if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
		return (c & 0x07)+9;
	} else {
		return 0;
	}
}

uint8_t int_to_hex(uint8_t c) {
	return hexchars[c & 0x0F];
}
