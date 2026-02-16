/**
 * Copyright (c) 2026 Adrian "asie" Siekierka
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

#include "button.h"
#include <nile.h>
#include <nile/hardware.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include "config.h"
#include "console.h"
#include "input.h"
#include "strings.h"

// Debouncing

static bool _button_pressed_fetch1;
static bool _button_pressed_fetch2;
static bool _button_pressed;

static inline void button_press_fetch(void) {
    ws_delay_us(4);
    _button_pressed_fetch1 = _button_pressed_fetch2;
    _button_pressed_fetch2 = (inportb(IO_NILE_IRQ_STATUS) & NILE_IRQ_BUTTON_HELD) != 0;
}

static void button_press_init(void) {
    button_press_fetch();
    button_press_fetch();
    _button_pressed = _button_pressed_fetch1 && _button_pressed_fetch2;
}

static bool button_is_pressed(void) {
    button_press_fetch();
    if (_button_pressed_fetch1 == _button_pressed_fetch2) {
        _button_pressed = _button_pressed_fetch2;
    }
    return _button_pressed;
}

static bool test_button_wait_for_state(bool expected_state, const char __far *cmd) {
    if (button_is_pressed() == expected_state)
        return true;

    wait_for_vblank();
    input_update();
    
    console_print(0, cmd);
    while (button_is_pressed() != expected_state) {
        wait_for_vblank();
        input_update();
        
        if (input_pressed & WS_KEY_B) {
            console_print_status(false);
            console_print_newline(0);
            return false;
        }
    }

    console_print_status(true);
    console_print_newline(0);
    return true;
}

bool test_button(void) {
    console_print_header(s_button_test);

    button_press_init();

    console_print(0, s_press_b_to_abort);
    console_print_newline(0);

    if (!test_button_wait_for_state(false, s_button_release)) return false;
    if (!test_button_wait_for_state(true, s_button_press)) return false;
    if (!test_button_wait_for_state(false, s_button_release)) return false;

    return true;
}
