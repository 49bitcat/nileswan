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

#ifndef _CONFIG_H_
#define _CONFIG_H_

// Feature flags

// Enable full EEPROM emulation.
// If disabled, READ and WDS/WEN commands are ignored (emulated on FPGA side only).
// #define CONFIG_FULL_EEPROM_EMULATION

// #define CONFIG_ENABLE_ADC
// #define CONFIG_ENABLE_CDC_DEBUG_PORT
#define CONFIG_ENABLE_CLOCK_LSE

// Firmware configuration

#define MCU_PROTOCOL_VERSION_MAJOR 1
#define MCU_PROTOCOL_VERSION_MINOR 2

#define MCU_SPI_MAX_PACKET_SIZE 512
#define MCU_SPI_RX_BUFFER_SIZE (MCU_SPI_MAX_PACKET_SIZE)
#define MCU_SPI_TX_BUFFER_SIZE ((MCU_SPI_MAX_PACKET_SIZE) + 2)

// ~2.01V
#define MCU_POWER_BATTERY_INSERTED_THRESHOLD   2500
// ~2.33V
#define MCU_POWER_BATTERY_SUFFICIENT_THRESHOLD 2900

// MCU-specific settings

#define MCU_UID_LENGTH 12

// Debug configuration

// #define CONFIG_MCU_FORCE_MAX_CLOCK
// #define CONFIG_DEBUG_SPI_NATIVE_CMD
// #define CONFIG_DEBUG_SPI_EEPROM_CMD
// #define CONFIG_DEBUG_SPI_RTC_CMD
// #define CONFIG_DEBUG_SPI_RTC_DISABLE_PROCESSING
// #define CONFIG_DEBUG_SPI_NATIVE_CDC_READ
// #define CONFIG_DEBUG_SPI_NATIVE_CDC_WRITE
// #define CONFIG_DEBUG_ACCEL

// GPIO A
#define MCU_PORT_SPI GPIOA
#define MCU_PIN_FPGA_BUSY LL_GPIO_PIN_1
#define MCU_PIN_TF_DETECT LL_GPIO_PIN_2
#define MCU_PIN_FPGA_IRQ LL_GPIO_PIN_3
#define MCU_PIN_SPI_NSS LL_GPIO_PIN_4
#define MCU_PIN_SPI_SCK LL_GPIO_PIN_5
#define MCU_PIN_SPI_POCI LL_GPIO_PIN_6
#define MCU_PIN_SPI_PICO LL_GPIO_PIN_7
#define MCU_PERIPH_SPI SPI1

#define MCU_PORT_I2C GPIOA
#define MCU_PIN_I2C_SCL LL_GPIO_PIN_9
#define MCU_PIN_I2C_SDA LL_GPIO_PIN_10
#define MCU_PIN_I2C_INT LL_GPIO_PIN_15
#define MCU_PERIPH_I2C I2C1

// GPIO B
#define MCU_PIN_BAT LL_GPIO_PIN_0
#define MCU_PIN_BAT_CHANNEL_ADC LL_ADC_CHANNEL_17
#define MCU_PIN_USB_POWER LL_GPIO_PIN_5
#define MCU_PIN_RUNS_ON_BAT LL_GPIO_PIN_7

#define MCU_DMA_CHANNEL_SPI_RX       LL_DMA_CHANNEL_2
#define MCU_DMA_CHANNEL_SPI_TX_DATA  LL_DMA_CHANNEL_3
#define MCU_DMA_CHANNEL_SPI_TX_EMPTY LL_DMA_CHANNEL_1

#define MCU_DMA_CHANNEL_I2C LL_DMA_CHANNEL_4

#define MCU_ACCEL_TIM TIM6

#define MCU_SPI_FREQ_384KHZ 0
#define MCU_SPI_FREQ_6MHZ   1
#define MCU_SPI_FREQ_24MHZ  2

#define MCU_IRQ_PRIORITY_SPI 100
#define MCU_IRQ_PRIORITY_SPI_DMA 90
#define MCU_IRQ_PRIORITY_RTC 80
#define MCU_IRQ_PRIORITY_I2C 70
#define MCU_IRQ_PRIORITY_I2C_DMA 20
#define MCU_IRQ_PRIORITY_USB 10
#define MCU_IRQ_PRIORITY_I2C_TIMER 5
#define MCU_IRQ_PRIORITY_DEFAULT 0

#define SAVE_ID_NONE 0xFFFFFFFF

#endif /* _CONFIG_H_ */
