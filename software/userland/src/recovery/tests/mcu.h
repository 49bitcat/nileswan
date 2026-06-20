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

#ifndef TEST_MCU_H_
#define TEST_MCU_H_

#include <stddef.h>
#include <wonderful.h>
#include <ws.h>

bool test_mcu_usb_cdc_echo(void);
bool test_mcu_accelerometer(void);
bool test_mcu_tf_insert_remove(void);
bool test_mcu_eeprom(void);
bool test_mcu_save_id(void);

#endif
