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
-Fix matrix_row_pins size check
    -Either automatically declare row_pins[MATRIX_ROWS] = {NO_PIN},
    or better skip check entirely
-Put python functions into separate file, since extra checks would bloat current file too much
-Check if trigger height checks happen against the actual switch or not, switch d seems not to check correctly
-Try a higher buffer depth and circular buffer?
-Implement dynamic calibration as an alternative

-info_defaults don't work?
*/
