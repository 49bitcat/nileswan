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

#include "tf_card.h"
#include <nilefs/diskio.h>
#include <stdlib.h>
#include <string.h>
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include "console.h"
#include "input.h"
#include "strings.h"

FATFS fs;

DSTATUS disk_initialize(BYTE pdrv);

bool op_tf_card_init(bool force, bool mount_filesystem) {
    char blank = 0;
    int result;
    uint32_t iv;

    // Already initialized?
    if (!force && fs.fs_type) {
        goto done;
    }

    if (force) {
        nilefs_eject();
        memset(&fs, 0, sizeof(FATFS));
        ws_delay_ms(500);
    }

    console_print(0, s_tf_card_init);
    result = disk_initialize(0);
    console_print_status(result == 0);
    console_print_newline(0);

    if (result != 0) {
        return false;
    }

    if (mount_filesystem) {
        console_print(0, s_tf_card_fs_init);
        result = f_mount(&fs, &blank, 1);
        console_print_status(result == FR_OK);
        if (result != FR_OK) {
            console_print_newline(0);
            console_printf(0, s_error_code, result);
        }
        console_print_newline(0);

        if (result != FR_OK) {
            return false;
        }
    }

	uint16_t prev_sram_bank = inportw(WS_CART_EXTBANK_RAM_PORT);
	outportw(WS_CART_EXTBANK_RAM_PORT, NILE_SEG_RAM_IPC);
    console_printf(0, s_tf_card_type, (int) MEM_NILE_IPC->tf_card_status);
	outportw(WS_CART_EXTBANK_RAM_PORT, prev_sram_bank);

    if (disk_ioctl(0, GET_SECTOR_COUNT, &iv) == RES_OK) {
        console_print(0, s_tf_card_size);
        console_printf(0, s_format_1_long, iv);
        console_print_newline(0);
    }

    if (disk_ioctl(0, GET_BLOCK_SIZE, &iv) == RES_OK) {
        console_print(0, s_tf_card_block_size);
        console_printf(0, s_format_1_long, iv);
        console_print_newline(0);
    }

    console_print_newline(0);

done:
    nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_TF);
    return true;
}

bool op_tf_card_test(void) {
    // Force re-initialization in case previous tests changed state
    bool result = op_tf_card_init(true, true);
    
    // Maybe return information about the card?

    return result;
}

static const char __wf_rom path_tftest_bin[] = "/tftest.bin";

#define TF_TEST_MAX_SIZE 16384
#define TF_TEST_BUFFER_IRAM ((uint8_t __far*) MK_FP(0x0000, 0x5000))
#define TF_TEST_BUFFER_SRAM ((uint8_t __far*) MK_FP(0x1000, 0x0000))

#define TEST_BUFFER_COMPARE 0
#define TEST_BUFFER_WRITE   1
#define TEST_BUFFER_ERASE   2

static bool tf_card_test_buffer(uint8_t __far *buffer, uint8_t mode, uint16_t max_size) {
    for (uint16_t i = 0; i < max_size; i += 256) {
        for (int j = 0; j < 256; j++) {
            uint8_t expected_value = j + (i >> 8);
            if (TEST_BUFFER_WRITE) {
                buffer[i + j] = expected_value;
            } else if (TEST_BUFFER_ERASE) {
                buffer[i + j] = expected_value ^ 0xFF;
            } else if (buffer[i + j] != expected_value) {
                return false;
            }
        }
    }
    return true;
}

