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

#include "mcu.h"
#include "tusb.h"

#include "spi.h"
#include "cdc.h"
#include "config.h"
#include "eeprom.h"
#include "rtc.h"
#include "spi_cmd.h"

static uint8_t spi_mode = MCU_SPI_MODE_NATIVE;
static uint8_t spi_freq = MCU_SPI_FREQ_384KHZ;

__attribute__((section(".noinit")))
uint8_t spi_tx_buffer[MCU_SPI_TX_BUFFER_SIZE];
uint8_t spi_rx_buffer[MCU_SPI_RX_BUFFER_SIZE];
const uint32_t spi_rx_buffer_circular = 0xFFFFFFFF;

static uint16_t spi_irq = 0;
static uint16_t spi_irq_enable = MCU_SPI_IRQ_RTC_ALARM;

static inline void mcu_spi_update_irq(void) {
    if (spi_irq & spi_irq_enable) {
        mcu_fpga_irq_set();
    } else {
        mcu_fpga_irq_clear();
    }
}

void mcu_spi_irq_set(uint16_t irq) {
    spi_irq |= irq;
    mcu_spi_update_irq();
}

void mcu_spi_irq_clear(uint16_t irq) {
    spi_irq &= ~irq;
    mcu_spi_update_irq();
}

uint16_t mcu_spi_irq_get_status(void) {
    return spi_irq;
}

uint16_t mcu_spi_irq_get_enable(void) {
    return spi_irq_enable;
}

void mcu_spi_irq_ack_status(uint16_t value) {
    spi_irq = spi_irq & ((~value) | MCU_SPI_IRQ_NON_ACK_MASK);
}

void mcu_spi_irq_set_enable(uint16_t value) {
    spi_irq_enable = value;
}

static void mcu_spi_clear_rx_queue(void) {
    while (LL_SPI_GetRxFIFOLevel(MCU_PERIPH_SPI)) {
        LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
    }
}

static inline void mcu_spi_disable_dma_tx_req(void) {
    LL_SPI_DisableDMAReq_TX(MCU_PERIPH_SPI);
}

static inline void mcu_spi_enable_dma_tx_req(void) {
    LL_SPI_EnableDMAReq_TX(MCU_PERIPH_SPI);
}

static inline void mcu_spi_disable_dma_tx_empty(void) {
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY);
}

static inline void mcu_spi_disable_dma_tx_data(void) {
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA);
}

static inline void mcu_spi_enable_dma_tx_empty(void) {
    LL_DMA_EnableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY);
}

static inline void mcu_spi_enable_dma_tx_data(void) {
    LL_DMA_EnableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA);
}

static void mcu_spi_disable_dma_rx(void) {
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_RX);
    LL_SPI_DisableDMAReq_RX(MCU_PERIPH_SPI);
}

static void mcu_spi_configure_dma_tx_data(const void *address, uint32_t length) {
    LL_DMA_ConfigAddresses(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA,
        (uint32_t) address, LL_SPI_DMA_GetRegAddr(MCU_PERIPH_SPI),
        LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, length);
}

static void mcu_spi_enable_dma_rx(void *address, uint32_t length) {
    LL_DMA_ConfigAddresses(DMA1, MCU_DMA_CHANNEL_SPI_RX,
        LL_SPI_DMA_GetRegAddr(MCU_PERIPH_SPI), (uint32_t) address,
        LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, MCU_DMA_CHANNEL_SPI_RX, length);

    LL_DMA_EnableChannel(DMA1, MCU_DMA_CHANNEL_SPI_RX);
    LL_SPI_EnableDMAReq_RX(MCU_PERIPH_SPI);
}

static void mcu_spi_dma_finish(void) {
    if (spi_mode == MCU_SPI_MODE_NATIVE) {
        int len = spi_native_finish_command_rx(spi_rx_buffer, spi_tx_buffer + 2);
        if (len < -1) return;

#ifdef CONFIG_DEBUG_SPI_NATIVE_CMD
        cdc_debug(", returning %d bytes\r\n", len);
#endif
        if (len == -1) {
            *((uint16_t*) spi_tx_buffer) = 1;
            len = 0;
        } else {
            *((uint16_t*) spi_tx_buffer) = (len << 1);
        }
        spi_tx_buffer[len + 2] = 0xFF;

        mcu_spi_configure_dma_tx_data(spi_tx_buffer, len + 3);
        mcu_critical({
            mcu_spi_disable_dma_tx_empty();
            mcu_spi_enable_dma_tx_data();
        });
    } else if (spi_mode == MCU_SPI_MODE_RTC) {
        mcu_fpga_start_busy();
        int len = rtc_finish_command_rx(spi_rx_buffer, spi_tx_buffer);
        if (len) {
            mcu_spi_configure_dma_tx_data(spi_tx_buffer, len);
            mcu_spi_enable_dma_tx_data();
            mcu_spi_enable_dma_tx_req();
        } else {
            LL_SPI_EnableIT_RXNE(MCU_PERIPH_SPI);
        }
        mcu_fpga_finish_busy();
    }
}

