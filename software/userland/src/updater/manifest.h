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

#ifndef MANIFEST_H_
#define MANIFEST_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

#define UM_CMD_END 0x00
#define UM_CMD_FLASH 0x02
#define UM_CMD_PACKED_FLASH 0x03
#define UM_CMD_MCU_FLASH 0x04
#define UM_CMD_MCU_PACKED_FLASH 0x05
#define UM_CMD_START_MANIFEST 0x06
#define UM_CMD_FINISH_MANIFEST 0x07
#define UM_CMD_CHECK_BOARD_REVISION_RANGE 0x08
#define UM_ID 0x5746

typedef struct __attribute__((packed)) {
	uint16_t id;
	uint16_t major;
	uint16_t minor;
	uint16_t patch;
	uint8_t reserved[3];
	uint8_t partial_install; ///< 0x00 if install successful, non-0x00 if partial
	uint8_t commit_id[20];
	uint8_t digest[32];
} um_version_t;

typedef struct __attribute__((packed)) {
	um_version_t version;
} um_header_t;

typedef struct __attribute__((packed)) {
	uint8_t cmd;
	uint16_t load_segment;
	uint16_t unpacked_length;
	uint32_t flash_address;
	uint16_t expected_crc;
	uint16_t board_revision;
} um_flash_cmd_t;

typedef struct __attribute__((packed)) {
	uint8_t cmd;
	uint32_t flash_address;
} um_manifest_cmd_t;

typedef struct __attribute__((packed)) {
	uint8_t cmd;
	uint16_t min_rev;
	uint16_t max_rev;
} um_board_revision_range_cmd_t;

#endif /* MANIFEST_H_ */
