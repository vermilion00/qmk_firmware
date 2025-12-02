 /* Copyright 2020 Imam Rafii
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */
#pragma once

#define DEBUG_MATRIX_SCAN_RATE

#define USB_POLLING_INTERVAL_MS 1

/* serial.c configuration for split keyboard */
#define SERIAL_USART_FULL_DUPLEX  // Enable full duplex operation mode.
#define SERIAL_USART_TX_PIN      A9
#define SERIAL_USART_RX_PIN      A10
#define SERIAL_USART_DRIVER      SD1
#define SERIAL_USART_TX_PAL_MODE 7    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 7
#define SERIAL_USART_RX_PAL_MODE 7    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 7
#define SERIAL_USART_TIMEOUT     100  // USART driver timeout. default 100
#define SERIAL_USART_SPEED       921600
//DMA streams:
//DMA2: USART1_RX = Stream 2 Channel 4 and Stream 5 Channel 4, USART1_TX = Stream 6 Channel 5 and Stream 7 Channel 4

#define CRC8_USE_TABLE
#define CRC8_OPTIMIZE_SPEED

// Bayleaf stuff
#define SPLIT_TRANSACTION_IDS_KB RPC_ID_KB_CONFIG_SYNC

// WS2812 RGB LED strip input and number of LEDs
#define WS2812_PWM_DRIVER   PWMD2  // default: PWMD2
#define WS2812_PWM_CHANNEL  2      // default: 2
#define WS2812_PWM_PAL_MODE 2      // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 2
// #define WS2812_EXTERNAL_PULLUP
//#define WS2812_PWM_COMPLEMENTARY_OUTPUT // Define for a complementary timer output (TIMx_CHyN); omit for a normal timer output (TIMx_CHy).
//TODO: Check if this applies
#define WS2812_PWM_DMA_STREAM   STM32_DMA1_STREAM1  // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_PWM_DMA_CHANNEL  3                   // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_PWM_TARGET_PERIOD 800000
#define RGBLIGHT_DISABLE_KEYCODES

