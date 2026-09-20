#!/usr/bin/python3
#
# Copyright (c) 2024, 2025 Adrian "asie" Siekierka
#
# Nileswan Userland is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# Nileswan Userland is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with Nileswan Userland. If not, see <https://www.gnu.org/licenses/>.

import argparse
import struct

import manifest_tools

parser = argparse.ArgumentParser(prog='manifest_to_mcu_recovery', description='Create MCU recovery flash file from manifest')
parser.add_argument('manifest', help='File describing the update contents')
parser.add_argument('output_bin', help='Output BIN file (.bin)')
args = parser.parse_args()

start_segment = 0
max_mcu_version = -1

mcu_file_paths = {}
mcu_file_segments = {}

updater_base_data = None
with open(args.manifest, 'r') as rules:
    for line in rules:
        rule = line.strip()
        if rule.startswith("#"):
            continue
        rule = rule.split(" ")
        rule_name = rule[0]
        rule_map = {}
        for i in range(0, len(rule), 2):
            rule_map[rule[i]] = rule[i+1]

        if rule_name == 'MCU_FLASH':
            flash_position = int(rule_map['AT'])
            if flash_position != 0:
                raise Exception(f"{rule[1]} at {flash_position} not supported")
            board_revision = int(rule_map['BOARD_REVISION'])

            mcu_file_paths[board_revision] = rule_map['MCU_FLASH']
            max_mcu_version = max(max_mcu_version, board_revision)

if max_mcu_version < 0:
	raise Exception("No MCU versions found")

start_segment = ((2 + 4 * (max_mcu_version + 1)) + 15) >> 4

with open(args.output_bin, 'wb') as fo:
    fo.write(struct.pack("<H", max_mcu_version + 1))
    for revision, path in mcu_file_paths.items():
        segment = start_segment
        if path in mcu_file_segments:
            segment = mcu_file_segments[path]
        else:
            unpacked_data = None
            with open(path, 'rb') as file:
                unpacked_data = file.read()
            if len(unpacked_data) > 65535:
                raise Exception(f"File too large: {path} ({len(unpacked_data)} bytes)")
            data = manifest_tools.compress_zx0(path, str(segment))
            fo.seek(start_segment * 16)
            fo.write(data)
            mcu_file_segments[path] = segment
            start_segment += (len(data) + 15) >> 4
        fo.seek(2 + 4 * revision)
        fo.write(struct.pack("<HH", segment, len(unpacked_data)))