static uint8_t spi_native_idx = 0;

void mcu_spi_task(void) {
    if (spi_native_idx == 2) {
        spi_native_idx = 0;
        mcu_spi_dma_finish();
    }
}

void DMA1_Channel2_3_IRQHandler(void) {
    if (LL_DMA_IsActiveFlag_TC3(DMA1)) {
        if (spi_mode == MCU_SPI_MODE_NATIVE) {
            mcu_spi_disable_dma_tx_data();
            mcu_spi_enable_dma_tx_empty();
        } else {
            mcu_spi_disable_dma_tx_req();
            mcu_spi_disable_dma_tx_data();
        }

        LL_SPI_EnableIT_RXNE(MCU_PERIPH_SPI);

        LL_DMA_ClearFlag_TC3(DMA1);
    }

    if (LL_DMA_IsActiveFlag_TC2(DMA1)) {
        mcu_spi_disable_dma_rx();
        LL_DMA_ClearFlag_TC2(DMA1);

        if (spi_mode == MCU_SPI_MODE_NATIVE) {
            spi_native_idx = 2;
        } else {
            mcu_spi_dma_finish();
        }
    }
}

static volatile uint8_t native_byte_queue = 0xFF;

__attribute__((aligned(32)))
void SPI1_IRQHandler(void) {
    if (LL_SPI_IsActiveFlag_RXNE(SPI1)) {
        if (spi_mode == MCU_SPI_MODE_NATIVE) {
            uint8_t last_byte = native_byte_queue;
            uint8_t byte = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
            native_byte_queue = byte;
            if (last_byte == 0xFF) {
                return;
            }

            LL_SPI_DisableIT_RXNE(MCU_PERIPH_SPI);

            uint16_t cmd = last_byte | (byte << 8);
            int rx_length = spi_native_start_command_rx(cmd);
            if (rx_length) {
                mcu_spi_enable_dma_rx(spi_rx_buffer, rx_length);
            } else {
                spi_native_idx = 2;
            }

            // Clear command byte queue
            native_byte_queue = 0xFF;
        } else if (spi_mode == MCU_SPI_MODE_EEPROM) {
            uint16_t data = LL_SPI_ReceiveData16(MCU_PERIPH_SPI);
#ifdef CONFIG_DEBUG_SPI_EEPROM_CMD
            cdc_debug_write_hex16(data);
#endif
#ifdef CONFIG_FULL_EEPROM_EMULATION
            LL_SPI_TransmitData16(MCU_PERIPH_SPI, eeprom_exch_word(data));
#else
            eeprom_exch_word(data);
#endif
        } else if (spi_mode == MCU_SPI_MODE_RTC) {
            LL_SPI_DisableIT_RXNE(MCU_PERIPH_SPI);
            mcu_fpga_start_busy();
            uint8_t cmd = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
#ifdef CONFIG_DEBUG_SPI_RTC_DISABLE_PROCESSING
#ifdef CONFIG_DEBUG_SPI_RTC_CMD
            cdc_debug_write_hex16(cmd);
#endif
            mcu_fpga_finish_busy();
#else
            int rx_length = rtc_start_command_rx(cmd);
            if (rx_length) {
                mcu_spi_enable_dma_rx(spi_rx_buffer, rx_length);
                mcu_fpga_finish_busy();
            } else {
                mcu_spi_dma_finish();
            }
#endif
        } else if (spi_mode == MCU_SPI_MODE_CDC_OUTPUT) {
            uint8_t ch = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
            if (tud_cdc_connected()) {
                if (ch == '\n') tud_cdc_write_char('\r');
                tud_cdc_write_char(ch);
                tud_cdc_write_flush();
            }
        }
    }
}

uint32_t mcu_spi_get_freq(void) {
    return spi_freq;
}

void mcu_spi_set_freq(uint32_t freq) {
#ifdef CONFIG_MCU_FORCE_MAX_CLOCK
    uint32_t pin_speed = LL_GPIO_SPEED_FREQ_HIGH;
#else
    uint32_t pin_speed = LL_GPIO_SPEED_FREQ_LOW;
    if (freq >= MCU_SPI_FREQ_24MHZ) pin_speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    else if (freq >= MCU_SPI_FREQ_6MHZ) pin_speed = LL_GPIO_SPEED_FREQ_HIGH;
#endif

    spi_freq = freq;
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_SCK, pin_speed);
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_POCI, pin_speed);
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_PICO, pin_speed);

    mcu_update_clock_speed();
}

mcu_spi_mode_t mcu_spi_get_mode(void) {
    return spi_mode;
}

void mcu_spi_enable(void) {
    LL_SPI_Enable(MCU_PERIPH_SPI);
}

