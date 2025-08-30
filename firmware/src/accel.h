/**
 * Copyright (c) 2024 Kemal Afzal
 *
 * Nileswan MCU is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan MCU is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan MCU. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _ACCEL_H_
#define _ACCEL_H_

#include <stdint.h>
#include <stdbool.h>

void accel_init(void);
void accel_deinit(void);
bool accel_is_detected(void);
bool accel_is_enabled(void);
void accel_adjust_i2c_timing(uint32_t pclk);

bool accel_enable_poll(bool enable);
void accel_copy_state(void* out);

#endif