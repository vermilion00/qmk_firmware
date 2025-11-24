#pragma once

/*
-Add _right options
-ADC Value changes faster at the bottom than at the top, adjust with a lut
    -Allow defining own lut
-Support for switch presets containing travel distance and lut
-If no switch data is in eeprom, immediately go into calibration mode, save data to eeprom when no changes have been made for many cycles, break out
-Change system to interrupt based
-Check if trigger height checks happen against the actual switch or not, switch d seems not to check correctly
-Try a higher buffer depth and circular buffer?
-Implement dynamic calibration as an alternative
-Add always inline stuff
-To implement dynamic calibration
    -Check if the value is outside the bounds, if yes, save the new value to the switch
    -Then call translation function on that switch
    -Automatically move the bounds towards the center if they haven't been reached in x activations

-Change power_before_scan to always have two active pins, if used ignore power_delay
    -Likely no need to do so since mux channel is changed after, so the delay from that should be enough
    -Gotta test it though, if not enough then change it

-Poll init keys and call functions appropriately
    -Probably done, just need to add proper bootmagic/bootloader function

-Evtl add joystick stuff for analog output
    -Can add option in rt_type, like rt_type 5 -> joystick_right

-Using the bootmagic key will always reset EEPROM!
    -Check if that is only a thing with the key, or a thing with the function
    -If it's a thing with the function, find another way to enter bootloader on press

-Check if I need to rework matrix transformation for split keyboards?
    -Probably not, should just be part of the defining process
    -Probably will have to add the hand offset to the scanning process if split is enabled

-info_defaults don't work?
-Pins are set to input by default -> check if QMK sets unused pins to output, if not diy
-Output is only 3.3v, maybe not enough to change mux channel?
-LED on B2 is active high
-Button on C13 is active high
*/

//DEBUG
// #undef HE_TOP_VALUES
// #define HE_TOP_VALUES {[0 ... SWITCH_NUM] = 630 }
// #undef HE_BOTTOM_VALUES
// #define HE_BOTTOM_VALUES {[0 ... SWITCH_NUM] = 270 }
