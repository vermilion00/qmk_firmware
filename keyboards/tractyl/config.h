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

#define USB_POLLING_INTERVAL_MS 1
#define WAIT_FOR_USB

#define ENCODER_DEFAULT_POS 0x3

#define SPI_DRIVER                           SPID1
#define SPI_SCK_PIN                          A5
#define SPI_SCK_PAL_MODE                     5
#define SPI_MOSI_PIN                         A7
#define SPI_MOSI_PAL_MODE                    5
#define SPI_MISO_PIN                         A6
#define SPI_MISO_PAL_MODE                    5

/* Trackball */
#define PMW33XX_CS_PIN                       A4
#define PMW33XX_SPI_MODE                     3
#define PMW33XX_SPI_DIVISOR                  64
#define PMW33XX_FIRMWARE_UPLOAD_FAST
#define PMW33XX_LIFTOFF_DISTANCE 0x01
//#define PMW3360_CLOCK_SPEED 2000000
//#define POINTING_DEVICE_INVERT_X
#define POINTING_DEVICE_INVERT_Y
//#define ROTATIONAL_TRANSFORM_ANGLE  90

/* Split configuration */
#define SPLIT_HAND_PIN C14 //high = left, low = right
#define SPLIT_USB_DETECT
#define SPLIT_POINTING_ENABLE
#define POINTING_DEVICE_RIGHT
#define SPLIT_USB_TIMEOUT 2000
#define SPLIT_USB_TIMEOUT_POLL 10
#define POINTING_DEVICE_TASK_THROTTLE_MS 1
#define SPLIT_TRANSACTION_IDS_KB RPC_ID_KB_CONFIG_SYNC

/* serial.c configuration for split keyboard */
#define SERIAL_USART_FULL_DUPLEX  // Enable full duplex operation mode.
#define SERIAL_USART_TX_PIN      B6 //Left Tx goes to Right Rx
#define SERIAL_USART_RX_PIN      B7
#define SERIAL_USART_DRIVER      SD1 
#define SERIAL_USART_TX_PAL_MODE 7    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 7
#define SERIAL_USART_RX_PAL_MODE 7    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 7
#define SERIAL_USART_TIMEOUT     100  // USART driver timeout. default 100
#define SERIAL_USART_SPEED       921600
#define CRC8_USE_TABLE
#define CRC8_OPTIMIZE_SPEED

/* RGBLight configuration */
//Previously RGD_DI_PIN
//#define WS2812_DI_PIN A1
//#define RGBLIGHT_LIMIT_VAL 100
/*#define RGBLED_NUM          18
#define RGBLIGHT_SPLIT
#define RGBLED_SPLIT \
    { 9, 9 }*/
//#define DEBUG_LED_PIN      C13
#define RGBLIGHT_DISABLE_KEYCODES
// WS2812 RGB LED strip input and number of LEDs
#define WS2812_PWM_DRIVER   PWMD2  // default: PWMD2
#define WS2812_PWM_CHANNEL  2      // default: 2
#define WS2812_PWM_PAL_MODE 1      // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 2
//#define WS2812_EXTERNAL_PULLUP
//#define WS2812_PWM_COMPLEMENTARY_OUTPUT // Define for a complementary timer output (TIMx_CHyN); omit for a normal timer output (TIMx_CHy).
#define WS2812_DMA_STREAM   STM32_DMA1_STREAM7  // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_DMA_CHANNEL  3                   // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_PWM_TARGET_PERIOD 800000 

#define MATRIX_ROWS 12
#define MATRIX_COLS 6

#define BOOTMAGIC_LITE_ROW 0
#define BOOTMAGIC_LITE_COLUMN 0
#define BOOTMAGIC_LITE_ROW_RIGHT 6
#define BOOTMAGIC_LITE_COLUMN_RIGHT 5
