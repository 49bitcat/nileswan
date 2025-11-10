/**
 * Copyright (c) 2024 Adrian "asie" Siekierka
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
#include <string.h>

#include "class/cdc/cdc_device.h"
#include "config.h"
#include "hid.h"
#include "mcu.h"
#include "cdc.h"
#include "nvram.h"
#include "spi_cmd.h"
#include "eeprom.h"
#include "rtc.h"
#include "spi.h"
#include "tusb.h"
#include "accel.h"

uint16_t spi_cmd;
#define SPI_NATIVE_CMD(n) ((n) & 0x7F)
#define SPI_NATIVE_ARG(n) ((n) >> 7)

static inline int arg_to_len(uint16_t arg) {
    return arg ? arg : 512;
}

int spi_native_start_command_rx(uint16_t cmd) {
    spi_cmd = cmd;
    uint16_t arg = SPI_NATIVE_ARG(spi_cmd);
    switch (SPI_NATIVE_CMD(spi_cmd)) {
    case MCU_SPI_CMD_ECHO:
    case MCU_SPI_CMD_USB_CDC_WRITE:
    case MCU_SPI_CMD_USB_HID_WRITE:
        return arg_to_len(arg);
    case MCU_SPI_CMD_EEPROM_WRITE:
        return (arg_to_len(arg) << 1) + 2;
    case MCU_SPI_CMD_EEPROM_READ:
        return 2;
    case MCU_SPI_CMD_SET_SAVE_ID:
        return 4;
    case MCU_SPI_CMD_RTC_COMMAND:
        return rtc_start_command_rx(arg);
    default:
        return 0;
    }
}

int spi_native_finish_command_rx(uint8_t *rx, uint8_t *tx) {
    uint16_t arg = SPI_NATIVE_ARG(spi_cmd);
#ifdef CONFIG_DEBUG_SPI_NATIVE_CMD
    cdc_debug("spi/native: received command %04X", spi_cmd);
#endif
    switch (SPI_NATIVE_CMD(spi_cmd)) {
    case MCU_SPI_CMD_ECHO:
        memcpy(tx, rx, arg_to_len(arg));
        return arg_to_len(arg);
    case MCU_SPI_CMD_MODE:
        if (arg == 0xFF) {
            mcu_shutdown();
        } else {
            mcu_spi_init(arg);
            if (arg != MCU_SPI_MODE_NATIVE) {
                accel_deinit();
            }
        }
        return -2;
    case MCU_SPI_CMD_FREQ:
        mcu_spi_set_freq(arg);
        tx[0] = 1;
        return 1;
    case MCU_SPI_CMD_ID:
        memcpy(tx, (void*) UID_BASE, MCU_UID_LENGTH);
        return MCU_UID_LENGTH;
    case MCU_SPI_CMD_VERSION:
    	((uint16_t*) tx)[0] = MCU_PROTOCOL_VERSION_MAJOR;
    	((uint16_t*) tx)[1] = MCU_PROTOCOL_VERSION_MINOR;
        tx[4] =
            MCU_SPI_CAP0_EEPROM
            | MCU_SPI_CAP0_USB
            | (accel_is_detected() ? MCU_SPI_CAP0_ACCEL : 0)
            | MCU_SPI_CAP0_RTC
            | (LL_RCC_LSE_IsReady() ? MCU_SPI_CAP0_RTC_LSE : 0)
            | (rtc_is_configured() ? MCU_SPI_CAP0_RTC_ENA : 0)
            | (mcu_usb_is_power_connected() ? MCU_SPI_CAP0_USB_DET : 0)
            | (mcu_usb_is_active() ? MCU_SPI_CAP0_USB_CON : 0);
        return 5;
    case MCU_SPI_CMD_EEPROM_MODE:
        eeprom_set_type(arg);
        tx[0] = 1;
        return 1;
    case MCU_SPI_CMD_EEPROM_ERASE:
        eeprom_erase();
        return 0;
    case MCU_SPI_CMD_EEPROM_WRITE:
        eeprom_write_data(rx + 2, *((uint16_t*) rx), arg_to_len(arg) << 1);
        return 0;
    case MCU_SPI_CMD_EEPROM_READ:
        eeprom_read_data(tx, *((uint16_t*) rx), arg_to_len(arg) << 1);
        return arg_to_len(arg) << 1;
    case MCU_SPI_CMD_RTC_COMMAND:
        return rtc_finish_command_rx(rx, tx);
    case MCU_SPI_CMD_EEPROM_GET_MODE:
        tx[0] = eeprom_get_type();
        return 1;
    case MCU_SPI_CMD_SET_SAVE_ID: {
        uint32_t save_id;
        memcpy(&save_id, rx, 4);
        if (arg & 0x1) nvram.save_id = save_id;
        else           nvram.save_id = SAVE_ID_NONE;
        if (arg & 0x2) TAMP->BKP8R = save_id;
        else           TAMP->BKP8R = SAVE_ID_NONE;
        tx[0] = 1;
    } return 1;
    case MCU_SPI_CMD_GET_SAVE_ID: {
        uint32_t save_id = SAVE_ID_NONE;
        if ((arg & 0x2) && TAMP->BKP8R   != SAVE_ID_NONE) save_id = TAMP->BKP8R;
        if ((arg & 0x1) && nvram.save_id != SAVE_ID_NONE) save_id = nvram.save_id;
        memcpy(tx, &save_id, 4);
    } return 4;
    case MCU_SPI_CMD_USB_CDC_READ: {
        if (!tud_cdc_connected()) {
            return 0;
        }
        uint32_t len = arg_to_len(arg);
        uint32_t result = tud_cdc_read(tx, len);
#ifdef CONFIG_DEBUG_SPI_NATIVE_CDC_READ
        if (len) {
            cdc_debug("spi/native/cdc: read %d/%d bytes", result, len);
            for (int i = 0; i < result; i++) {
                cdc_debug_write_hex8_space(tx[i]);
                if (!(i & 7)) tud_cdc_n_write_flush(1);
            }
            cdc_debug_write("\r\n", 2);
        }
#endif
        return result;
    }
    case MCU_SPI_CMD_USB_CDC_WRITE: {
        uint32_t result = 0;
        if (tud_cdc_connected()) {
            uint32_t len = arg_to_len(arg);
            result = tud_cdc_write(rx, len);
#ifdef CONFIG_DEBUG_SPI_NATIVE_CDC_WRITE
            cdc_debug("spi/native/cdc: wrote %d/%d bytes", result, len);
            if (result) {
                for (int i = 0; i < result; i++) {
                    cdc_debug_write_hex8_space(rx[i]);
                    if (!(i & 7)) tud_cdc_n_write_flush(1);
                }
                cdc_debug_write("\r\n", 2);
            }
#endif
        }
        *((uint16_t*) tx) = result;
        return 2;
    }
    case MCU_SPI_CMD_USB_CDC_FLUSH: {
        if (tud_cdc_connected()) {
            tud_cdc_write_flush();
            tud_cdc_write_clear();
            tud_cdc_read_flush();
        }
        return 0;
    }
    case MCU_SPI_CMD_USB_HID_WRITE: {
        hid_send_update(*((uint16_t*) rx));
        return 0;
    }
    case MCU_SPI_CMD_USB_CDC_AVAILABLE: {
        if (!tud_cdc_connected()) {
            *((uint16_t*) tx) = 0;
        } else {
            *((uint16_t*) tx) = tud_cdc_available();
        }
        return 2;
    }
    case MCU_SPI_CMD_ACCEL_POLL: {
        *tx = accel_enable_poll(arg != 0, arg);
        return 1;
    }
    case MCU_SPI_CMD_ACCEL_READ: {
        accel_copy_state(tx);
        return 6;
    }
    default:
        return -1;
    }
    return 0;
}

#if 0
void tud_cdc_rx_cb(uint8_t itf) {
    if (itf == 0 && tud_cdc_n_available(itf)) {
        cdc_debug("available: %d\n", tud_cdc_n_available(itf));
    }
}
#endif
