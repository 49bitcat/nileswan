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

#ifndef INPUT_H_
#define INPUT_H_

#include <stdbool.h>
#include <stdint.h>
#include <ws.h>
#include "config.h"

extern uint16_t input_pressed, input_held;

#define KEY_UP KEY_X1
#define KEY_DOWN KEY_X3
#define KEY_LEFT KEY_X4
#define KEY_RIGHT KEY_X2

#define KEY_AUP KEY_Y1
#define KEY_ADOWN KEY_Y3
#define KEY_ALEFT KEY_Y4
#define KEY_ARIGHT KEY_Y2

#ifdef CONFIG_SOUND_ALERTS
typedef enum {
    ALERT_NONE,
    ALERT_FAIL,
    ALERT_PASS,
    ALERT_ALERT
} alert_mode_t;
void alert_mode_set(alert_mode_t mode);
#endif

__attribute__((assume_ss_data, interrupt))
void __far vblank_int_handler(void);
void wait_for_vblank(void);

void vblank_input_update(void);
void input_reset(void);
void input_update(void);
void input_wait_clear(void);
void input_wait_key(uint16_t key);
void input_wait_any_key(void);

#endif /* INPUT_H_ */
