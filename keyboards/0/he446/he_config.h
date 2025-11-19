#pragma once

/*
-Pins are set to input by default -> check if QMK sets unused pins to output, if not diy
-Output is only 3.3v, maybe not enough to change mux channel?
-LED on B2 is active high
-Button on C13 is active high
-Set a default EEPROM configuration when enabling HE, and allow overwriting with a custom config
-Check side and adjust pins if _RIGHT is defined
-ADC Value changes faster at the bottom than at the top, adjust with a lut
    -Allow defining own lut
-Support for switch presets containing travel distance and lut
-If no switch data is in eeprom, immediately go into calibration mode, save data to eeprom when no changes have been made for many cycles, break out
-Allow enabling rapid trigger on specific keys only
-Change system to interrupt based
-Derive num_to_matrix from layout macro instead, basically the same procedure
-Implement from_bottom mode where the heights are defined as the distance from the bottom

-info_defaults don't work?
*/

// Skip EEPROM Reading/Writing
//TODO: Implement EEPROM
#define NO_EEPROM

// TESTING STUFF

//TODO: Implement this?
//      Need to change how calibration and evaluation works, so make it an opt-in thing
//      with the current implementation as the default
// #define DYNAMIC_CALIBRATION

#define TOP_VALUES { 619, 632 }
#define BOTTOM_VALUES { 264, 264 }

// #define TOP_VALUES {[0 ... SWITCH_NUM] = 500 }
// #define BOTTOM_VALUES {[0 ... SWITCH_NUM] = 350 }
