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
#include "multiplexer.h"

#ifdef SPLIT_KEYBOARD
#if KEYBOARD_SIDE == UNKNOWN
extern uint8_t switch_num;
extern uint8_t adc_pin_num;
#ifdef MUX_PINS
extern uint8_t mux_channel_num;
#ifndef EQUAL_MUX_PINS
extern uint8_t mux_pin_num;
#ifdef MUX_PINS_RIGHT_CONTINUOUS
extern uint8_t mux_offset;
extern stm32_gpio_t* mux_port;
#endif
#endif
#endif
#endif

#ifdef POWER_PINS
//TODO: Add power pin optimizations as well
extern uint8_t power_pin_num;
#endif
extern SPLIT_MUTABLE uint8_t mux_to_num[SMAX(MUX_CHANNELS)][ADC_PIN_NUM];
extern SPLIT_MUTABLE uint8_t num_to_matrix[SMAX(SWITCH_NUM)][2];

extern SPLIT_MUTABLE uint8_t key_modes[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];

#if AM_INIT_KEY_NUM > 0
extern uint8_t init_keys[AM_INIT_KEY_NUM][2];
extern init_func_t init_functions[AM_INIT_KEY_NUM];
#endif

#ifdef PRIORITY_INDICES
extern uint8_t priority_index_num;
extern uint8_t priority_indices[SMAX(SWITCH_NUM)];
#endif

//TODO: Figure out proper guards here (keymap config, etc)
#if defined USE_TRIGGER_HEIGHT
extern CONFIG_MUTABLE float trigger_height[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
extern CONFIG_MUTABLE float release_height[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
#endif // if defined USE_TRIGGER_HEIGHT
#if defined USE_RT_DISTANCE
extern float CONFIG_MUTABLE rt_press_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
extern float CONFIG_MUTABLE rt_release_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
#endif // if defined USE_RT_DISTANCE

#endif // defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN

extern CONFIG_MUTABLE uint8_t key_modes[AM_PROFILE_NUM][SWITCH_NUM];
#ifndef KEY_MODES
extern const uint8_t key_modes_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
#endif
#if !defined TRIGGER_HEIGHT && (defined USE_NONE)
extern const float trigger_height_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
extern CONFIG_MUTABLE float trigger_height[AM_PROFILE_NUM][SWITCH_NUM];
extern const float release_height_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
extern CONFIG_MUTABLE float release_height[AM_PROFILE_NUM][SWITCH_NUM];
#endif
#if !defined RT_PRESS_DISTANCE && (defined USE_CONSTANT_RAPID_TRIGGER)
extern CONFIG_MUTABLE float rt_press_distance[AM_PROFILE_NUM][SWITCH_NUM];
extern CONFIG_MUTABLE float rt_release_distance[AM_PROFILE_NUM][SWITCH_NUM];
extern const float rt_press_distance_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
extern const float rt_release_distance_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
#endif

//TODO: I can likely simplify several cases here, some duplicate code
//TODO: Instead of checking for keymap definitions on everything, just force defining everything in one place
//MARK: Split side
// Assigns the split side and copies arrays
void assign_config(bool side) {
#ifdef SPLIT_KEYBOARD
    #if KEYBOARD_SIDE == UNKNOWN
    if(side == RIGHT) {
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

        #if SMAX(AM_INIT_KEY_NUM) > 0
        const uint8_t init_keys_r[AM_INIT_KEY_NUM_R][2] = AM_INIT_KEYS_R;
        const init_func_t init_functions_r[AM_INIT_KEY_NUM_R] = AM_INIT_FUNCTIONS_R;
        memcpy(&init_keys, &init_keys_r, sizeof(init_keys_r));
        memcpy(&init_functions, &init_functions_r, sizeof(init_functions_r));
        #endif

        //TODO: Test this
        #ifdef PRIORITY_INDICES
        const uint8_t priority_indices_r[SMAX(SWITCH_NUM)] = PRIORITY_INDICES_R;
        const uint8_t priority_index_num_r = PRIORITY_INDEX_NUM_R;
        memcpy(&priority_indices, &priority_indices_r, sizeof(priority_indices));
        priority_index_num = priority_index_num_r;
        #endif


        #if defined KEY_MODES
        const uint8_t key_modes_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = KEY_MODES_R;
        memcpy(&key_modes, &key_modes_r, sizeof(key_modes_r));
        #else
        //TODO: Test if this offset works, maybe it's SWITCH_NUM_L - 1
        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            memcpy(&key_modes[profile], &key_mode_config[profile][SWITCH_NUM_L], SWITCH_NUM_R);
        }
        #endif

        #if defined USE_TRIGGER_HEIGHT
        #ifdef TRIGGER_HEIGHT
        const float trigger_height_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = TRIGGER_HEIGHT_R;
        const float release_height_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RELEASE_HEIGHT_R;
        memcpy(&trigger_height, &trigger_height_r, sizeof(trigger_height_r));
        memcpy(&release_height, &release_height_r, sizeof(release_height_r));
        #else
        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            memcpy(&trigger_height[profile], &trigger_height_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float));
            // If release height hasn't been defined, it will be 0 here, so in that case we copy trigger_height instead
            if(release_height_config[profile][0] == 0) { memcpy(&release_height[profile], &trigger_height_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float)); }
            else { memcpy(&release_height[profile], &release_height_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float)); }
        }
        #endif
        #endif
        #if defined USE_RT_DISTANCE
        #if defined RT_PRESS_DISTANCE
        const float rt_press_distance_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RT_PRESS_DISTANCE_R;
        const float rt_release_distance_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RT_RELEASE_DISTANCE_R;
        memcpy(&rt_press_distance, &rt_press_distance_r, sizeof(rt_press_distance_r));
        memcpy(&rt_release_distance, &rt_release_distance_r, sizeof(rt_release_distance_r));
        #else
        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            memcpy(&rt_press_distance[profile], &rt_press_distance_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float));
            if(rt_release_distance_config[profile][0] == 0) { memcpy(&rt_release_distance[profile], &rt_release_distance_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float)); }
            else { memcpy(&rt_release_distance[profile], &rt_release_distance_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float)); }
        }
        #endif
        #endif
    }

    // Keymap config assignment
    #elif KEYBOARD_SIDE == RIGHT
    if(side == RIGHT) {
        // Copy the values for each profile
        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            #ifndef KEY_MODES
            memcpy(&key_modes[profile], &key_modes_config[profile][SWITCH_NUM_L], SWITCH_NUM_R);
            #endif
            #if defined USE_TRIGGER_HEIGHT
            #ifndef TRIGGER_HEIGHT
            memcpy(&trigger_height[profile], &trigger_height_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float));
            if(release_height_config[profile][0] == 0) { memcpy(&release_height[profile], &trigger_height_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float)); }
            else { memcpy(&release_height[profile], &release_height_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float)); }
            #endif
            #endif
            #if defined USE_RT_DISTANCE
            #ifndef RT_PRESS_DISTANCE
            memcpy(&rt_press_distance[profile], &rt_press_distance_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float));
            if(rt_release_distance_config[profile][0] == 0) { memcpy(&rt_release_distance[profile], &rt_release_distance_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float)); }
            else { memcpy(&rt_release_distance[profile], &rt_release_distance_config[profile][SWITCH_NUM_L], SWITCH_NUM_R * sizeof(float)); }
            #endif
            #endif
        }
    }
    #elif KEYBOARD_SIDE == LEFT
    if(side == LEFT) {
        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            #ifndef KEY_MODES
            memcpy(&key_modes[profile], &key_modes_config[profile], SWITCH_NUM_L);
            #endif
            #if defined USE_TRIGGER_HEIGHT
            #ifndef TRIGGER_HEIGHT
            memcpy(&trigger_height[profile], &trigger_height_config[profile], SWITCH_NUM_L * sizeof(float));
            memcpy(&release_height[profile], &release_height_config[profile], SWITCH_NUM_L * sizeof(float));
            #endif
            #endif
            #if defined USE_RT_DISTANCE
            #ifndef RT_PRESS_DISTANCE
            memcpy(&rt_press_distance[profile], &rt_press_distance_config[profile], SWITCH_NUM_L * sizeof(float));
            memcpy(&rt_release_distance[profile], &rt_release_distance_config[profile], SWITCH_NUM_L * sizeof(float));
            #endif
            #endif
        }
    }
    #endif // if KEYBOARD_SIDE == UNKNOWN else
