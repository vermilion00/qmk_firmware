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
-*Implement dynamic calibration
    -Check if the value is outside the bounds, if yes, save the new value to the switch
    -Then call translation function on that switch
    -Automatically move the bounds towards the center if they haven't been reached in x activations
    -OR: Every few boots the value on every key is moved slightly towards the center, and updated every press

-*Change power_before_scan to always have two active pins, if used ignore power_delay
    -Likely no need to do so since mux channel is changed after, so the delay from that should be enough
    -Gotta test it though, if not enough then change it

-*Poll init keys and call functions appropriately
    -Probably done, just need to add proper bootmagic function
    -Make sure the default bootmagic definition can be used

-Evtl add joystick stuff for analog output
    -Can add option in rt_type, like rt_type 5 -> joystick_right

-Check if I need to rework matrix transformation for split keyboards?
    -Probably not, should just be part of the defining process
    -Probably will have to add the hand offset to the scanning process if split is enabled

-When MUX_PINS isn't defined, compilation fails due to mux_to_num and mux_channels

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
