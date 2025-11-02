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

#include "console.h"
#include "input.h"
#include "iram.h"
#include "menu.h"

int menu_run(const char __far* const __far* options) {
    console_clear();

    int option_count = 0;
    for (const char __far * const __far * option = options; *option != NULL; option++) {
        console_draw(1, option_count, 0, *option);
        option_count++;
    }

    int selected_option = 0;
    int prev_selected_option = -1;
    while (true) {
        if (selected_option != prev_selected_option) {
            if (prev_selected_option >= 0) {
                ws_screen_modify_tiles(screen_1, ~SCR_ENTRY_PALETTE_MASK, SCR_ENTRY_PALETTE(0), 0, prev_selected_option, 28, 1);
            }
            ws_screen_modify_tiles(screen_1, ~SCR_ENTRY_PALETTE_MASK, SCR_ENTRY_PALETTE(2), 0, selected_option, 28, 1);
            prev_selected_option = selected_option;
        }

        wait_for_vblank();
        input_update();
        if (input_pressed & KEY_UP) {
            if (selected_option > 0) {
                selected_option--;
            } else {
                selected_option = option_count - 1;
            }
        }
        if (input_pressed & KEY_DOWN) {
            if (selected_option < (option_count - 1)) {
                selected_option++;
            } else {
                selected_option = 0;
            }
        }
        if (input_pressed & KEY_A) {
            break;
        }
        if (input_pressed & KEY_B) {
            selected_option = -1;
            break;
        }
    }

    ws_screen_modify_tiles(screen_1, ~SCR_ENTRY_PALETTE_MASK, SCR_ENTRY_PALETTE(0), 0, prev_selected_option, 28, 1);
    return selected_option;
}
