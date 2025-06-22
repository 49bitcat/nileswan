#!/usr/bin/python3
#
# Copyright (c) 2024, 2025 Adrian Siekierka
#
# Nileswan Updater is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# Nileswan Updater is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with Nileswan Updater. If not, see <https://www.gnu.org/licenses/>.

import struct, subprocess

def create_manifest(args, digest_hash):
    version = {
        "major": 0,
        "minor": 0,
        "patch": 0,
        "commit": "0000000000000000000000000000000000000000",
        "digest": "0000000000000000000000000000000000000000000000000000000000000000"
    }

    if args.version is not None:
        version_from_args = args.version.split(".")
        if len(version_from_args) == 3:
            version["major"] = int(version_from_args[0])
            version["minor"] = int(version_from_args[1])
            version["patch"] = int(version_from_args[2])
        else:
            raise Exception(f"Invalid version format: {version}")

    try:
        git_out = subprocess.run(["git", "rev-parse", "--short=40", "HEAD"], stdout=subprocess.PIPE)
        commit_out = git_out.stdout.decode("utf-8").strip()
        if len(commit_out) == 40:
            version["commit"] = commit_out
    except e:
        pass

    if digest_hash is not None:
        version["digest"] = digest_hash.hexdigest()

    return bytearray(struct.pack("<BBHHHHH", 70, 87, version["major"], version["minor"], version["patch"], 0, 0)) + bytearray.fromhex(version["commit"]) + bytearray.fromhex(version["digest"])
