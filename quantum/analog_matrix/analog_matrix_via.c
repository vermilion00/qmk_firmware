#include "analog_matrix.h"
#include "analog_matrix_via.h"
#include "nvm_dynamic_keymap.h"

//TODO: Assign the switch config here
am_keyboard_t am_keyboard_data;

//TODO: Call the am_via_init function that assigns the sides
// I'm already using matrix_to_num for the joystick/midi stuff, how do I guard this properly?
SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM;

void analog_matrix_via_init(void) {
    #ifdef SPLIT_KEYBOARD
    if(!is_keyboard_left()) {
        const uint8_t matrix_to_num_r[MATRIX_ROWS_PER_HAND][MATRIX_COLS] = MATRIX_TO_NUM_R;
        memcpy(&matrix_to_num, &matrix_to_num_r, sizeof(matrix_to_num));
    }
    #endif

    // Read the analog matrix configuration from EEPROM
    nvm_get_analog_matrix_config();
}

// Returns the compressed height (uint8_t instead of float)
height_t get_switch_height(uint8_t key, uint8_t profile, height_addr_t height) {
    return (am_keyboard_data.key[key].profile[profile].raw + (height_t)height);
}

// Returns the actual height as a float
float get_actual_height(uint8_t key, uint8_t profile, height_addr_t height) {
    return ((am_keyboard_data.key[key].profile[profile].raw + (height_t)height) / HEIGHT_MULT);
}

uint8_t get_switch_mode(uint8_t key, uint8_t profile) {
    return (am_keyboard_data.key[key].profile[profile].mode);
}

bool get_switch_priority_mode(uint8_t key, uint8_t profile) {
    return !!(am_keyboard_data.key[key].profile[profile].mode & 0b10000000);
}

void set_switch_height(uint8_t key, uint8_t profile, height_addr_t height, height_t value) {
    am_keyboard_data.key[key].profile[profile].raw = value << ((sizeof(height_t) * 8) * height);
}

void set_switch_mode(uint8_t key, uint8_t profile, key_mode_t mode) {
    am_keyboard_data.key[key].profile[profile].mode = mode;
}

void set_switch_priority_mode(uint8_t key, uint8_t profile, bool priority) {
    if (priority) am_keyboard_data.key[key].profile[profile].mode |= 0b10000000;
    else am_keyboard_data.key[key].profile[profile].mode &= 0b01111111;
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

        case get_keyboard_data: { // I guess I can overwrite the entire message and it still works
            const uint16_t page = (command_data[1] << 8) | command_data[0];
            const uint16_t start = page * RAW_HID_SIZE;
            uint16_t end = start + RAW_HID_SIZE;

            if(start >= sizeof(am_keyboard_t)) return;
            if(end > sizeof(am_keyboard_t)) end = sizeof(am_keyboard_t);

            //TODO: Make sure this offsets correctly, since I can't just index into am_keyboard_data due to profile_layers and priority_profiles
            memcpy(&data, &am_keyboard_data + (uint8_t)start, end - start);
            break;
        }

        case get_switch_profile: {
            const uint8_t row = command_data[0];
            const uint8_t col = command_data[1];
            if(row >= MATRIX_ROWS || col >= MATRIX_COLS) return;
            const uint8_t profile = command_data[2];
            if(profile >= AM_PROFILE_NUM) return;
            const uint8_t index = matrix_to_num[row][col];
            if(index == 255) return;

            // The maximum size for one switch profile is 9 bytes
            memcpy(&command_data[3], &am_keyboard_data.key[index].profile[profile], sizeof(am_switch_profile_t));
        }

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
