#pragma once

// Pull the actual keymap code so that we can inspect stuff from it
// #include KEYMAP_C

/*TODO:
2. Sync calibration to slave
3. EEPROMs
4. LUT

-Change position of init to happen at current position of _force_bootloader?

-Why is the slave joystick not updating?
    -Do the slave joystick matrix positions show as updated?
    -Try setting the key mode to be the actual axis, so that I can set the value there directly instead of saving it to a middleman

-The random keypresses don't seem to be caused by the matrix state being changed?
    -Is the evaluate function returning true for row 0 keys? (Including row 6 on the slave half) - Not the case, it's the joystick keys being active on a non-joystick layer
    -Is the joystick mask fucking up, causing them to be always triggered?
    -It's not actually the numbers being pressed but the keypad keys on the function layer? That seems like the likeliest thing - Yes
        -This would mean that either the master or slave don't switch the mode in time? Prob the slave
        -Since the layer change is only synchronised after the matrix, the key pos are still activated from the joystick keys
            -This means I need to mask the slave joystick keys on the master side when switching layers

-How do the transactions work exactly?
    -I thought running the master transaction called the slave transaction, but looking at matrix_post_scan,
     the slave transaction is just called at the same time the master calls master transaction
    -Can I call them whenever to just copy the data to the shared memory, and the master transaction just grabs that when called?
    -Which probably means I can just place the registrations, and call the handlers myself instead of placing them in transactions_master etc,
     since those get called every scan

-Sending 0 size transactions is possible

-Since the scan value is now saved to the switch, I can update the joystick values in process_* instead of the scan eval function
    -The problem there is that process_* is only triggered when the matrix state changes, so need to set rt and a low distance automatically?
    -Also means that I'd need to either have extra space in the switch config for the joystick diff, or calculate it every update

-Add keycode to lock/unlock switching profiles via layers, to allow combining keycodes and layers

Joystick:
-Currently, the slave side seems to not work, probs because the stuff is just not synced
-Sync the state of the slave axis_values array to master, and evaluate there
    -Possible collisions when both halves write to one axis, but shouldn't be an issue otherwise
-Add option to choose between 4-way and 8-way reporting of DPAD
-If multiple keys have the same axis keycode, |= their results? The first one should set, the second one should |?
-Profile switching is only broken with enabled joystick?
    -Joystick feature, or key_mode on the profile?
-To support multiple keys, the first one should set, the second one should +
    -Or reset state to 0 before and + all of them, pressing multiple keys of the same axis at once is unlikely

-To build own lut curve, add mode where the currently sensed distance and value is printed for a switch

-Enable FPU on supported processors
    -Check if it causes problems?
    -Make sure floats are used, not doubles
    -Not really necessary, just makes init faster

-Split init happens after matrix init, so set a flag in calibration init key func and check that later

-Currently, a 10 bit ADC resolution is hardcoded

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
    -Check if anything needs to be saved at the end of the flash (bootloader flag?)
    -Add check to dynamic calibration to only update config when >n switches need updating
    -Check for update need during housekeeping(?)

-Add option to define normal buttons to immediately jump to calibration/bootloader/reset
    -Or even mixed matrices, for encoders etc
    -Change layout mux validation to allow normal keys?
    -Make it a separate matrix scan triggered off of a define
    -Maybe just direct pins?
    -Best way would probably be to make the matrix scan task separate, include the needed ones via define
    -Need separate matrix_size definition for he matrix though
    -Also a way to combine both into one complete matrix for the macro?

-Allow associating a color with a profile?
    -Or just set them by layer

-Allow setting heights via layout macro
    -Also get config from heights file in keyboard folder
    -If no heights file is available, use profile info to generate one
    -If x and y are set correctly for the keyboard, add tabs and spaces for a correct visual representation
    -Easy for normal keyboards, hard for split keyboards
    -Need to find a way to split one macro into two arrays

-Add MIDI mode with velocity controlled by the change in adc value
    -Allow triggering past a certain height instead of only at the bottom

-Enable interleaved ADC mode for supported mcus

-Make sure no conflicts happen with other features using the ADC

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
