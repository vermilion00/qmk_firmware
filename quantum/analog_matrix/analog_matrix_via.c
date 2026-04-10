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

//TODO: Assign the switch config here
am_keyboard_t am_keyboard_data;

#ifndef MATRIX_TO_NUM_DEF
SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM;
#   define MATRIX_TO_NUM_DEF
#endif

void analog_matrix_via_init(void) {
    //TODO: Make sure that assign side runs early enough for via -> I can just call it after the normal init, no problem
    // #ifdef SPLIT_KEYBOARD
    // if(!is_keyboard_left()) {
    //     const uint8_t matrix_to_num_r[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM_R;
    //     memcpy(&matrix_to_num, &matrix_to_num_r, sizeof(matrix_to_num));
    // }
    // #endif

    // Read the analog matrix configuration from EEPROM
    nvm_get_analog_matrix_config();
}

//TODO: Pass the value by reference, so that it can be changed in case of split keyboard, then easily synced to slave after
// Returns the compressed height (uint8_t instead of float)
height_t get_switch_height(uint8_t key, uint8_t profile, height_addr_t height) {
    return (am_keyboard_data.key[key].profile[profile].raw_height + (height_t)height);
}

// Returns the actual height as a float
float get_actual_height(uint8_t key, uint8_t profile, height_addr_t height) {
    return ((am_keyboard_data.key[key].profile[profile].raw_height+ + (height_t)height) / HEIGHT_MULT);
}

uint8_t get_switch_mode(uint8_t key, uint8_t profile) {
    return (am_keyboard_data.key[key].profile[profile].mode);
}

bool get_switch_priority_mode(uint8_t key, uint8_t profile) {
    return !!(am_keyboard_data.key[key].profile[profile].mode & 0b10000000);
}

void set_switch_height(uint8_t key, uint8_t profile, height_addr_t height, height_t value) {
    am_keyboard_data.key[key].profile[profile].raw_height = value << ((sizeof(height_t) * 8) * height);
}

void set_switch_mode(uint8_t key, uint8_t profile, key_mode_t mode) {
    // Since the MSB represents the priority state, it needs to be masked
    am_keyboard_data.key[key].profile[profile].mode &= 0b10000000;
    am_keyboard_data.key[key].profile[profile].mode |= mode;
}

void set_switch_priority_mode(uint8_t key, uint8_t profile, bool priority) {
    if (priority) am_keyboard_data.key[key].profile[profile].mode |= 0b10000000;
    else am_keyboard_data.key[key].profile[profile].mode &= 0b01111111;
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

            #endif
            break;
        }

        case get_switch_profile: {
            const uint8_t row = command_data[0];
            const uint8_t col = command_data[1];
            if(row >= MATRIX_ROWS || col >= MATRIX_COLS) return;
            const uint8_t index = matrix_to_num[row][col];
            if(index == 255) return;
            const uint8_t profile = command_data[2];
            if(profile >= AM_PROFILE_NUM) return;

            // The maximum size for one switch profile is 9 bytes
            memcpy(&command_data[3], &am_keyboard_data.key[index].profile[profile], sizeof(am_switch_profile_t));
        }

        //TODO: All set commands need to sync on splits
        case set_switch_profile:{
            const uint8_t row = command_data[0];
            const uint8_t col = command_data[1];
            const uint8_t profile = command_data[2];
            const uint8_t index = matrix_to_num[row][col];
        }

        case set_profile_layers:

        case clear_calibration_data:

        case reset_keyboard_data:

        break;
    }
}
