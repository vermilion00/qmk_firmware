#include <stdint.h>
#include <string.h>
#include "analog_joystick.h"
#include "analog_matrix.h"
#include "info_config.h"
#include "joystick.h"
#include "keyboard.h"
#include "keycodes.h"
#include "matrix.h"
#ifdef SPLIT_KEYBOARD
#   include "transport.h"
#endif
#ifndef JOYSTICK_SPECIAL_MODE
#   include KEYMAP_C
#endif

//TODO: Is this a uint8_t?
#define clamp_axis(value) (value > 200 ? 0 : (value > 127 ? 127 : value))
// #define clamp_axis(value) (value < 0 ? 0 : (value > 127 ? 127 : value))

//Add separate smoothing and deadzone options
//Add curve option

SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS][MATRIX_COLS] = MATRIX_TO_NUM;
#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
const uint8_t matrix_to_num_r[MATRIX_ROWS][MATRIX_COLS] = MATRIX_TO_NUM_R;
#endif

// Double the amount of axes, since every axis is represented by two keys
uint8_t axis_values[JOYSTICK_AXIS_COUNT * 2];
analog_joystick_t axis_config[JOYSTICK_AXIS_COUNT] = AM_JOYSTICK_AXIS_CONFIG;

//TODO: Make sure joystick init happens after matrix init
void analog_joystick_init(void) {
    for (uint8_t index = 0; index < JOYSTICK_AXIS_COUNT * 2; index++) {
        axis_values[index] = 0;
    }
    for (uint8_t index = 0; index < switch_num; index ++) {
        for (uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            if(key_config[index].mode[profile] == joystick) {
                // If the key mode is set to joystick, repurpose the release_value to hold the travel distance instead to avoid calculating every update
                //TODO: Apply JS_TOP_DEADZONE etc here as well
                #ifndef INVERT_ADC
                key_config[index].release_value[profile] = key_config[index].top_value - key_config[index].bottom_value;
                #else
                key_config[index].release_value[profile] = key_config[index].bottom_value - key_config[index].top_value;
                #endif
            }
        }
    }
}

//TODO: Is it faster to pass all as params, or just pass index?
// Calculate the joystick component value
void evaluate_joystick_axis(uint8_t index) {
    //TODO: Check if I can use larger values here to avoid the float
    // Since the trigger value for this profile isn't used anyway, use the field to store the axis value instead
    uint16_t scan_value = key_config[index].scan_value;
    //The way these deadzones are applied is probably causing issues with weird output, prob need to apply them to the range calculation in init as well
    #ifndef INVERT_ADC
    if(scan_value > key_config[index].top_value - JS_TOP_DEADZONE) { key_config[index].trigger_value[active_profile] = 0; }
    else if(scan_value < key_config[index].bottom_value + JS_BOTTOM_DEADZONE) { key_config[index].trigger_value[active_profile] = 127; }
    else {
    //TODO: Test to make sure this doesn't underflow, since it's uint
    //TODO: Test to make sure I need the uint8_t cast here
        // The release value contains the difference between the top and bottom value for that switch
        key_config[index].trigger_value[active_profile] = (uint8_t)clamp_axis((((float)(key_config[index].top_value - scan_value)) / key_config[index].release_value[active_profile]) * 127);
    }
    // key_config[index].trigger_value[active_profile] = clamp_axis((((float)(key_config[index].top_value - key_config[index].scan_value)) / (key_config[index].top_value - key_config[index].bottom_value)) * 127);
    #else
    if(scan_value < key_config[index].top_value + JS_TOP_DEADZONE) { key_config[index].trigger_value[active_profile] = 0; }
    else if(scan_value > key_config[index].bottom_value - JS_BOTTOM_DEADZONE) { key_config[index].trigger_value[active_profile] = 127; }
    else {
        key_config[index].trigger_value[active_profile] = clamp_axis((((float)(scan_value - key_config[index].top_value)) / key_config[index].release_value[active_profile]) * 127);
    }
    #endif
}


/*TODO:
Currently, the right half never updates because the array values are updated during process_ and that only triggers on master
host_joystick_send() can be used to send the entire joystick state at once
-Split this into two functions:
    First function updates the array with the new value and sets a flag that joystick position needs to be updated
    Slave only syncs when update flag is set
    Second function runs joystick_set_axis on master and clears update flag
    Clear update flag on slave after sync
    Find a way to bulk update joystick values efficiently
    Does memcmp return the difference between the values, or is it indeterminate
    The problem is the fact that the scan function doesn't know the keycode assigned to it, and therefore can't update the value in the axis array
    Either allow keycode processing for joystick keycodes on slave or get the assigned keycodes from keymap somehow
        -Best way would be to have a keymode for each axis direction and set them during compilation or during init by reading from keymap
        -#include KEYMAP_C should allow me to get keymaps[][][] without a header file
            -Problem here is that layers != profiles, need to find a way around that
            -Perhaps by setting a gamepad layer and profile with defines, and layer_state_set updates profile automatically
*/


