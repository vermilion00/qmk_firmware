#pragma once

/*TODO:
-Enable FPU on supported processors
    -Check if it causes problems?
    -Make sure floats are used, not doubles
    -Not really necessary, just makes startup faster

-Split init happens after matrix init, so set a flag in calibration init key func and check that later

-Currently, some hardcoded values assume a 10 bit ADC resolution
    -Either make the values dynamic or force the resolution to be 10 bit

-Change -s flag to force recompilation of config.h instead of deleting build dir
    -Or at least only delete the keyboard folder
    -Can probably just touch config.h
    -Will that work when other files are also dependent on the flag?

-*Add option to define keys that get scanned every scan, every other key only gets scanned
 every x scans instead for higher update rate (if I can get 8khz to work)
    -Current implementation barely helps
    -8Khz is doable but the PHY situation is annoying
    -Add stuff so that other half also doesn't get scanned
        -Is this necessary when syncing already happens rarely?

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

-Add option to define normal buttons to immediately jump to calibration/bootloader/reset
    -Or even mixed matrices, for encoders etc
    -Make it a separate matrix scan triggered off of a define
    -Maybe just direct pins?
    -Best way would probably be to make the matrix scan task separate, include the needed ones via define
    -Need separate matrix_size definition for he matrix though
    -Also a way to combine both into one complete matrix for the macro?

-Allow associating a color with a profile?
    -Or just set them by layer

-Add key to print current calibration values to console (useful if dynamic calibration is enabled)
    -On split keyboards, paste left and right values separately

-Allow setting heights via layout macro
    -Also get config from heights file in keyboard folder
    -If no heights file is available, use profile info to generate one
    -If x and y are set correctly for the keyboard, add tabs and spaces for a correct visual representation
    -Easy for normal keyboards, hard for split keyboards
    -Need to find a way to split one macro into two arrays

-Add MIDI mode with velocity controlled by the change in adc value
    -Allow triggering past a certain height instead of only at the bottom

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
