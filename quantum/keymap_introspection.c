// Copyright 2022 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#include "gpio.h"
#include "info_config.h"
#include "keyboard.h"
#include "keycodes.h"
#include "matrix.h"
#if defined(COMMUNITY_MODULES_ENABLE)
#    include "community_modules_introspection.h"
#endif // defined(COMMUNITY_MODULES_ENABLE)

// Pull the actual keymap code so that we can inspect stuff from it
#include KEYMAP_C

// Allow for keymap or userspace rules.mk to specify an alternate location for the keymap array
#ifdef INTROSPECTION_KEYMAP_C
#    include INTROSPECTION_KEYMAP_C
#endif // INTROSPECTION_KEYMAP_C

#include "compiler_support.h"
#include "keymap_introspection.h"
#include "util.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Key mapping

#define NUM_KEYMAP_LAYERS_RAW ((uint8_t)(sizeof(keymaps) / ((MATRIX_ROWS) * (MATRIX_COLS) * sizeof(uint16_t))))

uint8_t keymap_layer_count_raw(void) {
    return NUM_KEYMAP_LAYERS_RAW;
}

__attribute__((weak)) uint8_t keymap_layer_count(void) {
    return keymap_layer_count_raw();
}

#ifdef DYNAMIC_KEYMAP_ENABLE
STATIC_ASSERT(NUM_KEYMAP_LAYERS_RAW <= MAX_LAYER, "Number of keymap layers exceeds maximum set by DYNAMIC_KEYMAP_LAYER_COUNT");
#else
STATIC_ASSERT(NUM_KEYMAP_LAYERS_RAW <= MAX_LAYER, "Number of keymap layers exceeds maximum set by LAYER_STATE_(8|16|32)BIT");
#endif

uint16_t keycode_at_keymap_location_raw(uint8_t layer_num, uint8_t row, uint8_t column) {
    if (layer_num < NUM_KEYMAP_LAYERS_RAW && row < MATRIX_ROWS && column < MATRIX_COLS) {
        return pgm_read_word(&keymaps[layer_num][row][column]);
    }
    return KC_TRNS;
}

