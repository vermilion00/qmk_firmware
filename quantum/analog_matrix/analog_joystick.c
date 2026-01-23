#include <stdint.h>
#include "analog_joystick.h"
#include "analog_matrix.h"
#include "info_config.h"
#include "joystick.h"
#include "keyboard.h"
#ifdef SPLIT_KEYBOARD
#   include "transport.h"
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
    #ifndef INVERT_ADC
    if(scan_value > key_config[index].top_value - JS_TOP_DEADZONE) { key_config[index].trigger_value[active_profile] = 0; }
    else if(scan_value < key_config[index].bottom_value + JS_BOTTOM_DEADZONE) { key_config[index].trigger_value[active_profile] = 127; }
    else {
    //TODO: Test to make sure this doesn't underflow, since it's uint
    //TODO: Test to make sure I need the uint8_t cast here
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

// Update the axis component value, handle conflicts, update joystick
void update_joystick_value(axis_index_t axis_index, uint8_t value) {
    // This has the positive and negative values for each axis separated
    axis_values[axis_index] = value;
    uint8_t axis = axis_index / 2;
    // Get the index of the positive axis part
    axis_index = axis * 2;

    if(!is_keyboard_master()) {
        // Copy the axis_values into the s2m struct immediately
        return;
    }

    //TODO: This won't work, since key events are only updated on the master, meaning slave updates are skipped
    // If both keys are pressed:
    switch(axis_config[axis].resolution) {
        case DIFFERENCE:
            //TODO: Make sure this won't underflow when axis_values is uint8_t
            // Set the output to be the difference between them
            joystick_set_axis(axis, axis_values[axis_index] - axis_values[axis_index + 1]);
            break;
        case LOWEST:
            // Ignore the component that isn't pressed as far
            if(axis_values[axis_index] > axis_values[axis_index + 1]) {
                joystick_set_axis(axis, axis_values[axis_index]);
            } else if (axis_values[axis_index] <= axis_values[axis_index + 1]) {
                joystick_set_axis(axis, -axis_values[axis_index + 1]);
            }
            break;
        case POSITIVE_DOMINANT:
            // Ignore the negative component
            if(axis_values[axis_index] > 0) { joystick_set_axis(axis, axis_values[axis_index]); }
            else { joystick_set_axis(axis, -axis_values[axis_index + 1]); }
            break;
        case NEGATIVE_DOMINANT:
            // Ignore the positive component
            if(axis_values[axis_index + 1] > 0) { joystick_set_axis(axis, -axis_values[axis_index + 1]); }
            else { joystick_set_axis(axis, axis_values[axis_index]); }
            break;
        case CANCEL:
            // Don't output anything
            if(axis_values[axis_index] > 0 && axis_values[axis_index + 1] > 0) {
                joystick_set_axis(axis, 0);
            } else {
                joystick_set_axis(axis, axis_values[axis_index] - axis_values[axis_index + 1]);
            }
            break;
    }
}

#ifdef SPLIT_KEYBOARD
uint8_t prev_slave_axis_values[JOYSTICK_AXIS_COUNT * 2];

//TODO: Add defines to optimise this
// Allow constraining the right axes to the right side etc
bool joystick_post_scan(void) {
    bool changed = false;
    if (is_keyboard_master()) {
        static bool  last_connected                     = false;
        uint8_t slave_axis_values[JOYSTICK_AXIS_COUNT * 2] = {0};
        //TODO: What does this do exactly?
        //TODO: Does this actually copy the slave values into axis_values? If so, what is the cmp for?
        // if (transport_master_if_connected(, slave_axis_values)) {
        // Skip connection checks since we just synced matrix
        if (transport_master(prev_slave_axis_values, slave_axis_values)) {
            //TODO:
            changed = memcmp(prev_slave_axis_values, slave_axis_values, sizeof(slave_axis_values)) != 0;
            last_connected = true;

        } else if (last_connected) {
            // reset other half when disconnected
            memset(slave_axis_values, 0, sizeof(slave_axis_values));
            changed = true;

            last_connected = false;
        }

        if (changed) {
            memcpy(prev_slave_axis_values, slave_axis_values, sizeof(slave_axis_values));
            for(uint8_t index = 0; index < JOYSTICK_AXIS_COUNT * 2; index++) {
                //TODO: Shouldn't need any safeguards as long as each axis index is constrained to one half
                // If one axis keycode is available on both halves, pressing them together will result in an overflow.
                axis_values[index] += slave_axis_values[index];
            }
        }

    } else { transport_slave(prev_slave_axis_values, axis_values); }

    return changed;
}
#endif
