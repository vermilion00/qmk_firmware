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
-Check if trigger height checks happen against the actual switch or not, switch d seems not to check correctly
-Try a higher buffer depth and circular buffer?
-Implement dynamic calibration as an alternative
-Make it so that no_eeprom always goes to calibration if no values are defined, and skips it otherwise
-Remove need to have a defined matrix pin
-Add always inline stuff
-Make sure the rt_active check in continuous rt works per key
-To implement dynamic calibration
    -Check if the value is outside the bounds, if yes, save the new value to the switch
    -Then call translation function on that switch
    -Automatically move the bounds towards the center if they haven't been reached in x activations

-To switch config per layer:
    -Make a define for each layer/config combination to allow setting values in the json file,
     then check for which ones are defined
    -To check which layers have special settings, use a uint16_t and mask it when reading
    -Each configuration of heights + rapid_trigger map is called a profile
    -Make functions to switch to each profile
    -Provide keybinds to call these functions
    -Set the layers to activate the profile in profile_1_layers
    -layer_state_set calls these functions when the layer appears in the array
    -If the set layer is in these arrays, remap the heights in the struct
    -Use the layer_state_set_kb function to change the heights for all keys in the key struct appropriately
    -If the layer doesn't appear in these, nothing changes (or remaps to default maybe?)

    -Profile 0 and the config heights are the same, either/or
    -Set an RT mode per profile, and allow turning RT on or off on a per key basis
    -Use a bitmap like current_matrix[], where each row is represented by a matrix_row_t
    -Automatically set HE_PROFILE_NUM based on json

-Refactor matrix so that it's a 2D array of row/col
    -This is to make matrix masking work for per key RT
-Rework matrix into a 3D array of [profile][row][col], and check directly against that
 instead of writing the new values to it on change
    -Takes up less space but might be slower?
-Merge mux to num and num to matrix by defining mux_to_num with [row, col]
 instead of the matrix index, and splitting them up via python
    -Check if the layout array allows defining an extra parameter, which could be the [ADC_PIN][MUX_PIN] combo
-Allow defining custom rapid_trigger_type that allows changing on the fly, make it default
    -If it's not custom, you can leave out the code pertaining to it, if it is, keep it all in
    -Maybe go through all configured profiles and set a define if a type appears in there?

-info_defaults don't work?
*/

//DEBUG
// #undef HE_TOP_VALUES
// #define HE_TOP_VALUES {[0 ... SWITCH_NUM] = 630 }
// #undef HE_BOTTOM_VALUES
// #define HE_BOTTOM_VALUES {[0 ... SWITCH_NUM] = 270 }