__attribute__((weak)) uint16_t keycode_at_keymap_location(uint8_t layer_num, uint8_t row, uint8_t column) {
    return keycode_at_keymap_location_raw(layer_num, row, column);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Encoder mapping

#if defined(ENCODER_ENABLE) && defined(ENCODER_MAP_ENABLE)

#    define NUM_ENCODERMAP_LAYERS_RAW ((uint8_t)(sizeof(encoder_map) / ((NUM_ENCODERS) * (NUM_DIRECTIONS) * sizeof(uint16_t))))

uint8_t encodermap_layer_count_raw(void) {
    return NUM_ENCODERMAP_LAYERS_RAW;
}

__attribute__((weak)) uint8_t encodermap_layer_count(void) {
    return encodermap_layer_count_raw();
}

STATIC_ASSERT(NUM_KEYMAP_LAYERS_RAW == NUM_ENCODERMAP_LAYERS_RAW, "Number of encoder_map layers doesn't match the number of keymap layers");

uint16_t keycode_at_encodermap_location_raw(uint8_t layer_num, uint8_t encoder_idx, bool clockwise) {
    if (layer_num < NUM_ENCODERMAP_LAYERS_RAW && encoder_idx < NUM_ENCODERS) {
        return pgm_read_word(&encoder_map[layer_num][encoder_idx][clockwise ? 0 : 1]);
    }
    return KC_TRNS;
}

__attribute__((weak)) uint16_t keycode_at_encodermap_location(uint8_t layer_num, uint8_t encoder_idx, bool clockwise) {
    return keycode_at_encodermap_location_raw(layer_num, encoder_idx, clockwise);
}

#endif // defined(ENCODER_ENABLE) && defined(ENCODER_MAP_ENABLE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Dip Switch mapping

#if defined(DIP_SWITCH_ENABLE) && defined(DIP_SWITCH_MAP_ENABLE)

uint16_t keycode_at_dip_switch_map_location_raw(uint8_t switch_idx, bool on) {
    if (switch_idx < NUM_DIP_SWITCHES) {
        return pgm_read_word(&dip_switch_map[switch_idx][!!on]);
    }
    return KC_TRNS;
}

__attribute__((weak)) uint16_t keycode_at_dip_switch_map_location(uint8_t switch_idx, bool on) {
    return keycode_at_dip_switch_map_location_raw(switch_idx, on);
}

#endif // defined(DIP_SWITCH_ENABLE) && defined(DIP_SWITCH_MAP_ENABLE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Combos

#if defined(COMBO_ENABLE)

uint16_t combo_count_raw(void) {
    return ARRAY_SIZE(key_combos);
}
__attribute__((weak)) uint16_t combo_count(void) {
    return combo_count_raw();
}

STATIC_ASSERT(ARRAY_SIZE(key_combos) <= (QK_KB), "Number of combos is abnormally high. Are you using SAFE_RANGE in an enum for combos?");

combo_t* combo_get_raw(uint16_t combo_idx) {
    if (combo_idx >= combo_count_raw()) {
        return NULL;
    }
    return &key_combos[combo_idx];
}
__attribute__((weak)) combo_t* combo_get(uint16_t combo_idx) {
    return combo_get_raw(combo_idx);
}

#endif // defined(COMBO_ENABLE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tap Dance

#if defined(TAP_DANCE_ENABLE)

uint16_t tap_dance_count_raw(void) {
    return ARRAY_SIZE(tap_dance_actions);
}

__attribute__((weak)) uint16_t tap_dance_count(void) {
    return tap_dance_count_raw();
}

STATIC_ASSERT(ARRAY_SIZE(tap_dance_actions) <= (QK_TAP_DANCE_MAX - QK_TAP_DANCE), "Number of tap dance actions exceeds maximum. Are you using SAFE_RANGE in tap dance enum?");

tap_dance_action_t* tap_dance_get_raw(uint16_t tap_dance_idx) {
    if (tap_dance_idx >= tap_dance_count_raw()) {
        return NULL;
    }
    return &tap_dance_actions[tap_dance_idx];
}

__attribute__((weak)) tap_dance_action_t* tap_dance_get(uint16_t tap_dance_idx) {
    return tap_dance_get_raw(tap_dance_idx);
}

#endif // defined(TAP_DANCE_ENABLE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Key Overrides

#if defined(KEY_OVERRIDE_ENABLE)

uint16_t key_override_count_raw(void) {
    return ARRAY_SIZE(key_overrides);
}

__attribute__((weak)) uint16_t key_override_count(void) {
    return key_override_count_raw();
}

STATIC_ASSERT(ARRAY_SIZE(key_overrides) <= (QK_KB), "Number of key overrides is abnormally high. Are you using SAFE_RANGE in an enum for key overrides?");

const key_override_t* key_override_get_raw(uint16_t key_override_idx) {
    if (key_override_idx >= key_override_count_raw()) {
        return NULL;
    }
    return key_overrides[key_override_idx];
}

__attribute__((weak)) const key_override_t* key_override_get(uint16_t key_override_idx) {
    return key_override_get_raw(key_override_idx);
}

#endif // defined(KEY_OVERRIDE_ENABLE)

#if defined(ANALOG_MATRIX_ENABLE)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Analog Matrix Joystick

#if defined(JOYSTICK_ENABLE) && !defined(USE_JOYSTICK)
#include "analog_matrix.h"
#include "analog_joystick.h"
bool joystick_layer = false;

extern SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS];

matrix_row_t joystick_mask[MATRIX_ROWS];
#ifdef SPLIT_KEYBOARD
extern uint8_t thisHand;
#else
const uint8_t thisHand = 0;
#endif
bool master;

//TODO:
// The switch states of some keys appears to get stuck after switching to the joystick layer/profile, if the joystick stuff is on the slave side
//TODO: keymap timers seem to not work (mouse layer etc) after switching to joystick stuff (only if joystick is on slave?) Specifically mouse layer gets stuck

//TODO: Is the reason for the issues perhaps me trying to read the wrong keys when I'm checking the right half joystick axes on the main half? (In the process_joystick thingy)

