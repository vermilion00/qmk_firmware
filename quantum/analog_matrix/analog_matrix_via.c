#include "analog_matrix.h"
#include "analog_matrix_via.h"
#include "nvm_dynamic_keymap.h"
#include "eeprom.h"
#ifdef SPLIT_KEYBOARD
#   include "transactions.h"
#endif
#ifdef MIXED_MATRIX_ENABLE
#   include "mixed_matrix.h"
#endif
#ifdef JOYSTICK_ENABLE
#   include "analog_joystick.h"
#endif

am_keyboard_t am_keyboard_data = {
    #ifdef USE_TRIGGER_HEIGHT
    .trigger_height = TOTAL_TRIGGER_HEIGHT,
    .release_height = TOTAL_RELEASE_HEIGHT,
    #endif
    #ifdef USE_RT_DISTANCE
    .rt_press_distance = TOTAL_RT_PRESS_DISTANCE,
    .rt_release_distance = TOTAL_RT_RELEASE_DISTANCE,
    #endif
    .key_mode = TOTAL_KEY_MODES,
    .profile_num = AM_PROFILE_NUM,
    .profile_config = AM_DEFAULT_PROFILE << 4 | PROFILE_SWITCH_MODE,
    .profile_layers = PROFILE_LAYERS,
    .top_deadzone = USER_TOP_DEADZONE,
    .bottom_deadzone = USER_BOTTOM_DEADZONE,
    .smoothing = ADC_SMOOTHING,
    .top_mult = TOP_DEADZONE_MULT * 100,
    #ifdef VIA_FILTER_STRENGTH
    .filter_strength = FILTER_STRENGTH,
    #endif

    #ifdef USE_PRIORITY_MODE
    .priority_profiles = PRIORITY_PROFILES,
    #endif
    #ifdef DYNAMIC_CALIBRATION
    .dc_switch_num = RECALIBRATED_SWITCHES,
    .dc_factor = AM_DC_FACTOR * 100,
    .dc_delta = AM_DC_DELTA,
    #endif
    #ifdef SPLIT_KEYBOARD
    #ifdef VIA_FILTER_STRENGTH
    .split_filter_strength = SLAVE_FILTER_STRENGTH << 4 | RIGHT_FILTER_STRENGTH,// The first 4 bits show the slave strength, the right 4 the right strength
    #endif
    //TODO: Just define the multiplier differently
    .right_mult = (uint8_t)(RIGHT_MULTIPLIER * 100) & 0x00FF,
    .slave_mult = (uint8_t)(SLAVE_MULTIPLIER * 100) & 0x00FF,
    #endif
};

#ifndef MATRIX_TO_NUM_DEF
__attribute__((weak)) SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM;
#   define MATRIX_TO_NUM_DEF
#endif

void analog_matrix_via_init(void) {
    // Read the analog matrix configuration from EEPROM
    nvm_get_analog_matrix_config();
}

//TODO: Pass the value by reference, so that it can be changed in case of split keyboard, then easily synced to slave after
//TODO: Redo these according to the final impl of am_keyboard_data
// Returns the compressed height (uint8_t instead of float)
height_t get_switch_height(uint8_t key, uint8_t profile, height_addr_t height) {
    switch(height) {
        #ifdef USE_TRIGGER_HEIGHT
        case trigger_height_addr:
            return am_keyboard_data.trigger_height[profile][key];
        case release_height_addr:
            return am_keyboard_data.release_height[profile][key];
        #endif
        #ifdef USE_RT_DISTANCE
        case rt_press_addr:
            return am_keyboard_data.rt_press_distance[profile][key];
        case rt_release_addr:
            return am_keyboard_data.rt_release_distance[profile][key];
        #endif
        default: return 0;
    }
}

// Returns the actual height as a float
float get_actual_height(uint8_t key, uint8_t profile, height_addr_t height) {
    return (float)(get_switch_height(key, profile, height) / HEIGHT_MULT);
}

uint8_t get_switch_mode(uint8_t key, uint8_t profile) {
    return am_keyboard_data.key_mode[profile][key];
}

bool get_switch_priority_mode(uint8_t key, uint8_t profile) {
    return !!(am_keyboard_data.key_mode[profile][key] & 0b10000000);
}

void set_switch_height(uint8_t key, uint8_t profile, height_addr_t height, height_t value) {
    switch(height) {
        #ifdef USE_TRIGGER_HEIGHT
        case trigger_height_addr:
            am_keyboard_data.trigger_height[profile][key] = value;
        case release_height_addr:
            am_keyboard_data.release_height[profile][key] = value;
        #endif
        #ifdef USE_RT_DISTANCE
        case rt_press_addr:
            am_keyboard_data.rt_press_distance[profile][key] = value;
        case rt_release_addr:
            am_keyboard_data.rt_release_distance[profile][key] = value;
        #endif
        default: break;
    }
}

void set_switch_mode(uint8_t key, uint8_t profile, key_mode_t mode) {
    // Since the MSB represents the priority state, it needs to be masked
    am_keyboard_data.key_mode[profile][key] &= 0b10000000;
    am_keyboard_data.key_mode[profile][key] |= mode;
}

