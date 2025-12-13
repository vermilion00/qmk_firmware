/* Copyright 2020 Nick Brassel (tzarc)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include_next "mcuconf.h"

#undef STM32_SERIAL_USE_USART1
#define STM32_SERIAL_USE_USART1 TRUE

// #undef STM32_PWM_USE_TIM2
// #define STM32_PWM_USE_TIM2 TRUE

#undef STM32_PWM_USE_TIM3
#define STM32_PWM_USE_TIM3 TRUE

#undef STM32_ST_USE_TIMER
#define STM32_ST_USE_TIMER 2

#undef STM32_SPI_USE_SPI1
#define STM32_SPI_USE_SPI1 TRUE

#undef STM32_ADC_USE_ADC1
#define STM32_ADC_USE_ADC1 TRUE

#undef STM32_ADC_ADC1_DMA_STREAM
// #define STM32_ADC_ADC1_DMA_STREAM           STM32_DMA_STREAM_ID(2, 0) // DMA 2 Stream 0
#define STM32_ADC_ADC1_DMA_STREAM           STM32_DMA_STREAM_ID(2, 4)
#define STM32_ADC_ADC1_DMA_PRIORITY         2
#define STM32_ADC_ADC1_DMA_IRQ_PRIORITY     6

// #undef STM32_PLLM_VALUE
// #define STM32_PLLM_VALUE                    8
#undef STM32_PLLN_VALUE
#define STM32_PLLN_VALUE                    360
// #undef STM32PLLP_VALUE
// #define STM32_PLLP_VALUE                    2
#undef STM32_PLLQ_VALUE
#define STM32_PLLQ_VALUE                    8
// #undef STM32_PLLI2SN_VALUE
// #define STM32_PLLI2SN_VALUE                 192
// #undef STM32_PLLI2SM_VALUE
// #define STM32_PLLI2SM_VALUE                 8
// #undef STM32_PLLI2SR_VALUE
// #define STM32_PLLI2SR_VALUE                 4
// #undef STM32_PLLI2SP_VALUE
// #define STM32_PLLI2SP_VALUE                 4
// #undef STM32_PLLI2SQ_VALUE
// #define STM32_PLLI2SQ_VALUE                 4
// #undef STM32_PLLSAIN_VALUE
// #define STM32_PLLSAIN_VALUE                 192
// #undef STM32_PLLSAIM_VALUE
// #define STM32_PLLSAIM_VALUE                 8
// #undef STM32_PLLSAIP_VALUE
// #define STM32_PLLSAIP_VALUE                 4
#undef STM32_PLLSAIQ_VALUE
#define STM32_PLLSAIQ_VALUE                 2

//PLL is the first option, PLLALT the second one for PLL48CLK Mux
// #undef STM32_CK48MSEL
// #define STM32_CK48MSEL                      STM32_CK48MSEL_PLL
// #define STM32_CK48MSEL                      STM32_CK48MSEL_PLLALT

// #define STM32_USB_USE_OTG1                  TRUE
// #define STM32_USB_USE_OTG2                  FALSE
// #define STM32_USB_OTG1_IRQ_PRIORITY         14
// #define STM32_USB_OTG2_IRQ_PRIORITY         14
// #define STM32_USB_OTG1_RX_FIFO_SIZE         512
// #define STM32_USB_OTG2_RX_FIFO_SIZE         1024
// #define STM32_USB_HOST_WAKEUP_DURATION      2