static FRESULT tf_card_test_open(FIL *f) {
    char buf[64];

    FRESULT result;
    strcpy(buf, path_tftest_bin);

    result = f_open(f, (const char*) buf, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    if (result != FR_OK) return result;
    
    if (f_size(f) != TF_TEST_MAX_SIZE) {
        result = f_lseek(f, 0);
        if (result != FR_OK) return result;
        result = f_truncate(f);
        if (result != FR_OK) return result;

        tf_card_test_buffer(TF_TEST_BUFFER_IRAM, TEST_BUFFER_WRITE, TF_TEST_MAX_SIZE);
        result = f_write(f, TF_TEST_BUFFER_IRAM, TF_TEST_MAX_SIZE, NULL);
        if (result != FR_OK) return result;
    }

    return FR_OK;
}

static bool tf_card_handle_error(FRESULT result) {
    if (result == FR_OK) return true;

    console_print_status(false);
    console_print_newline(0);
    console_printf(0, s_error_code, result);
    return false;
}

bool op_tf_card_format(void) {
    char path[2];
    uint8_t work[2048];
    path[0] = '/';
    path[1] = 0;

    console_print_header(s_tf_card_format);

    if (!op_tf_card_init(true, false)) return false;

	console_print(CONSOLE_FLAG_CENTER, s_tf_card_format_warning);
	console_print(0, s_proceed);
	input_wait_any_key();
	console_print_newline(0);
	console_print_newline(0);
	if (!(input_pressed & KEY_A)) return true;

    console_print(0, s_tf_card_formatting);

    FRESULT result = f_mkfs(path, NULL, work, sizeof(work));
    if (!console_print_status(result == FR_OK)) return false;

    if (!op_tf_card_init(false, true)) return false;
    return true;
}

static bool tf_card_test_read(FIL *file, uint8_t __far* buffer, uint16_t len, bool quiet) {
    uint16_t br;

    tf_card_test_buffer(buffer, TEST_BUFFER_ERASE, len);
    if (!tf_card_handle_error(f_lseek(file, 0))) return false;
    if (!quiet) console_printf(0, s_benchmark_reading_bytes, len);

    outportw(IO_HBLANK_TIMER, 65535);
    outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE | HBLANK_TIMER_ONESHOT);
    if (!tf_card_handle_error(f_read(file, buffer, len, &br))) return false;

    uint16_t hblanks = 65535 - inportw(IO_HBLANK_COUNTER);
    outportw(IO_TIMER_CTRL, 0);
    if (hblanks < 1) hblanks = 1;

    if (!tf_card_test_buffer(buffer, TEST_BUFFER_COMPARE, len)) {
        if (!quiet) console_print_status(false);
        console_print_newline(0);
        console_printf(0, s_benchmark_data_read_mismatch);
        return false;
    }

    if (!quiet) console_print_status(true);
    uint16_t bytes_msec = ((uint32_t) len * 12) / hblanks;
    // bytes/msec are approximately equal to kbytes/sec
    if (!quiet) console_printf(0, s_benchmark_hblanks, hblanks, bytes_msec);
    if (!quiet) console_print_newline(0);

    return true;
}

bool op_tf_card_benchmark_read(uint8_t buffer_type) {
    FIL file;

    console_print_header(s_benchmark_card_read);

    if (!ws_system_color_active() && buffer_type == TF_BENCH_BUFFER_IRAM) {
        console_print(0, s_model_unsupported);
        return false;
    }

    if (!op_tf_card_init(false, true)) return false;
    console_print(0, s_benchmark_preparing_test_file);
    if (!tf_card_handle_error(tf_card_test_open(&file))) return false;
    console_print_status(true);
    console_print_newline(0);
    console_print_newline(0);

    ws_bank_with_ram(0, {
        ws_bank_with_flash(buffer_type == TF_BENCH_BUFFER_PSRAM ? 1 : 0, {
            uint8_t __far *buffer = buffer_type == TF_BENCH_BUFFER_IRAM ? TF_TEST_BUFFER_IRAM : TF_TEST_BUFFER_SRAM;

            for (uint16_t len = 512; len <= TF_TEST_MAX_SIZE; len <<= 1) {
                if (!tf_card_test_read(&file, buffer, len, false))
                    return false;
            }
        });
    });

    return true;
}

