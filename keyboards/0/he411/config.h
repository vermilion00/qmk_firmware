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

// SPI Flash config (Winbond 25Q64JV)
#define EXTERNAL_FLASH_SIZE (8 * 1024 * 1024)
#define EXTERNAL_FLASH_SPI_CLOCK_DIVISOR 8
#define EXTERNAL_FLASH_PAGE_SIZE 256
#define EXTERNAL_FLASH_SECTOR_SIZE (4 * 1024)
#define EXTERNAL_FLASH_BLOCK_SIZE (64 * 1024)
#define EXTERNAL_FLASH_ADDRESS_SIZE 3
#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN A4

// SPI driver config
#define SPI_DRIVER                           SPID1
#define SPI_SCK_PIN                          A5
#define SPI_SCK_PAL_MODE                     5
#define SPI_MOSI_PIN                         A7
#define SPI_MOSI_PAL_MODE                    5
#define SPI_MISO_PIN                         A6
#define SPI_MISO_PAL_MODE                    5

// Wear leveling config
#define WEAR_LEVELING_EXTERNAL_FLASH_BLOCK_COUNT 64
#define WEAR_LEVELING_EXTERNAL_FLASH_BLOCK_OFFSET 0
#define WEAR_LEVELING_LOGICAL_SIZE (8*1024)
#define WEAR_LEVELING_BACKING_SIZE (64*1024)
#define BACKING_STORE_WRITE_SIZE 8


#define FORCE_BOOTLOADER_PIN A0
#define FORCE_BOOTLOADER_CHANNEL 9

#define LED_PIN C13
#define LED_INVERTED

// #define DEBUG_MATRIX_SCAN_RATE

#define CRC8_USE_TABLE
#define CRC8_OPTIMIZE_SPEED