void mcu_spi_disable(void) {
    LL_mDelay(1);
    LL_SPI_Disable(MCU_PERIPH_SPI);
    mcu_spi_clear_rx_queue();
}

void mcu_spi_init(mcu_spi_mode_t mode) {
    if (LL_SPI_IsEnabled(MCU_PERIPH_SPI)) {
        mcu_spi_disable_dma_tx_req();
        mcu_spi_disable_dma_tx_data();
        mcu_spi_disable_dma_tx_empty();
        mcu_spi_disable_dma_rx();

        mcu_spi_disable();

        // Force SPI reset to clear TX queue
        LL_APB1_GRP2_ForceReset(LL_APB1_GRP2_PERIPH_SPI1);
        LL_APB1_GRP2_ReleaseReset(LL_APB1_GRP2_PERIPH_SPI1);
    }

    // Initialize SPI
    MCU_PERIPH_SPI->CR1 = LL_SPI_MODE_SLAVE | LL_SPI_MSB_FIRST | LL_SPI_FULL_DUPLEX
        | LL_SPI_POLARITY_LOW | LL_SPI_PHASE_1EDGE | LL_SPI_BAUDRATEPRESCALER_DIV2;

    spi_mode = mode;
    bool dma_enabled = spi_mode != MCU_SPI_MODE_EEPROM;

    mcu_update_dma_clock();

    if (dma_enabled) {
        // Initialize SPI DMA
        LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, LL_DMAMUX_REQ_SPI1_TX);
        LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, LL_DMAMUX_REQ_SPI1_TX);
        LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMAMUX_REQ_SPI1_RX);

        LL_DMA_SetDataTransferDirection(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
        LL_DMA_SetChannelPriorityLevel(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, LL_DMA_PRIORITY_LOW);
        LL_DMA_SetPeriphIncMode(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, LL_DMA_MDATAALIGN_BYTE);

        LL_DMA_SetDataTransferDirection(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
        LL_DMA_SetChannelPriorityLevel(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, LL_DMA_PRIORITY_LOW);
        LL_DMA_SetPeriphIncMode(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, LL_DMA_MDATAALIGN_BYTE);

        LL_DMA_SetDataTransferDirection(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
        LL_DMA_SetChannelPriorityLevel(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PRIORITY_LOW);
        LL_DMA_SetPeriphIncMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MDATAALIGN_BYTE);

        LL_DMA_SetMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MODE_NORMAL);
        LL_DMA_SetMode(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, LL_DMA_MODE_CIRCULAR);
        LL_DMA_SetMode(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA, LL_DMA_MODE_NORMAL);

        LL_DMA_ConfigAddresses(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY,
            (uint32_t) &spi_rx_buffer_circular, LL_SPI_DMA_GetRegAddr(MCU_PERIPH_SPI),
            LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
        LL_DMA_SetDataLength(DMA1, MCU_DMA_CHANNEL_SPI_TX_EMPTY, sizeof(spi_rx_buffer_circular));
    }

    if (spi_mode == MCU_SPI_MODE_EEPROM) {
        MCU_PERIPH_SPI->CR2 = LL_SPI_DATAWIDTH_16BIT | LL_SPI_RX_FIFO_TH_HALF | SPI_CR2_RXNEIE;
#ifdef CONFIG_FULL_EEPROM_EMULATION
        LL_SPI_TransmitData16(MCU_PERIPH_SPI, 0xFFFF);
#else
        LL_SPI_SetTransferDirection(MCU_PERIPH_SPI, LL_SPI_SIMPLEX_RX);
#endif
    } else {
        MCU_PERIPH_SPI->CR2 = LL_SPI_DATAWIDTH_8BIT | LL_SPI_RX_FIFO_TH_QUARTER | SPI_CR2_RXNEIE;
    }

    // LL_DMA_ClearFlag_TC2(DMA1);
    // LL_DMA_ClearFlag_TC3(DMA1);
    DMA1->IFCR = DMA_IFCR_CGIF2 | DMA_IFCR_CGIF3;

    if (dma_enabled) {
        LL_DMA_EnableIT_TC(DMA1, MCU_DMA_CHANNEL_SPI_RX);
        LL_DMA_EnableIT_TC(DMA1, MCU_DMA_CHANNEL_SPI_TX_DATA);
    }

    NVIC_SetPriority(SPI1_IRQn, MCU_IRQ_PRIORITY_SPI);
    NVIC_EnableIRQ(SPI1_IRQn);
    if (dma_enabled) {
        NVIC_SetPriority(DMA1_Channel2_3_IRQn, MCU_IRQ_PRIORITY_SPI_DMA);
        NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    } else {
        NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
    }

    mcu_update_clock_speed();

    // Enable SPI
    mcu_spi_enable();

    if (spi_mode == MCU_SPI_MODE_NATIVE) {
        mcu_spi_enable_dma_tx_empty();
        mcu_spi_enable_dma_tx_req();
    }
}
