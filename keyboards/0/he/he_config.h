#pragma once

// Pull the actual keymap code so that we can inspect stuff from it
// #include KEYMAP_C

/*TODO:
2. Sync calibration to slave
3. EEPROMs
4. LUT

// Does this work here? Could be a better way of using the trigger height as the default for the release height
// #pragma comment(linker, "/alternatename: _test=_testDefault")

-Check /data/constants/keycodes, what is it for? Do I need to add my keycodes there somewhere?

-Add profile_state_change function that triggers on profile changes, same as layer_state_change

-Add mux and power pin optimizations for RP2040
    -mux stuff needs to be tested

-Allow toggling priority key mode, with options to enable it on certain profiles automatically

-Take a look at bootmagic to turn on RGB stuff when calibrating, and turn it off when finished?

-Check if setting a larger amount of adc pins than necessary is a problem
-What if one half needs more mux/adc pins than the other?

-Allow setting stuff in config.h directly again
    -To disable config checking, for analog_matrix: true in json->features+

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

-Since the scan value is now saved to the switch, I can update the joystick values in process_* instead of the scan eval function
    -The problem there is that process_* is only triggered when the matrix state changes, so need to set rt and a low distance automatically?
    -Also means that I'd need to either have extra space in the switch config for the joystick diff, or calculate it every update

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
-Slave side axes will only work if the master has access to the slave matrix_to_num at least

MIDI:
-Basically the same logic as the joystick stuff
-Also use a mask with all the notes
-Allow triggering the midi send command off of the trigger height/rt setting
    -When the switch is triggered, the software looks at the change in value between now and the last scan(?) or over multiple and calculates the velocity off of that
    -Maybe even have different modes for how many values it should save to calculate a more precise velocity
-Use a defined multiplier to change the scaling of the velocity, so a difference of 10 could mean 127 or 56 vel
-Use a process_* function similar to the joystick to catch the values

-Currently, a 10 bit ADC resolution is hardcoded

-Change -s flag to force recompilation of config.h instead of deleting build dir
    -Or at least only delete the keyboard folder
    -Can probably just touch config.h
    -Will that work when other files are also dependent on the flag?

-If calibrating, sync calibration start/end and values every print
    -If no_eeprom, sync slave bounds to master for printing

-Currently the _RIGHT stuff isn't being checked in mux and power functions
    -Either disallow setting different ones (preferred), or add support for it (messy)

-ADC Value changes faster at the bottom than at the top, adjust with a lut
    -Allow defining own lut
    -Instead of offsetting the scan value with the lut, use it to offset the height values at init
    -Support for switch presets containing travel distance and lut
    -Instead of lut, use a definable function? Would help with different values

-Try a higher buffer depth and circular buffer?
-Add always inline stuff

-Add mode for multiple actions on one key (DKS) somehow
    -Use switch mode >= 10
    -switch mode - 10 gives us the index in an array of dks structs, where up to 4 heights are mapped to 4 actions

-Add option to define normal buttons to immediately jump to calibration/bootloader/reset
-->Technically, you could use normal buttons by wiring one pin to +V, the other to a mux channel and pull low with a resistor
    -This should work perfectly if invert_adc isn't set
    -If it is, wire one pin to GND and the other to a mux channel with a pull-up resistor

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

-Enable interleaved ADC mode for supported mcus

-Make sure no conflicts happen with other features using the ADC

-Go through all changed files and make sure that all new includes are actually necessary, and gate them if possible

-Dynamic calibration:
    -Set different modes (e.g. only update values during profile/layer change)
    -Only set new values when enough keys need to be changed, to avoid writing to eeprom too often
    -Have different intensities (how often should new values apply?)
    -Calibration threshold should be multiplicative, like 5% off or so

-For split calibration, make a while loop when finished that tries to sync slave calibration values to master if no_eeprom
    -manual transaction function should be available at all times for keycode? Or just remove the keycode


Height assignment through layout macro:
What about copying the keymap principle, then memcpy them into their respective arrays?
    -If I put this in keymap.c, can I use the array in analog_matrix.c?
    -How do I make sure only one is used?
        -I can check if height defines are available in info_config, if not, assign this
    -Need to generate another macro where all keys are in a single array, and no fillers (XXX) are used
    -The params are in the same order as the LAYOUT macro
Assuming I go with the keymap plan, I have two options: either I put all keys into a single line, use that array for the left side, and copy the right half to another array at init
Or I put the halfs into separate arrays here and copy both over
Figure out how to delete the arrays (including const) after they're assigned (how do I use malloc and free?)
Need to put in the USE_* defines somehow though

I don't need the _r side arrays, since I'm copying at init anyways.
I shouldn't even need the fillers, I should be able to define TRIGGER_HEIGHT as {} and then copy into the normal definition, as long as it's not const
Instead of making a separate gate for every height, it would be cleaner to assume that either all or none of the configs are defined through the keymap, and set a define
that changes SPLIT_MUTABLE to not be const, if they're defined in keymap

-Add an option for an offset applied to release height, if it isn't defined
    -As in release height becomes trigger height + offset instead of just trigger height


BUGS:
-A high smoothing value causes keys to get stuck occasionally (40 to replicate)
-The joystick layer doesn't switch off, it goes back to base layer immediately, the update function just doesn't register that
-The joystick layer registers weird keys occasionally, like ctrl when I've unbound it from that position
    -Also specifically the rnx and rpx keys are spammed
    -The joystick layer problems disappear when right y axes keys aren't in the keymap
    -Not a problem with the specific location they're in
-I need to have one more joystick axis than I use in my keymap (or at least I need 6 axes to be able to use RY)
-The value for LPX is updated binarily (not the axis at key T)
    -Is it cuz that's the first index?

-Current code doesn't work with AVR, palSetLineMode etc isn't a thing (but can be fixed, high effort low prio)
-Debug output works better with qhe than qhed
-info_defaults don't work?
-Pins are set to input by default -> QMK sets unused pins to output high (Why not low?)
-LED on B2 is active high
-Button on C13 is active high
*/
