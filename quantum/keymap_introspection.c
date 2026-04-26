// Copyright 2022 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

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
// Analog Matrix
//MARK: Definitions

#if defined(ANALOG_MATRIX_ENABLE)
#include "analog_matrix.h"

// Define *_layer variables here for easier usage
#ifdef JOYSTICK_ENABLE
bool joystick_layer = false;
#else
#   define joystick_layer false
#endif
#ifdef MIDI_ENABLE
bool midi_layer = false;
#else
#   define midi_layer false
#endif
#ifdef USE_SPECIAL_MODE
bool special_layer = false;
#else
#   define special_layer false
#endif

#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
extern uint8_t adc_pin_num;
#ifdef MUX_PINS
extern uint8_t mux_channel_num;
#ifndef EQUAL_MUX_PINS
extern uint8_t mux_pin_num;
#ifdef MUX_PINS_RIGHT_CONTINUOUS
extern uint8_t mux_offset;
extern stm32_gpio_t* mux_port;
#endif
#endif // ifndef EQUAL_MUX_PINS
#endif // ifdef MUX_PINS

#ifdef POWER_PINS
//TODO: Add power pin optimizations as well
extern uint8_t power_pin_num;
#endif
extern SPLIT_MUTABLE uint8_t mux_to_num[SMAX(MUX_CHANNELS)][ADC_PIN_NUM];
extern SPLIT_MUTABLE uint8_t num_to_matrix[SMAX(SWITCH_NUM)][2];

#if AM_INIT_KEY_NUM > 0
extern uint8_t init_keys[AM_INIT_KEY_NUM][2];
extern init_func_t init_functions[AM_INIT_KEY_NUM];
#endif

#ifdef PRIORITY_INDICES
// extern uint8_t priority_index_num;
extern SPLIT_VIA_MUT uint8_t priority_indices[SMAX(SWITCH_NUM)];
#endif

// #ifdef MATRIX_TO_NUM_DEF
// extern SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS];
// #endif

