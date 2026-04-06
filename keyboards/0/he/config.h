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

// #define NO_SLAVE_AXES

#define DEBUG_MATRIX_SCAN_RATE

// #define RIGHT_MULTIPLIER 2.0
#define FILTER_STRENGTH 3
// #define SLAVE_FILTER_STRENGTH 4
// #define FILTER_STRENGTH 3
// Don't update the top values in DC
#define NO_TOP_UPDATE
//TODO: Make these the same thing
#define CAL_THRESHOLD 400
#define INIT_THRESHOLD 400

// This doesn't make it more precise, and costs 300 sps. Next step up from ADC_SAMPLE_3
// #define ADC_SAMPLING_RATE ADC_SAMPLE_15

// #define ADJUST_TRAVEL

#define LED_PIN B2

//TODO: Add a voltage divider to check vbus using a gpio pin, this means SPLIT_USB_DETECT isn't required
//#define USB_VBUS_PIN __

// Pointing Device Accel
#define POINTING_DEVICE_ACCEL_TAKEOFF 2.7      // lower/higher value = curve takes off more smoothly/abruptly
#define POINTING_DEVICE_ACCEL_GROWTH_RATE 0.8  // lower/higher value = curve reaches its upper limit slower/faster
#define POINTING_DEVICE_ACCEL_OFFSET 1.3       // lower/higher value = acceleration kicks in earlier/later
#define POINTING_DEVICE_ACCEL_LIMIT 0.12       // lower limit of accel curve (minimum acceleration factor)

/* serial.c configuration */
#define SERIAL_USART_FULL_DUPLEX  // Enable full duplex operation mode.
#define SERIAL_USART_TX_PIN      A9
#define SERIAL_USART_RX_PIN      A10
#define SERIAL_USART_DRIVER      SD1
#define SERIAL_USART_TX_PAL_MODE 7    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 7
#define SERIAL_USART_RX_PAL_MODE 7    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 7
#define SERIAL_USART_TIMEOUT     100  // USART driver timeout. default 100
#define SERIAL_USART_SPEED       921600 // Double the "max" speed according to the QMK docs
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
#define WS2812_PWM_DMA_STREAM   STM32_DMA1_STREAM2  // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_PWM_DMA_CHANNEL  5                   // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_PWM_TARGET_PERIOD 800000
#define RGBLIGHT_DISABLE_KEYCODES
//TODO: I don't think this calls layer_state_set, so no point in having this in?
// #define SPLIT_LAYER_STATE_ENABLE

/* SPI config for pmw3360 sensor */
#define SPI_DRIVER                           SPID1
#define SPI_SCK_PIN                          B3
#define SPI_SCK_PAL_MODE                     5
#define SPI_MOSI_PIN                         A7
#define SPI_MOSI_PAL_MODE                    5
#define SPI_MISO_PIN                         B4
#define SPI_MISO_PAL_MODE                    5

/* PMW3360 config  */
#define PMW33XX_CS_PIN                       A15
#define PMW33XX_SPI_DIVISOR                  8
#define PMW33XX_SPI_MODE                     3
#define PMW33XX_CPI                          12000
#define PMW33XX_LIFTOFF_DISTANCE 0x01
#define POINTING_DEVICE_INVERT_Y
#define PMW33XX_FIRMWARE_UPLOAD_FAST

// SPI Flash config (Macronix MX25L4006E)
// These should be the default values anyway, so not needed
#define EXTERNAL_FLASH_SIZE (512 * 1024 * 1024)
#define EXTERNAL_FLASH_SPI_CLOCK_DIVISOR 8
#define EXTERNAL_FLASH_PAGE_SIZE 256
#define EXTERNAL_FLASH_SECTOR_SIZE (4 * 1024)
#define EXTERNAL_FLASH_BLOCK_SIZE (64 * 1024)
#define EXTERNAL_FLASH_ADDRESS_SIZE 3
#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN B0 // or B0

// Wear leveling config
#define WEAR_LEVELING_EXTERNAL_FLASH_BLOCK_COUNT 1
#define WEAR_LEVELING_EXTERNAL_FLASH_BLOCK_OFFSET 0
#define WEAR_LEVELING_LOGICAL_SIZE (8*1024)
#define WEAR_LEVELING_BACKING_SIZE (64*1024)
#define BACKING_STORE_WRITE_SIZE 8

#define SPLIT_POINTING_ENABLE
#define POINTING_DEVICE_TASK_THROTTLE_MS 1
#define POINTING_DEVICE_RIGHT

#define NO_ACTION_TAPPING
#define NO_ACTION_ONESHOT

#define SPLIT_WATCHDOG_ENABLE

// #define SPLIT_TRANSACTION_IDS_KB RPC_ID_KB_CONFIG_SYNC

/*
#define MATRIX(k0A, k0B, k0C, k0D, k0E, k0F, k6A, k6B, k6C, k6D, k6E, k6F, k1A, k1B, k1C, k1D, k1E, k1F, k7A, k7B, k7C, k7D, k7E, k7F, k2A, k2B, k2C, k2D, k2E, k2F, k8A, k8B, k8C, k8D, k8E, k8F, k3A, k3B, k3C, k3D, k3E, k3F, k9A, k9B, k9C, k9D, k9E, k9F, k4C, k4D, kAC, kAD, k5D, k4E, k4F, kBA, kAA, kAB, k5E, k5F, kBB) \
              {k0A, k0B, k0C, k0D, k0E, k0F, k1A, k1B, k1C, k1D, k1E, k1F, k2A, k2B, k2C, k2D, k2E, k2F, k3A, k3B, k3C, k3D, k3E, k3F, k4C, k4D, k5D, k4E, k4F, k5E, k5F, k6A, k6B, k6C, k6D, k6E, k6F, k7A, k7B, k7C, k7D, k7E, k7F, k8A, k8B, k8C, k8D, k8E, k8F, k9A, k9B, k9C, k9D, k9E, k9F, kAC, kAD, kBA, kAA, kAB, kBB}
*/
