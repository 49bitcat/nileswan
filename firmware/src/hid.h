/**
 * Copyright (c) 2025 Adrian Siekierka
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

#ifndef _HID_H_
#define _HID_H_

#include "mcu.h"
#include "tusb_config.h"

#define HID_UPDATE_ENABLE_X_HAT     0x1000
#define HID_UPDATE_DISABLE_X_BUTTON 0x2000
#define HID_UPDATE_ENABLE_Y_HAT     0x4000
#define HID_UPDATE_DISABLE_Y_BUTTON 0x8000

#if CFG_TUD_HID
void hid_send_update(uint16_t mask);
#else
#define hid_send_update(...)
#endif

#endif /* _HID_H_ */
