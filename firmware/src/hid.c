/**
 * Copyright (c) 2025 Adrian "asie" Siekierka
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "hid.h"
#include "mcu.h"
#include "tusb.h"

static uint8_t hid_button_to_hat[] = {
    GAMEPAD_HAT_CENTERED,
    GAMEPAD_HAT_UP,
    GAMEPAD_HAT_RIGHT,
    GAMEPAD_HAT_UP_RIGHT,
    GAMEPAD_HAT_DOWN,
    GAMEPAD_HAT_CENTERED,
    GAMEPAD_HAT_DOWN_RIGHT,
    GAMEPAD_HAT_RIGHT,
    GAMEPAD_HAT_LEFT,
    GAMEPAD_HAT_UP_LEFT,
    GAMEPAD_HAT_CENTERED,
    GAMEPAD_HAT_UP,
    GAMEPAD_HAT_DOWN_LEFT,
    GAMEPAD_HAT_LEFT,
    GAMEPAD_HAT_DOWN,
    GAMEPAD_HAT_CENTERED
};

#if CFG_TUD_HID
void hid_send_update(uint16_t mask) {
  if (!tud_hid_ready())
    return;

  uint8_t data[3] = {0};
  if (mask & HID_UPDATE_ENABLE_X_HAT)
      data[0] |= hid_button_to_hat[mask & 0xF];
  if (mask & HID_UPDATE_ENABLE_Y_HAT)
      data[0] |= (hid_button_to_hat[(mask >> 4) & 0xF] << 4);
  if (mask & HID_UPDATE_DISABLE_X_BUTTON)
      mask &= ~0x78;
  if (mask & HID_UPDATE_DISABLE_Y_BUTTON)
      mask &= ~0x780;
  data[1] = mask;
  data[2] = (mask >> 8) & 0x7;
  tud_hid_report(1, data, sizeof(data));
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {

}
#endif
