#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "analog_matrix.h"
#include "analog_joystick.h"
#include "info_config.h"
#include "keyboard.h"
#include "matrix.h"

#define clamp_axis(value) (value < 0 ? 0 : (value > 127 ? 127 : value))

#ifndef MATRIX_TO_NUM_DEF
__attribute__((weak)) SPLIT_MUTABLE uint8_t matrix_to_num[][MATRIX_COLS] = MATRIX_TO_NUM;
#   define MATRIX_TO_NUM_DEF
#endif

// Double the amount of axes, since every axis is represented by two components
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
}


//MARK: Translate
//TODO: Is it faster to pass all as params, or just pass index?
// Calculate the joystick component value
bool translate_joystick_axis(uint8_t index, uint16_t scan_value) {
    //TODO: Check if I can use larger values here to avoid the float
    uint16_t* const travel_diff = &key_config[index].joystick_travel;
    uint8_t* const axis_value = &axis_values[key_config[index].axis_index];

    const uint8_t prev_value = *axis_value;

    #ifndef INVERT_ADC
    *axis_value += clamp_axis(((float)(key_config[index].top_value - scan_value - JS_TOP_DEADZONE) * 127) / (float)*travel_diff);
    *axis_value = *axis_value > 127 ? 127 : *axis_value;
    #else
    *axis_value += clamp_axis(((float)(scan_value - key_config[index].top_value - JS_TOP_DEADZONE) * 127) / (float)*travel_diff);
    *axis_value = *axis_value > 127 ? 127 : *axis_value;
    #endif

    // Return true if the value has changed
    return (prev_value == *axis_value);
}


//MARK: Update axis
bool joystick_update_axis(const uint8_t axis, const int16_t value) {
    if (value != joystick_state.axes[axis]) {
        joystick_state.axes[axis] = value;
        joystick_state.dirty      = true;
        return true;
    }
    return false;
}


//MARK: Eval axis
bool evaluate_joystick_axis(axis_component_t axis_component) {
    int16_t joystick_value = 0;
    const uint8_t axis = axis_component >> 1; // Get the index of the axis
    //TODO: Test if it would be better to just change the axis_component instead of making a new var
    const uint8_t axis_index = axis_component & ~1;

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


//MARK: Post process
#ifdef ROUND_STICKS
#include "math.h"
void post_process_sticks(void) {
    // Process left stick (axes 0 and 1)
    #if JOYSTICK_AXIS_COUNT >= 2
    if(joystick_state.axes[0] != 0 && joystick_state.axes[1] != 0) {
        float angle = fabs(atanf((float)joystick_state.axes[1] / joystick_state.axes[0]));
        joystick_state.axes[0] *= cosf(angle);
        joystick_state.axes[1] *= sinf(angle);
    }
    #endif

    // Process right stick (axes 3 and 4)
    #if JOYSTICK_AXIS_COUNT >= 5
    if(joystick_state.axes[3] != 0 && joystick_state.axes[4] != 0) {
        float angle = fabs(atanf((float)joystick_state.axes[4] / joystick_state.axes[3]));
        joystick_state.axes[3] *= cosf(angle);
        joystick_state.axes[4] *= sinf(angle);
    }
    #endif
}
#endif


//MARK: Joystick task
void analog_joystick_task(void) {
    if (!joystick_state.dirty) return;

    // On split keyboards with slave axes, we need to evaluate those on the master as well
    #if defined SPLIT_KEYBOARD && !defined NO_SLAVE_AXES
    for(uint8_t axis = 0; axis < JOYSTICK_AXIS_COUNT; axis++) {
        evaluate_joystick_axis(axis * 2);
    }
    #endif

    #ifdef ROUND_STICKS
    // Adjusts the output of the stick axes to output in a circle
    post_process_sticks();
    #endif

    joystick_flush();

    memset(&axis_values, 0, sizeof(axis_values));
}

