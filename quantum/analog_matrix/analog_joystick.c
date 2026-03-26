#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "analog_joystick.h"
#include "analog_matrix.h"
#include "info_config.h"
#include "keyboard.h"
#include "keycodes.h"
#include "matrix.h"
#include "keymap_introspection.h"
#ifdef SPLIT_KEYBOARD
#   include "transport.h"
#endif

#define clamp_axis(value) (value < 0 ? 0 : (value > 127 ? 127 : value))

//Add curve option
// TODO: Go through and set the smallest type everywhere (including the key_config fields)
SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM;
#ifdef SPLIT_KEYBOARD
uint8_t matrix_to_num_slave[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM_R;
const uint8_t matrix_to_num_r[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM_R;
#endif

// Double the amount of axes, since every axis is represented by two keys
uint8_t axis_values[JOYSTICK_AXIS_COUNT * 2];
analog_joystick_t axis_config[JOYSTICK_AXIS_COUNT] = AM_JOYSTICK_AXIS_CONFIG;

//MARK: Init
void analog_joystick_init(void) {
    memset(&axis_values, 0, sizeof(axis_values));

    for (uint8_t index = 0; index < switch_num; index++) {
        #ifndef INVERT_ADC
        const uint16_t travel = key_config[index].top_value - key_config[index].bottom_value - JS_TOP_DEADZONE - JS_BOTTOM_DEADZONE;
        #else
        const uint16_t travel = key_config[index].bottom_value - key_config[index].top_value - JS_TOP_DEADZONE - JS_BOTTOM_DEADZONE;
        #endif
        #ifdef USE_JOYSTICK
        for (uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            if(key_config[index].mode[profile] == joystick) {
                // If the key mode is set to joystick, repurpose the release_value to hold the travel distance instead to avoid calculating every update
                key_config[index].release_value = travel;
            }
        }
        #else // ifdef USE_JOYSTICK
        key_config[index].joystick_travel = travel;
        #endif // ifdef USE_JOYSTICK
    }

    #ifdef SPLIT_KEYBOARD
    if(is_keyboard_master() && !is_keyboard_left()) {
        memcpy(&matrix_to_num_slave, matrix_to_num, sizeof(matrix_to_num_slave));
    }

    #if KEYBOARD_SIDE == UNKNOWN
    if(!is_keyboard_left()) {
        memcpy(&matrix_to_num, &matrix_to_num_r, sizeof(matrix_to_num));
    }
    #endif
    #endif
}


//MARK: Translate
//TODO: Is it faster to pass all as params, or just pass index?
// Calculate the joystick component value
bool translate_joystick_axis(uint8_t index) {
    //TODO: Check if I can use larger values here to avoid the float
    // Since the trigger value for this profile isn't used anyway, use the field to store the axis value instead
    const uint16_t scan_value = key_config[index].scan_value;
    #ifdef USE_JOYSTICK
    uint16_t* const travel_diff = &key_config[index].release_value[active_profile];
    //TODO: Make sure that the axis index is saved to the trigger value
    uint16_t* const axis_value = &axis_values[key_config[index].trigger_value[active_profile]];
    #else
    uint16_t* const travel_diff = &key_config[index].joystick_travel;
    uint8_t* const axis_value = &axis_values[key_config[index].axis_index];
    #endif

    uint8_t prev_value = *axis_value;

    #ifndef INVERT_ADC
    //TODO: Would it be faster to just let clamping handle it, since it now kinda tests twice?
    if(scan_value > key_config[index].top_value - JS_TOP_DEADZONE) { *axis_value = 0; }
    else if(scan_value < key_config[index].bottom_value) { *axis_value = 127; }
    else { *axis_value = clamp_axis(((float)(key_config[index].top_value - scan_value - JS_TOP_DEADZONE) * 127) / (float)*travel_diff); }

    #else
    if(scan_value < key_config[index].top_value + JS_TOP_DEADZONE) { *axis_value = 0; }
    else if(scan_value > key_config[index].bottom_value - JS_BOTTOM_DEADZONE) { *axis_value = 127; }
    else { *axis_value = clamp_axis(((float)(scan_value - key_config[index].top_value - JS_TOP_DEADZONE) * 127) / (float)*travel_diff); }
    #endif

    // Return true if the value has changed
    return (prev_value == *axis_value);
}


//MARK: Update axis
bool joystick_update_axis(const uint8_t axis, const int16_t value) {
    if (value != joystick_state.axes[axis]) {
        joystick_state.axes[axis] = value;
        joystick_state.dirty      = false;
        return true;
    }
    return false;
}


//MARK: Eval axis
bool evaluate_joystick_axis(axis_name_t axis) {
    int16_t joystick_value = 0;
    const uint8_t axis_index = axis * 2;

    // If both keys are pressed:
    switch(axis_config[axis].resolution) {
        case DIFFERENCE: // Set the output to be the difference between them; default option
            joystick_value = axis_values[axis_index] - axis_values[axis_index + 1];
            break;

        case LOWEST: // Ignore the component that isn't pressed as far
            if(axis_values[axis_index] > axis_values[axis_index + 1]) {
                joystick_value = axis_values[axis_index];
            } else if (axis_values[axis_index] <= axis_values[axis_index + 1]) {
                joystick_value = -axis_values[axis_index + 1];
            }
            break;

        case POSITIVE_DOMINANT: // Ignore the negative component
            if(axis_values[axis_index] > 0) {
                joystick_value = axis_values[axis_index];
            } else {
                joystick_value = -axis_values[axis_index + 1];
            }
            break;

        case NEGATIVE_DOMINANT: // Ignore the positive component
            if(axis_values[axis_index + 1] > 0) {
                joystick_value = -axis_values[axis_index + 1];
            }
            else {
                joystick_value = axis_values[axis_index];
            }
            break;

        case CANCEL: // Don't output anything if both are non-zero
            if(axis_values[axis_index] > 0 && axis_values[axis_index + 1] > 0) {
                joystick_value = 0;
            } else {
                joystick_value = axis_values[axis_index] - axis_values[axis_index + 1];
            }
            break;
    }

    return joystick_update_axis(axis, joystick_value);
}

// extern matrix_row_t matrix[MATRIX_ROWS];
// Clear the matrix and switch state of all joystick keys
//TODO: If this is called on the slave, wouldn't it reset keys outside of array? Is matrix defined as full rows there, or rows per hand?
// void reset_joystick_keys(void) {
//     // Reset the pressed state of all joystick keys to avoid stuck keys
//     for(uint8_t index = 0; index < switch_num; index++) {
//         // Here I could also check if the joystick axis field is set
//         const uint8_t row = key_config[index].row;
//         const uint8_t col = key_config[index].col;

//         if(joystick_mask[row] & 1 << col) {
//             key_config[index].pressed = false;
//         }
//     }
//     // Reset the matrix state of all joystick keys
//     for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
//         matrix[row] &= ~joystick_mask[row];
//     }
// }

//MARK: Joystick task
void analog_joystick_task(void) {
    joystick_flush();
}

