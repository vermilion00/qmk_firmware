#pragma once

/*TODO:
-Add support for side assignment at init
    -*Get side from split_util.c
    -Change matrixes to pointers
    -Assign the pointers in init according to needs

-When powering sensors via gpio, they need ~7us to be powered and are limited to 25ma
    -Scan rate for 1 sensor is 17160/s (quite slow)
-Each sensor ~4-5 ma, and only 3.3V
-Try using fast transistor, and testing the activation delay then
-If it is still so slow, just power them always, or keep the next 2-3 powered too

-Sync profile state between halves
-If calibrating, sync calibration start/end
-If no_eeprom, sync slave bounds to master for printing

-Try halving left matrix_to_num rows again

-Test an uneven matrix

-Currently the _RIGHT stuff isn't being checked in mux and power functions
    -Either disallow setting different ones (preferred), or add support for it (messy)

-Add more validation stuff
-ADC Value changes faster at the bottom than at the top, adjust with a lut
    -Allow defining own lut
    -Instead of offsetting the scan value with the lut, use it to offset the height values
    -Support for switch presets containing travel distance and lut
-*If no switch data is in eeprom, immediately go into calibration mode, save data to eeprom when no changes have been made for many cycles, break out
-Change system to interrupt based?
-Try a higher buffer depth and circular buffer?
-Add always inline stuff

-Special modes
    -Stored in he_matrix[key].mode[profile], 0 = Default, 1-3 = RT, 10... = Special keys
    -Joystick axes
    -SOCD keys
    -Make a separate json option for SOCD and combine it with RT for eval?
    -Only add the "lowest value" SOCD mode, since the combination with RT makes for too many combos?
    -Dynamic Keystroke (4 press distances with separate actions)

-EEPROM stuff:
    -Since we only use HE_ADC_RESOLUTION bits, we can use bitfields for the values
    -Size will be n * 20 bits by default, so 1400 bits for 70 keys
    -Check if anything needs to be saved at the end of the flash (bootloader flag?)

-Update trigger_height from config for split stuff

-Add json feature option for hall_effect, to apply all the relevant patches

-When using STM32 mcus, set mux pins/power pins via BSR-register instead, should be faster



-Debug output works better with qhe than qhed
-info_defaults don't work?
-Pins are set to input by default -> QMK sets unused pins to output high (Why not low?)
-LED on B2 is active high
-Button on C13 is active high
*/

//DEBUG
// #undef HE_TOP_VALUES
// #define HE_TOP_VALUES {[0 ... SWITCH_NUM] = 630 }
// #undef HE_BOTTOM_VALUES
// #define HE_BOTTOM_VALUES {[0 ... SWITCH_NUM] = 270 }
