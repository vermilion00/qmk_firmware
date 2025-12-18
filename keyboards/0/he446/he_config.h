#pragma once

/*TODO:
-Set the he_matrix[] size to be equal to the larger half, if it's the right side then use memcpy
 to copy right side stuff into left side matrix

-Currently, some hardcoded values assume a 10 bit ADC resolution
    -Either make the values dynamic or force the resolution to be 10 bit=

-Change -s flag to force recompilation of config.h instead of deleting build dir
    -Or at least only delete the keyboard folder
    -Can probably just touch config.h
    -Will that work when other files are also dependent on the flag?

-*Add option to define keys that get scanned every scan, every other key only gets scanned
 every x scans instead for higher update rate (if I can get 8khz to work)
    -8Khz is doable but the PHY situation is annoying
    -Add stuff so that other half also doesn't get scanned

-Add support for side assignment at init
    -*Get side from split_util.c
    -Change matrixes to pointers
    -Assign the pointers in init according to needs

-Sync profile state between halves
-If calibrating, sync calibration start/end and values every print
    -Print data for each half independently
    -Save finished bool per half, exit calibration if both halves are finished
-If no_eeprom, sync slave bounds to master for printing

-Currently the _RIGHT stuff isn't being checked in mux and power functions
    -Either disallow setting different ones (preferred), or add support for it (messy)

-ADC Value changes faster at the bottom than at the top, adjust with a lut
    -Allow defining own lut
    -Instead of offsetting the scan value with the lut, use it to offset the height values at init
    -Support for switch presets containing travel distance and lut
-*If no switch data is in eeprom, immediately go into calibration mode, save data to eeprom when no changes have been made for many cycles, break out
-Change system to interrupt based?
-Try a higher buffer depth and circular buffer?
-Add always inline stuff

-Special modes
    -Stored in he_matrix[key].mode[profile], 0 = Default, 1-3 = RT, 10... = Special keys
    -Joystick axes
    -Make a separate json option for SOCD and combine it with RT for eval?
    -Dynamic Keystroke (4 press distances with separate actions)

-EEPROM stuff:
    -Since we only use HE_ADC_RESOLUTION bits, we can use bitfields for the values
    -Size will be n * 20 bits by default, so 1400 bits for 70 keys
    -Check if anything needs to be saved at the end of the flash (bootloader flag?)

-Add json feature option for hall_effect, to apply all the relevant patches

-When using STM32 mcus, set mux pins/power pins via BSR-register instead, should be faster

-Add option to define normal buttons to immediately jump to calibration/bootloader/reset
    -Or even mixed matrices, for encoders etc
    -Make it a separate matrix scan triggered off of a define
    -Maybe just direct pins?

-Add option to define heights etc using the layout macro, to make setting specific keys easier
    -Easy for normal keyboards, hard for split keyboards
    -Need to find a way to split one macro into two arrays

-Allow associating a color with a profile?
    -Or just set them by layer

-Add key to print current calibration values to console if dynamic calibration is enabled
    -On split keyboards, paste left and right values separately

-Allow setting heights via layout macro
    -Also get config from heights file in keyboard folder
    -If no heights file is available, use profile info to generate one
    -If x and y are set correctly for the keyboard, add tabs and spaces for a correct visual representation

-Add MIDI mode with velocity controlled by the change in adc value
    -Allow triggering past a certain height instead of only at the bottom

-Add option to power all sensors through one mosfet, and turn it off when inactive

-Enable interleaved ADC mode for supported mcus

-Debug output works better with qhe than qhed
-info_defaults don't work?
-Pins are set to input by default -> QMK sets unused pins to output high (Why not low?)
-LED on B2 is active high
-Button on C13 is active high
*/

// //DEBUG
// #undef HE_TOP_VALUES
// #define HE_TOP_VALUES {[0 ... SWITCH_NUM - 1] = 630 }
// #undef HE_BOTTOM_VALUES
// #define HE_BOTTOM_VALUES {[0 ... SWITCH_NUM - 1] = 270 }
