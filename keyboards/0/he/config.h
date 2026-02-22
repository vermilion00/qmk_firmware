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

// Using -s right in the build command defines SIDE_RIGHT and runs make clean
// #ifndef SIDE_RIGHT
// #   define WAIT_FOR_USB
// #   define USB_POLLING_INTERVAL_MS 1
// #endif

// #define JOYSTICK_AXIS_COUNT 6
// #define JOYSTICK_BUTTON_COUNT 16

#define USB_POLLING_INTERVAL_MS 0.5

#define FORCE_BOOTLOADER_PIN A0
#define FORCE_BOOTLOADER_CHANNEL 9

#define DEBUG_MATRIX_SCAN_RATE

// Pointing Device Accel
#define POINTING_DEVICE_ACCEL_TAKEOFF 2.7      // lower/higher value = curve takes off more smoothly/abruptly
#define POINTING_DEVICE_ACCEL_GROWTH_RATE 0.8  // lower/higher value = curve reaches its upper limit slower/faster
#define POINTING_DEVICE_ACCEL_OFFSET 1.3       // lower/higher value = acceleration kicks in earlier/later
#define POINTING_DEVICE_ACCEL_LIMIT 0.12       // lower limit of accel curve (minimum acceleration factor)

/* serial.c configuration for split keyboard */
#define SERIAL_USART_FULL_DUPLEX  // Enable full duplex operation mode.
#define SERIAL_USART_TX_PIN      A9
#define SERIAL_USART_RX_PIN      A10
#define SERIAL_USART_DRIVER      SD1
#define SERIAL_USART_TX_PAL_MODE 7    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 7
#define SERIAL_USART_RX_PAL_MODE 7    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 7
#define SERIAL_USART_TIMEOUT     100  // USART driver timeout. default 100
#define SERIAL_USART_SPEED       921600
// //DMA streams:
// //DMA2: USART1_RX = Stream 2 Channel 4 and Stream 5 Channel 4, USART1_TX = Stream 6 Channel 5 and Stream 7 Channel 4

#define CRC8_USE_TABLE
#define CRC8_OPTIMIZE_SPEED

// WS2812 RGB LED strip input and number of LEDs
#define WS2812_PWM_DRIVER   PWMD3  // default: PWMD2
#define WS2812_PWM_CHANNEL  2      // default: 2
#define WS2812_PWM_PAL_MODE 2      // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 2
// #define WS2812_EXTERNAL_PULLUP
//#define WS2812_PWM_COMPLEMENTARY_OUTPUT // Define for a complementary timer output (TIMx_CHyN); omit for a normal timer output (TIMx_CHy).
//TODO: Check if this applies
#define WS2812_PWM_DMA_STREAM   STM32_DMA1_STREAM2  // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_PWM_DMA_CHANNEL  5                   // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_PWM_TARGET_PERIOD 800000
#define RGBLIGHT_DISABLE_KEYCODES
#define SPLIT_LAYER_STATE_ENABLE

// /* SPI config for eeprom and pmw3360 sensor */
#define SPI_DRIVER                           SPID1
#define SPI_SCK_PIN                          B3
#define SPI_SCK_PAL_MODE                     5
#define SPI_MOSI_PIN                         A7
#define SPI_MOSI_PAL_MODE                    5
#define SPI_MISO_PIN                         B4
#define SPI_MISO_PAL_MODE                    5

/* PMW3360 config  */
#define PMW33XX_CS_PIN                       A15
#define PMW33XX_SPI_DIVISOR                  32
#define PMW33XX_SPI_MODE                     3
#define PMW33XX_CPI                          12000
#define PMW33XX_LIFTOFF_DISTANCE 0x01
#define POINTING_DEVICE_INVERT_Y
#define PMW33XX_FIRMWARE_UPLOAD_FAST

#define SPLIT_POINTING_ENABLE
#define POINTING_DEVICE_TASK_THROTTLE_MS 1
#define POINTING_DEVICE_RIGHT

#define SPLIT_TRANSACTION_IDS_KB RPC_ID_KB_CONFIG_SYNC
// #define SPLIT_TRANSACTION_IDS_KB RPC_ID_KB_CONFIG_SYNC, AM_PROFILE_SYNC
// #define SPLIT_TRANSACTION_IDS_KB RPC_ID_KB_CONFIG_SYNC, AM_PROFILE_SYNC, AM_JOYSTICK_SYNC
// #define SPLIT_TRANSACTION_IDS_KB RPC_ID_KB_CONFIG_SYNC, AM_PROFILE_SYNC, AM_CALIBRATION_M2S_SYNC, AM_CALIBRATION_S2M_SYNC, AM_CALIBRATION_STATE_SYNC

#define NO_ACTION_TAPPING
#define NO_ACTION_ONESHOT

#define SPLIT_WATCHDOG_ENABLE
