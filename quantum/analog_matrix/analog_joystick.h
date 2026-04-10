#pragma once

#include <stdint.h>
#include "joystick.h"
#include "analog_matrix.h"
#include "joystick_aliases.h"
#include "matrix.h"
#ifndef USE_JOYSTICK
#include "keymap_introspection.h"
#endif

typedef enum axis_name_t {
    LEFT_X_AXIS = 0,
    LEFT_Y_AXIS,
    TRIGGER_AXIS,
    RIGHT_X_AXIS,
    RIGHT_Y_AXIS,
    RIGHT_Z_AXIS
} axis_name_t;

typedef enum axis_index_t {
    LEFT_POSITIVE_X_INDEX = 0,
    LEFT_NEGATIVE_X_INDEX,
    LEFT_NEGATIVE_Y_INDEX,
    LEFT_POSITIVE_Y_INDEX,
    LEFT_TRIGGER_INDEX,
    RIGHT_TRIGGER_INDEX,
    RIGHT_POSITIVE_X_INDEX,
    RIGHT_NEGATIVE_X_INDEX,
    RIGHT_NEGATIVE_Y_INDEX,
    RIGHT_POSITIVE_Y_INDEX,
    RIGHT_POSITIVE_Z_INDEX,
    RIGHT_NEGATIVE_Z_INDEX
} axis_component_t;

// typedef uint8_t axis_component_t;

typedef enum conflict_options_t {
    DIFFERENCE = 0,
    LOWEST,
    POSITIVE_DOMINANT,
    NEGATIVE_DOMINANT,
    CANCEL
} conflict_options_t;

//TODO: Either add stuff to it or remove the struct
typedef struct analog_joystick_t {
    conflict_options_t resolution;
} analog_joystick_t;

extern SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS];
#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
extern const uint8_t matrix_to_num_r[MATRIX_ROWS_PER_HAND][MATRIX_COLS];
#endif

extern uint8_t axis_values[JOYSTICK_AXIS_COUNT * 2];
extern analog_joystick_t axis_config[JOYSTICK_AXIS_COUNT];

void analog_joystick_init(void);
bool translate_joystick_axis(uint8_t index, uint16_t value);
bool evaluate_joystick_axis(axis_component_t axis_component);
bool joystick_post_scan(void);
void analog_joystick_task(void);
