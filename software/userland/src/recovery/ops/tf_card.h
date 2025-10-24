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

#ifndef OPS_TF_CARD_H_
#define OPS_TF_CARD_H_

#include <stddef.h>
#include <wonderful.h>
#include <ws.h>
#include <nilefs.h>

extern FATFS fs;

bool op_tf_card_init(bool force);
bool op_tf_card_test(void);
bool op_tf_card_format(void);
bool op_tf_card_benchmark_read(void);
bool op_tf_card_benchmark_write(void);

#endif
