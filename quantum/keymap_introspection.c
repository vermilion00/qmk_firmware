// Copyright 2022 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#ifdef ANALOG_MATRIX_ENABLE
#include "analog_matrix.h"
#ifdef JOYSTICK_ENABLE
#include "analog_matrix/analog_joystick.h"
#endif
#endif
#include "gpio.h"
#include "info_config.h"
#include "keyboard.h"
#include "keycodes.h"
#include "matrix.h"
// #include "analog_matrix/analog_matrix.h"
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

#if defined(COMBO_ENABLE) && !defined(VIAL_COMBO_ENABLE)

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

#if defined(TAP_DANCE_ENABLE) && !defined(VIAL_TAP_DANCE_ENABLE)

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

#if defined(KEY_OVERRIDE_ENABLE) && !defined(VIAL_KEY_OVERRIDE_ENABLE)

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Joystick

#if defined(JOYSTICK_ENABLE) && !defined(USE_JOYSTICK)
bool joystick_layer = false;

matrix_row_t joystick_mask[MATRIX_ROWS];
#ifdef SPLIT_KEYBOARD
extern uint8_t thisHand;
#else
const uint8_t thisHand = 0;
#endif
bool master;

//TODO:
// The switch states of some keys appears to get stuck after switching to the joystick layer/profile, if the joystick stuff is on the slave side
//       mostly number keys? Possibly syncing to the wrong location? The matrix location perhaps
//TODO: keymap timers seem to not work (mouse layer etc) after switching to joystick stuff (only if joystick is on slave?)

//MARK: joystick mask
void create_joystick_mask(uint8_t current_layer) {
    master = is_keyboard_master();
    joystick_layer = false;
    //TODO: Do I even need the joystick mask if I save the axis to the key config directly?
    // Still useful for resetting only the joystick keys, but can be used for other things
    memset(joystick_mask, 0, sizeof(joystick_mask));

    //TODO: Make sure this doesn't cause issues when the joystick layer state is different
    // The master needs to check every keycode, the slave only the keycodes for the slave half
    // printf("Layer: %u\n", current_layer);
    for(uint8_t row = master ? 0 : thisHand; row < (master ? MATRIX_ROWS : (MATRIX_ROWS_PER_HAND + thisHand)); row++) {
        // for(uint8_t row = thisHand; row < (MATRIX_ROWS_PER_HAND + thisHand); row++) {
        //TODO: Is matrix col LTR in matrix array? Cuz that means that joystick_mask is inverted
        //      Doesn't look like it's flipped, but maybe test anyway
        for(uint8_t col = 0; col < MATRIX_COLS; col++) {
            const uint16_t keycode = keymaps[current_layer][row][col];
            // printf("K: %u, ", keycode);
            // const uint16_t keycode = keycode_at_keymap_location(current_layer, row, col);
            // if(IS_QK_JOYSTICK_AXIS(keycode)) {
            const uint8_t key_index = matrix_to_num[row - thisHand][col] - 1;

            if(IS_AM_JOYSTICK_AXIS(keycode)) {
                joystick_layer = true;
                joystick_mask[row] |= 1 << col;
                printf("JS R:%u, C:%u\n", row, col);
                //TODO: Test to make sure this doesn't overflow or smth
                key_config[key_index].axis_index = keycode - QK_AM_JOYSTICK_AXIS;
                // printf("A: %i, ", key_config[key_index].axis_index);
            } else {
                key_config[key_index].axis_index = -1;
            }
        }
        // printf("\n");
    }
}

#endif // defined(JOYSTICK_ENABLE) && !defined(USE_JOYSTICK)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Community modules (must be last in this file!)

#if defined(COMMUNITY_MODULES_ENABLE)
#    include "community_modules_introspection.c"
#endif // defined(COMMUNITY_MODULES_ENABLE)