#else // ifdef SPLIT_KEYBOARD
    // Only the keymap config needs to be assigned
    #ifndef KEY_MODES
    memcpy(&key_modes, &key_modes_config, SWITCH_NUM * AM_PROFILE_NUM);
    #endif
    #if defined USE_TRIGGER_HEIGHT
    #ifndef TRIGGER_HEIGHT
    memcpy(&trigger_height, &trigger_height_config, SWITCH_NUM * AM_PROFILE_NUM * sizeof(float));
    memcpy(&release_height, &release_height_config, SWITCH_NUM * AM_PROFILE_NUM * sizeof(float));
    #endif
    #endif
    #if defined USE_RT_DISTANCE
    #ifndef RT_PRESS_DISTANCE
    memcpy(&rt_press_distance, &rt_press_distance_config, SWITCH_NUM * AM_PROFILE_NUM * sizeof(float));
    memcpy(&rt_release_distance, &rt_release_distance_config, SWITCH_NUM * AM_PROFILE_NUM * sizeof(float));
    #endif
    #endif
#endif // ifdef SPLIT_KEYBOARD else
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Analog Matrix Joystick

#if defined(JOYSTICK_ENABLE) && !defined(USE_JOYSTICK)
// #include "analog_matrix.h"
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

            if(key_index == 255) continue;

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
// #include "analog_matrix.h"
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
            const uint8_t key_index = matrix_to_num[row - thisHand][col];
            if(key_index == 255) continue;

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

    //TODO: Add more mask functions here as necessary
}

#endif // if defined(ANALOG_MATRIX_ENABLE)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Community modules (must be last in this file!)

#if defined(COMMUNITY_MODULES_ENABLE)
#    include "community_modules_introspection.c"
#endif // defined(COMMUNITY_MODULES_ENABLE)