//MARK: Split side
// Assigns the split side and copies arrays
void assign_side(void) {
    if (is_keyboard_left()) return;

    #ifdef VIA_ENABLE
    switch_low = SWITCH_NUM_L;
    #endif
    switch_num = SWITCH_NUM_R;
    adc_pin_num = ADC_PIN_NUM_R;
    #ifdef MUX_PINS
    mux_channel_num = MUX_CHANNELS_R;
    #ifndef EQUAL_MUX_PINS
    mux_pin_num = MUX_PIN_NUM_R;
    //TODO: Test optimization defines here (port and offset)
    #ifdef MUX_PINS_RIGHT_CONTINUOUS
    mux_offset = MUX_PIN_RIGHT_OFFSET;
    mux_port = CONTINUOUS_MUX_PORT_RIGHT;
    #endif
    #endif
    #endif
    #ifdef POWER_PINS
    power_pin_num = POWER_PIN_NUM_R;
    #endif

    const uint8_t mux_to_num_r[SMAX(MUX_CHANNELS)][ADC_PIN_NUM] = MUX_TO_NUM_R;
    memcpy(&mux_to_num, &mux_to_num_r, sizeof(mux_to_num_r));
    const uint8_t num_to_matrix_r[SMAX(SWITCH_NUM)][2] = NUM_TO_MATRIX_R;
    memcpy(&num_to_matrix, &num_to_matrix_r, sizeof(num_to_matrix_r));

    // Since matrix_to_num is needed by multiple special features, a define is set when it is first declared
    #if defined MATRIX_TO_NUM_DEF && !defined VIA_ENABLE
    const uint8_t matrix_to_num_r[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM_R;
    memcpy(&matrix_to_num, &matrix_to_num_r, sizeof(matrix_to_num));
    #endif

    #ifndef EQUAL_ADC_PINS
    const pin_t adc_pins_r[ADC_PIN_NUM_R] = ADC_PINS_R;
    memcpy(&adc_pins, &adc_pins_r, sizeof(adc_pins_r));
    #endif
    #if defined MUX_PINS && !defined EQUAL_MUX_PINS
    const pin_t mux_pins_r[MUX_PIN_NUM_R] = MUX_PINS_R;
    memcpy(&mux_pins, &mux_pins_r, sizeof(mux_pins_r));
    #endif
    #if defined POWER_PINS && !defined EQUAL_POWER_PINS
    const pin_t power_pins_r[POWER_PIN_NUM_R] = POWER_PINS_R;
    memcpy(&power_pins, &power_pins_r, sizeof(power_pins_r));
    #endif

    //TODO: Test if this works with uneven amounts per half
    #if SMAX(AM_INIT_KEY_NUM) > 0
    const uint8_t init_keys_r[AM_INIT_KEY_NUM_R][2] = AM_INIT_KEYS_R;
    const init_func_t init_functions_r[AM_INIT_KEY_NUM_R] = AM_INIT_FUNCTIONS_R;
    memcpy(&init_keys, &init_keys_r, sizeof(init_keys_r));
    memcpy(&init_functions, &init_functions_r, sizeof(init_functions_r));
    #endif

    #ifndef VIA_ENABLE
    //TODO: Test prio indices_r
    #ifdef PRIORITY_INDICES
    const uint8_t priority_indices_r[SWITCH_NUM_R] = PRIORITY_INDICES_R;
    memcpy(&priority_indices, &priority_indices_r, sizeof(priority_indices_r));
    // priority_index_num = PRIORITY_INDEX_NUM_R;
    #endif

    const uint8_t key_modes_r[AM_PROFILE_NUM][SWITCH_NUM_R] = KEY_MODES_R;
    memcpy(&am_keyboard_data.key_mode, &key_modes_r, sizeof(key_modes_r));

    #ifdef USE_TRIGGER_HEIGHT
    const height_t trigger_height_r[AM_PROFILE_NUM][SWITCH_NUM_R] = TRIGGER_HEIGHT_R;
    const height_t release_height_r[AM_PROFILE_NUM][SWITCH_NUM_R] = RELEASE_HEIGHT_R;
    memcpy(&am_keyboard_data.trigger_height, &trigger_height_r, sizeof(trigger_height_r));
    memcpy(&am_keyboard_data.release_height, &release_height_r, sizeof(release_height_r));
    #endif
    #if defined RT_PRESS_DISTANCE
    const height_t rt_press_distance_r[AM_PROFILE_NUM][SWITCH_NUM_R] = RT_PRESS_DISTANCE_R;
    const height_t rt_release_distance_r[AM_PROFILE_NUM][SWITCH_NUM_R] = RT_RELEASE_DISTANCE_R;
    memcpy(&am_keyboard_data.rt_press_distance, &rt_press_distance_r, sizeof(rt_press_distance_r));
    memcpy(&am_keyboard_data.rt_release_distance, &rt_release_distance_r, sizeof(rt_release_distance_r));
    #endif
    #endif
}
#else
#   define assign_side()
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Analog Matrix Joystick

#if defined(JOYSTICK_ENABLE)
#include "analog_joystick.h"

// extern SPLIT_MUTABLE uint8_t matrix_to_num[][MATRIX_COLS];

#ifdef SPLIT_KEYBOARD
extern uint8_t thatHand;
#endif

//MARK: joystick mask
// Sets the joystick axes of all keys on the layer
void create_joystick_mask(uint8_t current_layer) {
    // If the previous layer was a joystick layer, clear the joystick_state
    if (joystick_layer) {
        memset(&joystick_state.axes, 0, sizeof(joystick_state.axes));
        joystick_state.dirty = false;
        joystick_layer = false;
    }

    for(uint8_t key = 0; key < switch_num; key++) {
        const uint8_t row = key_config[key].row;
        const uint8_t col = key_config[key].col;
        const uint16_t keycode = keymaps[current_layer][row][col];

        if(IS_ANALOG_JOYSTICK_KEYCODE(keycode)) {
            joystick_layer = true;
            key_config[key].axis_index = keycode - AM_JOYSTICK_RANGE;
        } else {
            key_config[key].axis_index = 255;
        }
    }

    #if defined SPLIT_KEYBOARD && !defined NO_SLAVE_AXES
    // Check the slave half for joystick keys, skip if joystick_layer is already true
    if(is_keyboard_master() && !joystick_layer) {
        for(uint8_t row = thatHand; row < (MATRIX_ROWS_PER_HAND + thatHand); row++) {
            for(uint8_t col = 0; col < MATRIX_COLS; col++) {
                const uint16_t keycode = keymaps[current_layer][row][col];
                if(IS_ANALOG_JOYSTICK_KEYCODE(keycode)) {
                    joystick_layer = true;
                    return;
                }
            }
            if(joystick_layer) return;
        }
    }
    #endif
}
#endif // defined(JOYSTICK_ENABLE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Analog Matrix MIDI

#if defined(MIDI_ENABLE)
// #include "analog_matrix.h"
#include "analog_midi.h"


#endif // defined(MIDI_ENABLE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Analog Matrix Masks

#ifdef VIA_ENABLE
#   include "nvm_dynamic_keymap.h"
extern bool config_update_required;
#endif

// Creates layer masks for all enabled features
void change_layer_settings(uint8_t current_layer) {
    #if defined(JOYSTICK_ENABLE)
    create_joystick_mask(current_layer);
    #endif

    #if defined(MIDI_ENABLE)
    create_midi_mask(current_layer);
    #endif

    #ifdef USE_SPECIAL_MODE
    special_layer = false;
    special_layer |= joystick_layer | midi_layer;
    #endif

    // Update the switch save data during a layer change to cause less disruption during scanning
    #ifdef DYNAMIC_CALIBRATION
    // Not the cleanest way to allow disabling the update check
    #if (!defined AM_NO_EEPROM && RECALIBRATED_SWITCHES < SMAX(SWITCH_NUM) && RECALIBRATED_SWITCHES > 0) || defined VIA_ENABLE
    if(recalibrated_switches >= am_keyboard_data.dc_switch_num) {
        eeconfig_update_keyboard((uint16_t*)&calibration_data);
        recalibrated_switches = 0;
    }
    #endif
    #endif // ifdef DYNAMIC_CALIBRATION

    // Update the VIA config during a layer change to avoid writing to flash too often
    #ifdef VIA_ENABLE
    if(config_update_required) {
        nvm_set_analog_matrix_config(&am_keyboard_data);
        config_update_required = false;
    }
    #endif
}

#endif // if defined(ANALOG_MATRIX_ENABLE)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Community modules (must be last in this file!)

#if defined(COMMUNITY_MODULES_ENABLE)
#    include "community_modules_introspection.c"
#endif // defined(COMMUNITY_MODULES_ENABLE)
