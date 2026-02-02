#include <stdint.h>
#include <stdio.h>
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

#define clamp_axis(value) (value < 0 ? 0 : (value > 127 ? 127 : value))

//Add curve option
//TODO: Go through and set the smallest type everywhere (including the key_config fields)
SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM;
#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
const uint8_t matrix_to_num_r[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM_R;
#endif

// Double the amount of axes, since every axis is represented by two keys
// int16_t axis_values[JOYSTICK_AXIS_COUNT * 2];
uint8_t axis_values[JOYSTICK_AXIS_COUNT * 2];
analog_joystick_t axis_config[JOYSTICK_AXIS_COUNT] = AM_JOYSTICK_AXIS_CONFIG;

//MARK: Init
void analog_joystick_init(void) {
    memset(&axis_values, 0, sizeof(axis_values));

    for (uint8_t index = 0; index < switch_num; index++) {
        #ifdef USE_JOYSTICK
        for (uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            if(key_config[index].mode[profile] == joystick) {
                // If the key mode is set to joystick, repurpose the release_value to hold the travel distance instead to avoid calculating every update
                //TODO: Apply JS_TOP_DEADZONE etc here as well
                #ifndef INVERT_ADC
                key_config[index].release_value[profile] = key_config[index].top_value - JS_TOP_DEADZONE - key_config[index].bottom_value - JS_BOTTOM_DEADZONE;
                #else
                key_config[index].release_value[profile] = key_config[index].bottom_value - JS_TOP_DEADZONE - key_config[index].top_value - JS_BOTTOM_DEADZONE;
                #endif
            }
        }
        #else // ifdef USE_JOYSTICK
        #ifndef INVERT_ADC
        key_config[index].joystick_travel = key_config[index].top_value - JS_TOP_DEADZONE - key_config[index].bottom_value - JS_BOTTOM_DEADZONE;
        #else
        key_config[index].joystick_travel = key_config[index].bottom_value - JS_TOP_DEADZONE - key_config[index].top_value - JS_BOTTOM_DEADZONE;
        #endif
        #endif // ifdef USE_JOYSTICK
    }
}

//MARK: Evaluate
//TODO: Is it faster to pass all as params, or just pass index?
// Calculate the joystick component value
void translate_joystick_axis(uint8_t index) {
    //TODO: Check if I can use larger values here to avoid the float
    // Since the trigger value for this profile isn't used anyway, use the field to store the axis value instead
    const uint16_t scan_value = key_config[index].scan_value;
    #ifdef USE_JOYSTICK
    uint16_t* const travel_diff = &key_config[index].release_value[active_profile];
    uint16_t* const axis_value = &key_config[index].trigger_value[active_profile];
    #else
    uint16_t* const travel_diff = &key_config[index].joystick_travel;
    //TODO: If I decide against this, revert this
    //      Or remove hte joystick_value field
    // uint8_t* const axis_value = &key_config[index].joystick_value;
    uint8_t* const axis_value = &axis_values[key_config[index].axis_index];
    #endif

    #ifndef INVERT_ADC
    if(scan_value > key_config[index].top_value - JS_TOP_DEADZONE) { *axis_value = 0; }
    else if(scan_value < key_config[index].bottom_value + JS_BOTTOM_DEADZONE) { *axis_value = 127; }
    else { *axis_value = clamp_axis((((float)(key_config[index].top_value - scan_value - JS_TOP_DEADZONE)) / *travel_diff) * 127); }

    #else
    if(scan_value < key_config[index].top_value + JS_TOP_DEADZONE) { *axis_value = 0; }
    else if(scan_value > key_config[index].bottom_value - JS_BOTTOM_DEADZONE) { *axis_value = 127; }
    else { *axis_value = clamp_axis((((float)(scan_value - key_config[index].top_value - JS_TOP_DEADZONE)) / *travel_diff) * 127); }
    #endif
}


//MARK: Update axis
// Same as joystick_set_axis except the boundary check
void joystick_update_axis(uint8_t axis, int16_t value) {
    // if (value != joystick_state.axes[axis]) {
        // joystick_state.axes[axis] = value;
        // joystick_state.dirty      = true;
    // }
    joystick_state.axes[axis] = value;
}


//MARK: Eval axis
void evaluate_joystick_axis(axis_name_t axis) {
    int16_t joystick_value = 0;
    uint8_t axis_index = axis * 2;

    // If both keys are pressed:
    switch(axis_config[axis].resolution) {
        case DIFFERENCE:
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

// void update_joystick_value(axis_index_t axis_index, uint8_t value) {

//     // This has the positive and negative values for each axis separated
//     axis_values[axis_index] = value;

//     if(!is_keyboard_master()) {
//         return;
//     }

//     uint8_t axis = axis_index / 2;
//     // Get the index of the positive axis part
//     axis_index = axis * 2;

//     // printf("axis_index: %u\n", axis_index);

//     int16_t joystick_value = 0;

//     // If both keys are pressed:
//     switch(axis_config[axis].resolution) {
//         case DIFFERENCE:
//             //TODO: Make sure this won't underflow when axis_values is uint8_t
//             // Set the output to be the difference between them; default option
//             joystick_value = axis_values[axis_index] - axis_values[axis_index + 1];
//             break;

//         case LOWEST:
//             // Ignore the component that isn't pressed as far
//             if(axis_values[axis_index] > axis_values[axis_index + 1]) {
//                 joystick_value = axis_values[axis_index];
//             } else if (axis_values[axis_index] <= axis_values[axis_index + 1]) {
//                 joystick_value = -axis_values[axis_index + 1];
//             }
//             break;

//         case POSITIVE_DOMINANT:
//             // Ignore the negative component
//             if(axis_values[axis_index] > 0) {
//                 joystick_value = axis_values[axis_index];
//             } else {
//                 joystick_value = -axis_values[axis_index + 1];
//             }
//             break;

//         case NEGATIVE_DOMINANT:
//             // Ignore the positive component
//             if(axis_values[axis_index + 1] > 0) {
//                 joystick_value = -axis_values[axis_index + 1];
//             }
//             else {
//                 joystick_value = axis_values[axis_index];
//             }
//             break;

//         case CANCEL:
//             // Don't output anything
//             if(axis_values[axis_index] > 0 && axis_values[axis_index + 1] > 0) {
//                 joystick_value = 0;
//             } else {
//                 joystick_value = axis_values[axis_index] - axis_values[axis_index + 1];
//             }
//             break;
//     }

//     joystick_update_axis(axis, joystick_value);
// }



//MARK: Joystick task
void analog_joystick_task(void) {
    // if (!joystick_state.dirty) return;
    //TODO: This is just for testing, if it fixes stuff then find a way to avoid calculating all axes
    // for (uint8_t axis = 0; axis < 5; axis++) {
    //     evaluate_joystick_axis(axis);
    // }

    joystick_flush();
}

