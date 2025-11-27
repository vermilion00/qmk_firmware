#pragma once

/*
-Add more validation stuff
-Add _right options
-ADC Value changes faster at the bottom than at the top, adjust with a lut
    -Allow defining own lut
-Support for switch presets containing travel distance and lut
-*If no switch data is in eeprom, immediately go into calibration mode, save data to eeprom when no changes have been made for many cycles, break out
-Change system to interrupt based?
-Check if trigger height checks happen against the actual switch or not, switch d seems not to check correctly
-Try a higher buffer depth and circular buffer?
-Add always inline stuff
-*Implement dynamic calibration
    -Check if the value is outside the bounds, if yes, save the new value to the switch
    -Then call translation function on that switch
    -Automatically move the bounds towards the center if they haven't been reached in x activations
    -*OR: Every boot the value on every key is moved slightly towards the center, and updated every press

-*Change power_before_scan to always have two active pins, if used ignore power_delay
    -Likely no need to do so since mux channel is changed after, so the delay from that should be enough
    -Gotta test it though, if not enough then change it

-Evtl add joystick stuff for analog output
    -Can add option in rt_type, like rt_type 5 -> joystick_right

-Check if I need to rework matrix transformation for split keyboards?
    -Probably not, should just be part of the defining process
    -Probably will have to add the hand offset to the scanning process if split is enabled

-Add continuous power pins config, based on mux_channel

-EEPROM stuff:
    -Since we only use HE_ADC_RESOLUTION bits, we can use bitfields for the values
    -Size will be n * 20 bits by default, so 1400 bits for 70 keys
    -Check if anything needs to be saved at the end of the flash (bootloader flag?)

-Split stuff:
    -When split is enabled, get the dividing point in rows, and allow same mux combinations for them
    -Compiling with split enabled currently fails, maybe because of missing row pins?
        -I think it's trying to halve the array but it's missing
    -When getting mux_to_num and num_to_matrix from layout,
        -divide them up into left and right by checking row > row_split
        -Assign them to two different const vars, and assign those to a var after checking handedness
        -Get switch_num per side through the split mux_to_num etc, and define the matrix size off of those
    -Maybe split the height values per half as well in python?
        -Either that or save them to both but transform local index to global
            -Have to flip the global_to_local thingy

    -I think my work on separating the mux_to_nums etc was for nought, since each side
     only knows about their own switches
    -Different mux_to_nums should be defined based on presence of _right pins
    -If no right pins are defined when split is enabled, copy contents of pins into _right pins
     and calculate the transformations off of that
    -We don't need to know the global index, only the global row/col of the matrix
    -Add a global variable to the handedness functions to check side

-Add trigger_height_r from config

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