void set_switch_priority_mode(uint8_t key, uint8_t profile, bool priority) {
    if (priority) am_keyboard_data.key_mode[profile][key] |= 0b10000000;
    else am_keyboard_data.key_mode[profile][key] &= 0b01111111;
}

void set_deadzones(uint8_t index, uint16_t value) {
    if(index > 2) return; // The index is repurposed as the index of the deadzone that should be changed
    const uint8_t raw_value = CLAMP8(value);
    #ifdef SPLIT_KEYBOARD
    // Sync the new value to the slave
    if(is_keyboard_master()) am_via_manual_transaction(index, 0, split_deadzone, value);

    // Apply the multipliers
    if(!is_keyboard_left()) value = value * (am_keyboard_data.right_mult / 10.0);
    if(!is_keyboard_master()) value = value * (am_keyboard_data.slave_mult / 10.0);
    #endif

    switch(index) {
        case 0:
            // Remove old deadzones, apply the new deadzones
            #ifdef INVERT_ADC
            for(uint8_t key = 0; key < switch_num; key++) key_config[key].top_value = key_config[key].top_value - top_deadzones[key] + value;
            #else
            for(uint8_t key = 0; key < switch_num; key++) key_config[key].top_value = key_config[key].top_value + top_deadzones[key] - value;
            #endif
            memset(&top_deadzones, CLAMP8(value), sizeof(top_deadzones));
            eeconfig_update_deadzone((uint8_t*)&top_deadzones);
            am_keyboard_data.top_deadzone = raw_value;
            break;

        case 1:
            #ifdef INVERT_ADC
            for(uint8_t key = 0; key < switch_num; key++) key_config[key].bottom_value = key_config[key].bottom_value + bottom_deadzone - value;
            #else
            for(uint8_t key = 0; key < switch_num; key++) key_config[key].bottom_value = key_config[key].bottom_value + bottom_deadzone - value;
            #endif
            bottom_deadzone = value;
            am_keyboard_data.bottom_deadzone = raw_value;
            break;

        case 2:
            am_keyboard_data.smoothing = raw_value;
            smoothing = value;
            for(uint8_t key = 0; key < switch_num; key++) translate_mm_to_value(key, false);
            break;
    }
}

// Handles the VIA(L) app HID commands
void analog_matrix_handle_hid(uint8_t *data, uint8_t length) {
    const uint8_t *command_id   = &(data[0]);
    uint8_t *command_data = &(data[1]);
    switch(*command_id) {
        case get_keyboard_size: {
            const uint16_t kbsize = sizeof(am_keyboard_t);
            command_data[0] = (uint8_t)(kbsize & 0x00FF);
            command_data[1] = (uint8_t)((kbsize & 0xFF00) >> 8);
            break;
        }

        case get_keyboard_options: {
            // Make sure 10 bytes are clear
            memset(&command_data[1], 0, 10);
            // Set height options
            #ifdef USE_TRIGGER_HEIGHT
            command_data[1] |= 0b00000001;
            #endif
            #ifdef USE_RT_DISTANCE
            command_data[1] |= 0b00000010;
            #endif
            #ifdef USE_NONE
            command_data[1] |= 0b00000100;
            #endif
            #ifdef USE_RAPID_TRIGGER
            command_data[1] |= 0b00001000;
            #endif
            #ifdef USE_CONTINUOUS_RAPID_TRIGGER
            command_data[1] |= 0b00010000;
            #endif
            #ifdef USE_CONSTANT_RAPID_TRIGGER
            command_data[1] |= 0b00100000;
            #endif

            // Set internal feature options
            #ifdef DYNAMIC_CALIBRATION
            command_data[2] |= 0b00000001;
            #endif
            #ifdef USE_PRIORITY_MODE
            command_data[2] |= 0b00000010;
            #endif
            #ifdef MIXED_MATRIX_ENABLE
            command_data[2] |= 0b00000100;
            #endif
            //TODO: Add SOCD config here

            // Set external feature options
            #ifdef JOYSTICK_ENABLE
            command_data[3] |= 0b00000001;
            #endif
            #ifdef MIDI_ENABLE
            command_data[3] |= 0b00000010;
            #endif
        }

        case get_keyboard_data: {
            const uint16_t page = (command_data[1] << 8) | command_data[0];
            const uint16_t start = page * RAW_HID_SIZE;
            if(start >= sizeof(am_keyboard_t)) return;
            uint16_t end = start + RAW_HID_SIZE;
            if(end > sizeof(am_keyboard_t)) end = sizeof(am_keyboard_t);

            //TODO: Make sure this offsets correctly, since I can't just index into am_keyboard_data
            memcpy(&data, &am_keyboard_data + (uint8_t)start, end - start);
            break;
        }

        case get_mixed_matrix: {
            #ifdef MIXED_MATRIX_ENABLE
            // Transfer matrix positions of all mech keys, so that the GUI can mark them appropriately
            //->RC_NUM_TO_MATRIX

            #endif
            break;
        }

        case set_profile_layers:

        case clear_calibration_data:

        case reset_keyboard_data:

        break;
    }
}
