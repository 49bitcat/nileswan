/**
 * Copyright (c) 2024 Kemal Afzal
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
#include "accel.h"

#include "config.h"
#include "cdc.h"

#include <stdint.h>
#include <stm32u0xx_ll_dma.h>
#include <stm32u0xx_ll_utils.h>
#include <string.h>

#include <stm32u0xx_ll_i2c.h>
#include <stm32u0xx_ll_tim.h>

#define MXC400_I2C_ADDR 0x15<<1

#define MXC400_REG_CONTROL 0x0D
#define MXC400_POWERDOWN (1<<0)

#define MXC400_REG_OUT_START 0x03

// timer ticks in microseconds
#define POLLINTERVAL ((1000*1000) / 100) // 100 Hz poll frequency

static uint32_t timing = 0;

static bool polling = false;

typedef enum {
    state_Idle,
    state_SendAddr,
    state_ReadAccelState
} State;
static volatile State state = state_Idle;

static uint32_t timer_prescaler;

static volatile uint8_t visible_accel_state = 0;
static int16_t accel_data[2][3] = {{0, 0, -(1<<10)}, {0, 0, -(1<<10)}};

static bool chip_detected = false;

void DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX_OVR_IRQHandler(void) {
    _Static_assert(MCU_DMA_CHANNEL_I2C == LL_DMA_CHANNEL_4, "Assumed I2C uses DMA channel 4");

    uint32_t write_state = visible_accel_state ^ 1;
    for (int i = 0; i < 3; i++)
        accel_data[write_state][i] =
            (int16_t)__builtin_bswap16(accel_data[write_state][i]) >> 4;
    //cdc_debug("done %d %d %d\n", accel_data[write_state][0], accel_data[write_state][1], accel_data[write_state][2]);

    LL_DMA_ClearFlag_TC4(DMA1);
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_I2C);
    state = state_Idle;

    visible_accel_state = write_state;
}

static bool __i2c_err_detected() {
    return LL_I2C_IsActiveFlag_BERR(MCU_PERIPH_I2C)
        || LL_I2C_IsActiveFlag_ARLO(MCU_PERIPH_I2C)
        || LL_I2C_IsActiveFlag_OVR(MCU_PERIPH_I2C)
        || LL_I2C_IsActiveFlag_NACK(MCU_PERIPH_I2C);
}

static void __i2c_err_clear() {
    LL_I2C_ClearFlag_BERR(MCU_PERIPH_I2C);
    LL_I2C_ClearFlag_ARLO(MCU_PERIPH_I2C);
    LL_I2C_ClearFlag_OVR(MCU_PERIPH_I2C);
    LL_I2C_ClearFlag_NACK(MCU_PERIPH_I2C);
}

void I2C1_IRQHandler(void) {
    //cdc_debug("interrupt %x\n", MCU_PERIPH_I2C->ISR);
    State cur_state = state;
    if (__i2c_err_detected()) {
        cdc_debug("I2C error %x\n", MCU_PERIPH_I2C->ISR);
        __i2c_err_clear();
        state = state_Idle;
    } else if (cur_state == state_SendAddr) {
        if (LL_I2C_IsActiveFlag_TC(MCU_PERIPH_I2C)) {
            //cdc_debug("tc, start read\n");

            LL_DMA_ConfigAddresses(DMA1,
                MCU_DMA_CHANNEL_I2C,
                (uint32_t) &accel_data[visible_accel_state^1],
                LL_I2C_DMA_GetRegAddr(MCU_PERIPH_I2C, LL_I2C_DMA_REG_DATA_RECEIVE),
                LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
            LL_DMA_SetDataLength(DMA1, MCU_DMA_CHANNEL_I2C, sizeof(accel_data[0]));

            LL_DMA_EnableChannel(DMA1, MCU_DMA_CHANNEL_I2C);
            LL_I2C_HandleTransfer(MCU_PERIPH_I2C,
                MXC400_I2C_ADDR,
                LL_I2C_ADDRSLAVE_7BIT,
                6,
                LL_I2C_MODE_AUTOEND,
                LL_I2C_GENERATE_RESTART_7BIT_READ);

            state = state_ReadAccelState;
        } else if (LL_I2C_IsActiveFlag_TXE(MCU_PERIPH_I2C)) {
            LL_I2C_TransmitData8(MCU_PERIPH_I2C, MXC400_REG_OUT_START);
        } else {
            cdc_debug("????\n");
        }
    } else {
        cdc_debug("i2c: invalid state %x\n", cur_state);
    }
}

void TIM6_DAC_LPTIM1_IRQHandler(void) {
    LL_TIM_ClearFlag_UPDATE(MCU_ACCEL_TIM);

    //cdc_debug("timer %d %x %x\n", state, MCU_PERIPH_I2C->ISR, timing);
    if (state == state_Idle) {
        state = state_SendAddr;
        LL_I2C_HandleTransfer(MCU_PERIPH_I2C,
                MXC400_I2C_ADDR,
                LL_I2C_ADDRSLAVE_7BIT,
                1,
                LL_I2C_MODE_SOFTEND,
                LL_I2C_GENERATE_START_WRITE);
    }
}

static void __write_control_reg(uint8_t control) {
    chip_detected = false;

    LL_I2C_DisableIT_TX(MCU_PERIPH_I2C);
    LL_I2C_DisableIT_ERR(MCU_PERIPH_I2C);
    LL_I2C_DisableIT_NACK(MCU_PERIPH_I2C);

    LL_I2C_HandleTransfer(MCU_PERIPH_I2C,
        MXC400_I2C_ADDR,
        LL_I2C_ADDRSLAVE_7BIT,
        2,
        LL_I2C_MODE_AUTOEND,
        LL_I2C_GENERATE_START_WRITE);

    LL_I2C_TransmitData8(MCU_PERIPH_I2C, MXC400_REG_CONTROL);
    while (!LL_I2C_IsActiveFlag_TXE(MCU_PERIPH_I2C)) {
        if (__i2c_err_detected()) {
            __i2c_err_clear();
            return;
        }
    }
    LL_I2C_TransmitData8(MCU_PERIPH_I2C, control);
    while (LL_I2C_IsActiveFlag_BUSY(MCU_PERIPH_I2C)) {
        if (__i2c_err_detected())  {
            __i2c_err_clear();
            return;
        }
    }

    LL_I2C_EnableIT_TX(MCU_PERIPH_I2C);
    LL_I2C_EnableIT_ERR(MCU_PERIPH_I2C);
    LL_I2C_EnableIT_NACK(MCU_PERIPH_I2C);

    chip_detected = true;
}

void accel_init(void) {
    LL_I2C_Disable(MCU_PERIPH_I2C);

    LL_I2C_SetTiming(MCU_PERIPH_I2C, timing);
    LL_I2C_EnableAnalogFilter(MCU_PERIPH_I2C);

    LL_I2C_Enable(MCU_PERIPH_I2C);

    LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_I2C, LL_DMAMUX_REQ_I2C1_RX);

    LL_DMA_SetDataTransferDirection(DMA1, MCU_DMA_CHANNEL_I2C, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, MCU_DMA_CHANNEL_I2C, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetPeriphIncMode(DMA1, MCU_DMA_CHANNEL_I2C, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, MCU_DMA_CHANNEL_I2C, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, MCU_DMA_CHANNEL_I2C, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, MCU_DMA_CHANNEL_I2C, LL_DMA_MDATAALIGN_BYTE);

    LL_DMA_EnableIT_TC(DMA1, MCU_DMA_CHANNEL_I2C);

    NVIC_SetPriority(DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX_OVR_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 120, 0));
    NVIC_EnableIRQ(DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX_OVR_IRQn);

    NVIC_SetPriority(I2C1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 120, 0));
    NVIC_EnableIRQ(I2C1_IRQn);

    // let write control reg enable the remaining interrupts later
    LL_I2C_EnableIT_TC(MCU_PERIPH_I2C);

    LL_I2C_EnableDMAReq_RX(MCU_PERIPH_I2C);

    LL_TIM_SetPrescaler(MCU_ACCEL_TIM, timer_prescaler);
    LL_TIM_SetAutoReload(MCU_ACCEL_TIM, POLLINTERVAL);
    LL_TIM_EnableUpdateEvent(MCU_ACCEL_TIM);
    LL_TIM_EnableIT_UPDATE(MCU_ACCEL_TIM);

    NVIC_EnableIRQ(TIM6_DAC_LPTIM1_IRQn);
    NVIC_SetPriority(TIM6_DAC_LPTIM1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 120, 0));

    // park the accelerometer until it is used
    __write_control_reg(MXC400_POWERDOWN);
}


static void __poll_pause() {
    if (polling) {
        //cdc_debug("poll pause... %x %x\n", MCU_PERIPH_I2C->CR1, MCU_PERIPH_I2C->CR2);
        LL_TIM_DisableCounter(MCU_ACCEL_TIM);
        while (state != state_Idle || LL_I2C_IsActiveFlag_BUSY(MCU_PERIPH_I2C));
        //cdc_debug("done\n");
    }
}

static void __poll_resume() {
    if (polling && chip_detected) {
        //cdc_debug("resuming\n");
        LL_TIM_EnableCounter(MCU_ACCEL_TIM);
    }
}

void accel_adjust_i2c_timing(uint32_t freq) {
    // https://github.com/a-downing/stm32f3_i2c_calc/blob/master/stm32f3_i2c_calc.py

    // parameters
    // 100 kHz I2C frequency
    // using 100 ns rise and fall time
    // with analog filter enabled

    uint32_t new_timing, new_prescaler;
    switch (freq) {
    case 4*1000*1000: new_timing = 0x00100D14; new_prescaler = 3; break;
    case 6*1000*1000: new_timing = 0x0020171D; new_prescaler = 5; break;
    case 12*1000*1000: new_timing = 0x00402D41; new_prescaler = 11; break;
    case 16*1000*1000: new_timing = 0x00503D58; new_prescaler = 15; break;
    case 48*1000*1000: new_timing = 0x10805D88; new_prescaler = 47; break;
    default: new_timing = 0; new_prescaler = 0; break;
    }

    if (new_prescaler == timer_prescaler)
        return;

    timing = new_timing;
    timer_prescaler = new_prescaler;

    __poll_pause();

    if (timing != 0) {
        if (LL_I2C_IsEnabled(MCU_PERIPH_I2C)) {
            // already initialised

            LL_I2C_Disable(MCU_PERIPH_I2C);
            while (LL_I2C_IsEnabled(MCU_PERIPH_I2C));
            LL_I2C_SetTiming(MCU_PERIPH_I2C, timing);
            LL_I2C_Enable(MCU_PERIPH_I2C);

            LL_TIM_SetPrescaler(MCU_ACCEL_TIM, timer_prescaler);

        }

        __poll_resume();
    }
}

void accel_enable_poll(bool enable) {
    if (polling ^ enable) {
        __poll_pause();
        __write_control_reg(enable ? 0 : MXC400_POWERDOWN);

        polling = enable;
        __poll_resume();
    }
}

void accel_copy_state(void* out) {
    // make sure the interrupt does not
    // fire midway through the copy
    NVIC_DisableIRQ(DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX_OVR_IRQn);
    memcpy(out, &accel_data[visible_accel_state], sizeof(accel_data[0]));
    NVIC_EnableIRQ(DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX_OVR_IRQn);
    //cdc_debug("copy state %d\n", accel_data.axis[0]);
}