//MARK: joystick mask
// Creates a mask of all joystick keycodes in the layer
void create_joystick_mask(uint8_t current_layer) {
    master = is_keyboard_master();
    joystick_layer = false;
    //TODO: Do I even need the joystick mask if I save the axis to the key config directly?
    // Still useful for resetting only the joystick keys, but can be used for other things
    memset(joystick_mask, 0, sizeof(joystick_mask));

    //TODO: Make sure this doesn't cause issues when the joystick layer state is different
    // The master needs to check every keycode, the slave only the keycodes for the slave half
    for(uint8_t row = 0; row < MATRIX_ROWS_PER_HAND; row++) {
    // for(uint8_t row = master ? 0 : thisHand; row < (master ? MATRIX_ROWS : (MATRIX_ROWS_PER_HAND + thisHand)); row++) {
        for(uint8_t col = 0; col < MATRIX_COLS; col++) {
            //TODO: Check if this is the correct row offset in all regards
            //      If this is the only important spot then I can just count to MATRIX_ROWS_PER_HAND here on the slave
            //TODO: This still doesn't work correctly because matrix_to_num only has the info for its half
            //      Current fix is to only check master half for axes
            //      One way of fixing it is to use the row/col info saved to each switch instead of matrix_to_num, and have the key_config array saved to the master, instead of just its half
            uint8_t key_index = matrix_to_num[row][col];
            // uint8_t key_index = matrix_to_num[row - thisHand][col];

            if(key_index == 0) continue;

            key_index -= 1;
            //TODO: When the master checks every row instead of just the master half, fix this
            const uint16_t keycode = keymaps[current_layer][row + thisHand][col];
            if(IS_AM_JOYSTICK_AXIS(keycode)) {
                joystick_layer = true;
                joystick_mask[row] |= 1 << col;
                //TODO: Test to make sure this doesn't overflow or smth
                key_config[key_index].axis_index = keycode - QK_AM_JOYSTICK_AXIS;
            } else {
                key_config[key_index].axis_index = -1;
            }
            // printf("K: %u, A: %i\n", key_index, key_config[key_index].axis_index);
        }
    }
}

#endif // defined(JOYSTICK_ENABLE) && !defined(USE_JOYSTICK)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Analog Matrix MIDI

#if defined(MIDI_ENABLE) && !defined(USE_MIDI)
#include "analog_matrix.h"
#include "analog_midi.h"
bool midi_layer = false;

matrix_row_t midi_mask[MATRIX_ROWS];
#if defined SPLIT_KEYBOARD
//TODO: Will this cause a conflict if joystick is enabled as well, or is it limited to the function scope?
extern uint8_t thisHand;
#else
const uint8_t thisHand = 0;
#endif
bool master;


//MARK: MIDI mask
// Creates a mask of all MIDI keycodes in the layer
void create_midi_mask(uint8_t current_layer) {
    master = is_keyboard_master();
    midi_layer = false;
    memset(midi_mask, 0, sizeof(midi_mask));

    // The master needs to check every keycode, the slave only the keycodes for the slave half
    // printf("Layer: %u\n", current_layer);
    for(uint8_t row = master ? 0 : thisHand; row < (master ? MATRIX_ROWS : (MATRIX_ROWS_PER_HAND + thisHand)); row++) {
        for(uint8_t col = 0; col < MATRIX_COLS; col++) {
            const uint16_t keycode = keymaps[current_layer][row][col];
            const uint8_t key_index = matrix_to_num[row - thisHand][col] - 1;

            if(IS_MIDI_NOTE(keycode)) {
                midi_layer = true;
                midi_mask[row] |= 1 << col;
                // printf("MIDI R:%u, C:%u\n", row, col);
                //TODO: Put whatever logic i'll use here
                // key_config[key_index].axis_index = keycode - QK_AM_JOYSTICK_AXIS;
            } else {
                // key_config[key_index].axis_index = -1;
            }
        }
    }
}

#endif // defined(MIDI_ENABLE) && !defined(USE_JOYSTICK)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Analog Matrix Masks

// Creates layer masks for all enabled features
void change_layer_settings(uint8_t current_layer) {
    #if defined(JOYSTICK_ENABLE) && !defined(USE_JOYSTICK)
    create_joystick_mask(current_layer);
    #endif

    #if defined(MIDI_ENABLE) && !defined(USE_MIDI)
    create_midi_mask(current_layer);
    #endif

    //TODO: Don't really need this, overflow while unused isn't a problem
    // #ifdef PRIORITY_INDICES
    // extern uint8_t scan_amt;
    // scan_amt = 0;
    // #endif

    //TODO: Add more mask functions here as necessary
}

#endif // if defined(ANALOG_MATRIX_ENABLE)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Community modules (must be last in this file!)

#if defined(COMMUNITY_MODULES_ENABLE)
#    include "community_modules_introspection.c"
#endif // defined(COMMUNITY_MODULES_ENABLE)