bool op_tf_card_benchmark_write(uint8_t buffer_type) {
    FIL file;
    uint16_t br;

    console_print_header(s_benchmark_card_write);

    if (!ws_system_color_active() && buffer_type == TF_BENCH_BUFFER_IRAM) {
        console_print(0, s_model_unsupported);
        return false;
    }

    if (!op_tf_card_init(false, true)) return false;
    console_print(0, s_benchmark_preparing_test_file);
    if (!tf_card_handle_error(tf_card_test_open(&file))) return false;
    console_print_status(true);
    console_print_newline(0);
    console_print_newline(0);

    ws_bank_with_ram(0, {
        ws_bank_with_flash(buffer_type == TF_BENCH_BUFFER_PSRAM ? 1 : 0, {
            uint8_t __far *buffer = buffer_type == TF_BENCH_BUFFER_IRAM ? TF_TEST_BUFFER_IRAM : TF_TEST_BUFFER_SRAM;

            for (uint16_t len = 512; len <= TF_TEST_MAX_SIZE; len <<= 1) {
                tf_card_test_buffer(buffer, TEST_BUFFER_WRITE, len);

                if (!tf_card_handle_error(f_lseek(&file, 0))) return false;
                console_printf(0, s_benchmark_writing_bytes, len);

                outportw(IO_HBLANK_TIMER, 65535);
                outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE | HBLANK_TIMER_ONESHOT);
                if (!tf_card_handle_error(f_write(&file, buffer, len, &br))) return false;

                uint16_t hblanks = 65535 - inportw(IO_HBLANK_COUNTER);
                outportw(IO_TIMER_CTRL, 0);
                if (hblanks < 1) hblanks = 1;

                if (!tf_card_handle_error(f_lseek(&file, 0))) return false;
                if (!tf_card_handle_error(f_read(&file, buffer, len, &br))) return false;
                if (!tf_card_test_buffer(buffer, TEST_BUFFER_COMPARE, len)) {
                    console_print_status(false);
                    console_print_newline(0);
                    console_printf(0, s_benchmark_data_read_mismatch);
                    return false;
                }

                console_print_status(true);
                uint16_t bytes_msec = ((uint32_t) len * 12) / hblanks;
                // bytes/msec are approximately equal to kbytes/sec
                console_printf(0, s_benchmark_hblanks, hblanks, bytes_msec);
                console_print_newline(0);
            }
        });
    });

    return true;
}

bool test_tf_card_stability(uint32_t runs) {
    FIL file;

    console_print_header(s_tf_card_stability_test);

    if (!ws_system_color_active()) {
        console_print(0, s_model_unsupported);
        return false;
    }

    if (!op_tf_card_init(false, true)) return false;
    console_print(0, s_benchmark_preparing_test_file);
    if (!tf_card_handle_error(tf_card_test_open(&file))) return false;
    console_print_status(true);
    console_print_newline(0);

    ws_bank_with_flash(1, {
        console_print(0, s_tf_card_stability_test);
        if (!runs) {
            console_print(0, s_hold_b_to_abort);
            input_update();
            while (!(input_pressed & KEY_B)) {
                ws_bank_with_ram(0, {
                    if (!tf_card_test_read(&file, TF_TEST_BUFFER_IRAM, 16384, true)) return false;
                    if (!tf_card_test_read(&file, TF_TEST_BUFFER_SRAM, 16384, true)) return false;
                });
                input_update();
                console_putc(0, '.');
            }
            console_putc(0, '.');
        } else {
            while (runs--) {
                ws_bank_with_ram(0, {
                    if (!tf_card_test_read(&file, TF_TEST_BUFFER_IRAM, 16384, true)) return false;
                    if (!tf_card_test_read(&file, TF_TEST_BUFFER_SRAM, 16384, true)) return false;
                });
                console_putc(0, '.');
            }
        }
    });

    console_print_status(true);
    console_print_newline(0);
    return true;
}
