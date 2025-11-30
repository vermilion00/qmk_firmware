#pragma once

/*TODO:
-Add support for side assignment at init
    -Get side from split_util.c
    -Change matrixes to pointers
    -Assign the pointers in init according to needs

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
-Support for switch presets containing travel distance and lut
-*If no switch data is in eeprom, immediately go into calibration mode, save data to eeprom when no changes have been made for many cycles, break out
-Change system to interrupt based?
-Check if trigger height checks happen against the actual switch or not, switch d seems not to check correctly
-Try a higher buffer depth and circular buffer?
-Add always inline stuff

-Evtl add joystick stuff for analog output
    -Stored in he_matrix[key].mode[profile], 0 = Default, 1-3 = RT, 10... = Special keys

-EEPROM stuff:
    -Since we only use HE_ADC_RESOLUTION bits, we can use bitfields for the values
    -Size will be n * 20 bits by default, so 1400 bits for 70 keys
    -Check if anything needs to be saved at the end of the flash (bootloader flag?)

-Update trigger_height from config for split stuff

-Add mode where a keystroke is segmented into parts with their own actions

-Add json feature option for hall_effect, to apply all the relevant patches

-Debug output works better with qhe than qhed
-info_defaults don't work?
-Pins are set to input by default -> check if QMK sets unused pins to output high
-LED on B2 is active high
-Button on C13 is active high
*/

//DEBUG
// #undef HE_TOP_VALUES
// #define HE_TOP_VALUES {[0 ... SWITCH_NUM] = 630 }
// #undef HE_BOTTOM_VALUES
// #define HE_BOTTOM_VALUES {[0 ... SWITCH_NUM] = 270 }
