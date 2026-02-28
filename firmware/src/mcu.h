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

#ifndef _MCU_H_
#define _MCU_H_

#include <stdbool.h>

#ifdef TARGET_U0
#include <stm32u073xx.h>
#include <stm32u0xx_ll_rcc.h>
#include <stm32u0xx_ll_bus.h>
#include <stm32u0xx_ll_exti.h>
#include <stm32u0xx_ll_system.h>
#include <stm32u0xx_ll_cortex.h>
#include <stm32u0xx_ll_utils.h>
#include <stm32u0xx_ll_gpio.h>
#include <stm32u0xx_ll_spi.h>
#include <stm32u0xx_ll_pwr.h>
#include <stm32u0xx_ll_usb.h>
#include <stm32u0xx_ll_crs.h>
#include <stm32u0xx_ll_dma.h>
#include <stm32u0xx_ll_rtc.h>
#include <stm32u0xx_ll_lptim.h>

#define USB USB_DRD_FS
#define LL_RCC_USB_CLKSOURCE_PLL LL_RCC_USB_CLKSOURCE_PLLQ
#define RTC_IRQn RTC_TAMP_IRQn
#else
#include <stm32l052xx.h>
#include <stm32l0xx_ll_rcc.h>
#include <stm32l0xx_ll_bus.h>
#include <stm32l0xx_ll_exti.h>
#include <stm32l0xx_ll_system.h>
#include <stm32l0xx_ll_cortex.h>
#include <stm32l0xx_ll_utils.h>
#include <stm32l0xx_ll_gpio.h>
#include <stm32l0xx_ll_spi.h>
#include <stm32l0xx_ll_pwr.h>
#include <stm32l0xx_ll_usb.h>
#include <stm32l0xx_ll_crs.h>
#include <stm32l0xx_ll_dma.h>
#include <stm32l0xx_ll_rtc.h>
#endif

#include "config.h"

#define mcu_critical(...) \
    do { \
        __disable_irq(); \
        __VA_ARGS__; \
        __enable_irq(); \
    } while(0)

void mcu_init(void);
void mcu_update_clock_speed(void);
void mcu_shutdown(void);
void mcu_usb_power_task(void);

/**
 * @brief Power down features which are only used in the "native" MCU mode.
 */
void mcu_exit_native_mode(void);

/**
 * @brief Signal to the FPGA that the MCU is currently busy.
 */
static inline void mcu_fpga_start_busy(void) {
    LL_GPIO_SetPinMode(GPIOA, MCU_PIN_FPGA_BUSY, LL_GPIO_MODE_OUTPUT);
}
/**
 * @brief Signal to the FPGA that the MCU is no longer busy.
 */
static inline void mcu_fpga_finish_busy(void) {
    LL_GPIO_SetPinMode(GPIOA, MCU_PIN_FPGA_BUSY, LL_GPIO_MODE_ANALOG);
}

static inline void mcu_fpga_irq_set(void) {
    LL_GPIO_ResetOutputPin(GPIOA, MCU_PIN_FPGA_IRQ);
}

static inline void mcu_fpga_irq_clear(void) {
    LL_GPIO_SetOutputPin(GPIOA, MCU_PIN_FPGA_IRQ);
}

/**
 * @brief Reset backup domain while preserving certain register values.
 */
void mcu_reset_backup_domain(void);

/**
 * @brief Set whether or not the USB port should be enabled (on the MCU side).
 * The USB hardware will only be activated if the USB port is enabled and an USB
 * cable is physically connected to a host.
 */
void mcu_usb_set_enabled(bool enabled);
/**
 * @brief Returns true if the USB port is powered on and enabled.
 */
bool mcu_usb_is_powered(void);
/**
 * @brief Returns true if the USB port is powered on and enabled,
 * and the USB device is active (connected, not suspended).
 */
bool mcu_usb_is_active(void);

/**
 * @brief Returns true if the USB port is powered on (physically).
 */
static inline bool mcu_usb_is_power_connected(void) {
    return LL_GPIO_IsInputPinSet(GPIOB, MCU_PIN_USB_POWER);
}

/**
 * @brief Read the ADC value for the currently detected battery voltage.
 *
 * @return uint16_t Raw ADC readout (0 - 4095).
 */

#ifdef CONFIG_ENABLE_ADC
uint16_t mcu_power_query_battery_voltage(void);

static inline bool mcu_power_is_battery_inserted(void) {
    return mcu_power_query_battery_voltage() >= MCU_POWER_BATTERY_INSERTED_THRESHOLD;
}

static inline bool mcu_power_is_battery_sufficient(void) {
    return mcu_power_query_battery_voltage() >= MCU_POWER_BATTERY_SUFFICIENT_THRESHOLD;
}
#else
static inline bool mcu_power_is_battery_inserted(void) {
    return LL_GPIO_IsInputPinSet(GPIOB, MCU_PIN_BAT);
}

static inline bool mcu_power_is_battery_sufficient(void) {
    return LL_GPIO_IsInputPinSet(GPIOB, MCU_PIN_BAT);
}
#endif

/**
 * @brief Returns true if the MCU is currently running on battery power.
 *
 * TODO: Does the user need to make sure to also check if the battery is
 * actually inserted?
 */
static inline bool mcu_power_is_running_on_battery(void) {
    return LL_GPIO_IsInputPinSet(GPIOB, MCU_PIN_RUNS_ON_BAT);
}

#endif /* _MCU_H_ */