bool joystick_layer = false;

matrix_row_t joystick_matrix[MATRIX_ROWS];

// Checks the current layer for joystick axis keycodes, and creates a mask for them
void create_joystick_mask(uint8_t current_layer) {
    joystick_layer = false;
    memset(joystick_matrix, 0, sizeof(joystick_matrix));

    for(uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for(uint8_t col = 0; col < MATRIX_COLS; col++) {
            if(IS_QK_JOYSTICK_AXIS(keymaps[current_layer][row][col])) {
                joystick_matrix[row] |= 1 << col;
                joystick_layer = true;
            }
        }
    }
}


// Same as joystick_set_axis except the boundary check
void joystick_update_axis(uint8_t axis, int16_t value) {
    if (value != joystick_state.axes[axis]) {
        joystick_state.axes[axis] = value;
        joystick_state.dirty      = true;
    }
}


void update_axis_array(axis_index_t axis_index, uint8_t value) {
    axis_values[axis_index] = value;
}

//TODO: Make a function that is called by layer_state_set
//      It will look through the new layer and find all joystick axis keycodes and make a matrix mask, where they are all activated.
//      Then run a process_joystick function afterwards, if on a layer with those keycodes, so set a flag in the new function

// Update the axis component value, handle conflicts, update joystick
void update_joystick_value(axis_index_t axis_index, uint8_t value) {
    // This has the positive and negative values for each axis separated
    axis_values[axis_index] = value;

    if(!is_keyboard_master()) {
        return;
    }

    uint8_t axis = axis_index / 2;
    // Get the index of the positive axis part
    axis_index = axis * 2;

    int16_t joystick_value = 0;

    // If both keys are pressed:
    switch(axis_config[axis].resolution) {
        case DIFFERENCE:
            //TODO: Make sure this won't underflow when axis_values is uint8_t
            // Set the output to be the difference between them; default option
            joystick_value = axis_values[axis_index] - axis_values[axis_index + 1];
            break;

        case LOWEST:
            // Ignore the component that isn't pressed as far
            if(axis_values[axis_index] > axis_values[axis_index + 1]) {
                joystick_value = axis_values[axis_index];
            } else if (axis_values[axis_index] <= axis_values[axis_index + 1]) {
                joystick_value = -axis_values[axis_index + 1];
            }
            break;

        case POSITIVE_DOMINANT:
            // Ignore the negative component
            if(axis_values[axis_index] > 0) {
                joystick_value = axis_values[axis_index];
            } else {
                joystick_value = -axis_values[axis_index + 1];
            }
            break;

        case NEGATIVE_DOMINANT:
            // Ignore the positive component
            if(axis_values[axis_index + 1] > 0) {
                joystick_value = -axis_values[axis_index + 1];
            }
            else {
                joystick_value = axis_values[axis_index];
            }
            break;

        case CANCEL:
            // Don't output anything
            if(axis_values[axis_index] > 0 && axis_values[axis_index + 1] > 0) {
                joystick_value = 0;
            } else {
                joystick_value = axis_values[axis_index] - axis_values[axis_index + 1];
            }
            break;
    }

    joystick_update_axis(axis, joystick_value);
}

void analog_joystick_task(void) {
    if(joystick_state.dirty) {

    }
}

#ifdef SPLIT_KEYBOARD
// uint8_t prev_slave_axis_values[JOYSTICK_AXIS_COUNT * 2];

//TODO: Add defines to optimise this
// Allow constraining the right axes to the right side etc
// bool joystick_post_scan(void) {
//     bool changed = false;
//     if (is_keyboard_master()) {
//         static bool  last_connected                     = false;
//         uint8_t slave_axis_values[JOYSTICK_AXIS_COUNT * 2] = {0};
//         //TODO: What does this do exactly?
//         //TODO: Does this actually copy the slave values into axis_values? If so, what is the cmp for?
//         // if (transport_master_if_connected(, slave_axis_values)) {
//         // Skip connection checks since we just synced matrix
//         if (transport_master(prev_slave_axis_values, slave_axis_values)) {
//             //TODO:
//             changed = memcmp(prev_slave_axis_values, slave_axis_values, sizeof(slave_axis_values)) != 0;
//             last_connected = true;

//         } else if (last_connected) {
//             // reset other half when disconnected
//             memset(slave_axis_values, 0, sizeof(slave_axis_values));
//             changed = true;

//             last_connected = false;
//         }

//         if (changed) {
//             memcpy(prev_slave_axis_values, slave_axis_values, sizeof(slave_axis_values));
//             for(uint8_t index = 0; index < JOYSTICK_AXIS_COUNT * 2; index++) {
//                 //TODO: Shouldn't need any safeguards as long as each axis index is constrained to one half
//                 // If one axis keycode is available on both halves, pressing them together will result in an overflow.
//                 axis_values[index] += slave_axis_values[index];
//             }
//         }

//     } else { transport_slave(prev_slave_axis_values, axis_values); }

//     return changed;
// }
#endif
