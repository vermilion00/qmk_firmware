#include "analog_matrix.h"
#include "analog_matrix_via.h"
#include "nvm_dynamic_keymap.h"
#include "eeprom.h"
#include "print.h" //TODO: RM
#ifdef SPLIT_KEYBOARD
#   include "transactions.h"
#endif
#ifdef MIXED_MATRIX_ENABLE
#   include "mixed_matrix.h"
extern SPLIT_MUTABLE uint8_t rc_to_matrix[ROW_PIN_NUM][COL_PIN_NUM][2];
#endif
#ifdef JOYSTICK_ENABLE
#   include "analog_joystick.h"
#endif

//TODO: RM once a better way is found
bool config_update_required = false;

uint16_t via_scan_value = 0;
uint8_t via_scan_index = 255;

//MARK: am_keyboard_t
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

    #ifdef USE_PRIORITY_MODE
    .priority_profiles = PRIORITY_PROFILES,
    #endif
    #ifdef DYNAMIC_CALIBRATION
    .dc_switch_num = RECALIBRATED_SWITCHES,
    .dc_factor = AM_DC_FACTOR * 100,
    .dc_delta = AM_DC_DELTA,
    #endif
    #ifdef SPLIT_KEYBOARD
    //TODO: Just define the multiplier differently
    .right_mult = (uint8_t)(RIGHT_MULTIPLIER * 100),
    .slave_mult = (uint8_t)(SLAVE_MULTIPLIER * 100),
    #ifdef VIA_FILTER_STRENGTH
    .split_filter_strength = SLAVE_FILTER_STRENGTH << 4 | RIGHT_FILTER_STRENGTH,// The first 4 bits show the slave strength, the right 4 the right strength
    #endif
    #endif // ifdef SPLIT_KEYBOARD
    #ifdef VIA_FILTER_STRENGTH
    .filter_strength = FILTER_STRENGTH,
    #endif
    .keyboard_size = sizeof(am_keyboard_t),
};

const uint8_t matrix_to_num[MATRIX_ROWS][MATRIX_COLS] = FULL_MATRIX_TO_NUM;

//MARK: VIA init
void analog_matrix_via_init(void) {
    // Read the analog matrix configuration from EEPROM
    am_keyboard_t eeprom_data;
    nvm_get_analog_matrix_config(&eeprom_data);

    if(eeprom_data.keyboard_size == sizeof(am_keyboard_t)) {
        // Replace the json configuration with the saved configuration if the sizes match
        memcpy(&am_keyboard_data, &eeprom_data, sizeof(am_keyboard_data));
    } else {
        // Overwrite the saved keyboard data with the json config if they don't
        nvm_set_analog_matrix_config(&am_keyboard_data);
    }
}

// Returns the compressed height (uint8_t instead of float)
// All following functions take the global switch index
//MARK: get height
height_t get_switch_height(uint8_t key, uint8_t profile, height_addr_t height) {
    switch(height) {
        #ifdef USE_TRIGGER_HEIGHT
        case trigger_height_addr:
            return am_keyboard_data.trigger_height[profile][key];
            break;
        case release_height_addr:
            return am_keyboard_data.release_height[profile][key];
            break;
        #endif
        #ifdef USE_RT_DISTANCE
        case rt_press_addr:
            return am_keyboard_data.rt_press_distance[profile][key];
            break;
        case rt_release_addr:
            return am_keyboard_data.rt_release_distance[profile][key];
            break;
        #endif
        default: return 255;
    }
}

// Returns the actual height as a float
// float get_actual_height(uint8_t key, uint8_t profile, height_addr_t height) {
//     return (float)(get_switch_height(key, profile, height) / HEIGHT_MULT);
// }

uint8_t get_switch_mode(uint8_t key, uint8_t profile) {
    return am_keyboard_data.key_mode[profile][key];
}

bool get_switch_priority_mode(uint8_t key, uint8_t profile) {
    return !!(am_keyboard_data.key_mode[profile][key] & 0b10000000);
}

//MARK: Set height
void set_switch_height(uint8_t key, uint8_t profile, height_addr_t height, height_t value) {
    switch(height) {
        #ifdef USE_TRIGGER_HEIGHT
        case trigger_height_addr:
            am_keyboard_data.trigger_height[profile][key] = value;
            break;
        case release_height_addr:
            am_keyboard_data.release_height[profile][key] = value;
            break;
        #endif
        #ifdef USE_RT_DISTANCE
        case rt_press_addr:
            am_keyboard_data.rt_press_distance[profile][key] = value;
            break;
        case rt_release_addr:
            am_keyboard_data.rt_release_distance[profile][key] = value;
            break;
        #endif
        default: return;
    }

    if((key < (switch_num + switch_low)) && (key >= switch_low)) {
        translate_mm_to_value((key - switch_low), false);
    }
}

void set_switch_mode(uint8_t key, uint8_t profile, key_mode_t mode) {
    // Since the MSB represents the priority state, it needs to be masked
    am_keyboard_data.key_mode[profile][key] &= 0b10000000;
    am_keyboard_data.key_mode[profile][key] |= mode;
    //TODO: Make sure that the mode passed here doesn't contain prio info
    if((key < switch_num + switch_low) && key >= switch_low) key_config[key - switch_low].mode[profile] = mode;
    if(profile == active_profile) change_layer_settings(am_highest_layer);
}

void set_switch_priority_mode(uint8_t key, uint8_t profile, bool priority) {
    if (priority) am_keyboard_data.key_mode[profile][key] |= 0b10000000;
    else am_keyboard_data.key_mode[profile][key] &= 0b01111111;

    #ifdef PRIORITY_INDICES
    if((key < switch_num + switch_low) && key >= switch_low) priority_indices[key - switch_low] = priority;
    #endif
}

#ifdef USE_PRIORITY_MODE
void set_priority_profiles(uint16_t value) {
    am_keyboard_data.priority_profiles = value;
}
#endif

void set_profile_layer_state(uint8_t profile, layer_state_t value) {
    am_keyboard_data.profile_layers[profile] = value;
}

// Returns true if the state of profile_lock is updated
bool get_profile_lock_save_state(void) {
    return !!(am_keyboard_data.profile_num & 0b01000000);
}

// If true, the state of profile_lock will be saved
void set_profile_lock_save_state(bool state) {
    if(state) am_keyboard_data.profile_num |= 0b01000000;
    else am_keyboard_data.profile_num &= 0b10111111;
}

void set_profile_num(uint8_t profiles) {
    am_keyboard_data.profile_num &= 0b11110000;
    am_keyboard_data.profile_num |= profiles;
}

void set_profile_config(uint8_t config) {
    am_keyboard_data.profile_config = config;
}

//MARK: Set deadzones
void set_deadzones(uint8_t index, uint16_t value) {
    const uint8_t raw_value = CLAMP8(value);
    #ifdef SPLIT_KEYBOARD
    //TODO: Change this according to impl, but I currently don't want to multiply the values at all since I'm setting the user values
    //      How should I do smoothing though, since that isn't divided yet?
    // Apply the multipliers
    if(!is_keyboard_left()) value = value * (am_keyboard_data.right_mult / 100.0);
    if(!is_keyboard_master()) value = value * (am_keyboard_data.slave_mult / 100.0);
    #endif

    //TODO: Since I only want to change the user deadzone value, not the ADC deadzone by via, I need to make sure that this is correct
    //      Perhaps allow different fields to control both values?
    switch(index) {
        case 0: { // Top deadzone
            // Remove old deadzones, apply the new deadzones
            #ifdef INVERT_ADC
            for(uint8_t key = 0; key < switch_num; key++) key_config[key].top_value = key_config[key].top_value - top_deadzones[key] + value;
            #else
            for(uint8_t key = 0; key < switch_num; key++) key_config[key].top_value = key_config[key].top_value + top_deadzones[key] - value;
            #endif
            //TODO: Does top_deadzones save the ADC deadzone or both? Should be just the ADC
            memset(&top_deadzones, CLAMP8(value), sizeof(top_deadzones));
            //TODO: Perhaps don't update eeprom here
            eeconfig_update_deadzone((uint8_t*)&top_deadzones);
            am_keyboard_data.top_deadzone = raw_value;
            break;
        }

        case 1: { // Bottom deadzone
            #ifdef INVERT_ADC
            for(uint8_t key = 0; key < switch_num; key++) key_config[key].bottom_value = key_config[key].bottom_value + bottom_deadzone - value;
            #else
            for(uint8_t key = 0; key < switch_num; key++) key_config[key].bottom_value = key_config[key].bottom_value + bottom_deadzone - value;
            #endif
            bottom_deadzone = value;
            am_keyboard_data.bottom_deadzone = raw_value;
            break;
        }

        case 2: { // Smoothing
            am_keyboard_data.smoothing = raw_value;
            smoothing = value;
            for(uint8_t key = 0; key < switch_num; key++) translate_mm_to_value(key, false);
            break;
        }

        case 3: { // Top mult
            uint8_t prev_deadzones[SMAX(SWITCH_NUM)];
            memcpy(&prev_deadzones, &top_deadzones, sizeof(top_deadzones));
            float top_mult = am_keyboard_data.top_mult / 100.0;
            for(uint8_t key = 0; key < switch_num; key++) top_deadzones[key] /= top_mult;
            am_keyboard_data.top_mult = raw_value;
            top_mult = raw_value / 100.0;
            for(uint8_t key = 0; key < switch_num; key++) {
                top_deadzones[key] = CLAMP8(top_deadzones[key] * top_mult);
                #ifdef INVERT_ADC
                key_config[key].top_value = key_config[key].top_value - prev_deadzones[key] + top_deadzones[key];
                #else
                key_config[key].top_value = key_config[key].top_value + prev_deadzones[key] - top_deadzones[key];
                #endif
            }
            break;
        }

        #ifdef SPLIT_KEYBOARD
        case 4: { // Right mult
            am_keyboard_data.right_mult = raw_value;
            if(is_keyboard_left()) return;

            uint16_t prev_bottom = bottom_deadzone;
            uint8_t prev_deadzones[SMAX(SWITCH_NUM)];
            memcpy(&prev_deadzones, &top_deadzones, sizeof(top_deadzones));
            float right_mult = am_keyboard_data.right_mult / 100.0;
            for(uint8_t key = 0; key < switch_num; key++) top_deadzones[key] /= right_mult;
            bottom_deadzone = (bottom_deadzone - am_keyboard_data.bottom_deadzone) / right_mult;
            right_mult = raw_value / 100.0;
            bottom_deadzone = bottom_deadzone * right_mult + am_keyboard_data.bottom_deadzone;
            for(uint8_t key = 0; key < switch_num; key++) {
                top_deadzones[key] = CLAMP8(top_deadzones[key] * right_mult);
                #ifdef INVERT_ADC
                key_config[key].top_value = key_config[key].top_value - prev_deadzones[key] + top_deadzones[key];
                key_config[key].bottom_value = key_config[key].bottom_value + prev_bottom - bottom_deadzone;
                #else
                key_config[key].top_value = key_config[key].top_value + prev_deadzones[key] - top_deadzones[key];
                key_config[key].bottom_value = key_config[key].bottom_value - prev_bottom + bottom_deadzone;
                #endif
            }
            break;
        }

        case 5: { // Slave mult
            am_keyboard_data.slave_mult = raw_value;
            if(is_keyboard_master()) return;

            uint16_t prev_bottom = bottom_deadzone;
            uint8_t prev_deadzones[SMAX(SWITCH_NUM)];
            memcpy(&prev_deadzones, &top_deadzones, sizeof(top_deadzones));
            float slave_mult = am_keyboard_data.slave_mult / 100.0;
            for(uint8_t key = 0; key < switch_num; key++) top_deadzones[key] /= slave_mult;
            bottom_deadzone = (bottom_deadzone - am_keyboard_data.bottom_deadzone) / slave_mult;
            slave_mult = raw_value / 100.0;
            bottom_deadzone = bottom_deadzone * slave_mult + am_keyboard_data.bottom_deadzone;
            for(uint8_t key = 0; key < switch_num; key++) {
                top_deadzones[key] = CLAMP8(top_deadzones[key] * slave_mult);
                #ifdef INVERT_ADC
                key_config[key].top_value = key_config[key].top_value - prev_deadzones[key] + top_deadzones[key];
                key_config[key].bottom_value = key_config[key].bottom_value + prev_bottom - bottom_deadzone;
                #else
                key_config[key].top_value = key_config[key].top_value + prev_deadzones[key] - top_deadzones[key];
                key_config[key].bottom_value = key_config[key].bottom_value - prev_bottom + bottom_deadzone;
                #endif
            }
            break;
        }

        #ifdef VIA_FILTER_STRENGTH
        case 6: { // Split right filter strength
            //TODO: Do more here, like changing the actual filter function
            am_keyboard_data.split_filter_strength = raw_value;
            break;
        }
        #endif // ifdef VIA_FILTER_STRENGTH
        #endif // ifdef SPLIT_KEYBOARD

        #ifdef VIA_FILTER_STRENGTH
        #error "VIA_FILTER_STRENGTH is currently not working!"
        case 7: { // Split slave filter strength
            am_keyboard_data.split_filter_strength = raw_value << 4; // The leftshift is to signify which option is used, but only one can be used at a time
            break;
        }
        #endif // ifdef VIA_FILTER_STRENGTH
    }
}

#ifdef DYNAMIC_CALIBRATION
// 0 = Switch num, 1 = Factor, 2 = Delta
void set_dynamic_calibration(uint8_t index, uint8_t value) {
    if(index > 2) return;
    //TODO: Make sure this is correct
    *((uint8_t*)&am_keyboard_data.dc_switch_num + index) = value;
}
#endif

void via_transfer_calibration(uint8_t *data, uint8_t index, uint8_t page) {
    const uint16_t start = page * RAW_HID_SIZE;
    if(start >= 2 * TOTAL_SWITCH_NUM) return;
    uint16_t end = start + RAW_HID_SIZE;
    if(end > 2 * TOTAL_SWITCH_NUM) end = 2 * TOTAL_SWITCH_NUM;
    uint16_t transfer_data[TOTAL_SWITCH_NUM];
    uint8_t thisHand_index = 0;

    #ifdef SPLIT_KEYBOARD
    uint8_t thatHand_index = SWITCH_NUM_L;
    const uint8_t thatHand_switch_num = TOTAL_SWITCH_NUM - switch_num;
    if(!is_keyboard_left()) {
        thisHand_index = SWITCH_NUM_L;
        thatHand_index = 0;
    }
    #endif

    if(index == 0) { // Transfer the top values
        //TODO: Apply the correct deadzone offsets here (per switch and side etc) -> Perhaps need to save the actual uint16_t top deadzones per switch as well
        for(uint8_t key = 0; key < switch_num; key++) {
            #ifdef INVERT_ADC
            transfer_data[thisHand_index + key] = key_config[key].top_value - top_deadzones[key];
            #else
            transfer_data[thisHand_index + key] = key_config[key].top_value + top_deadzones[key];
            #endif
        }
        #ifdef SPLIT_KEYBOARD
        memcpy((uint8_t*)&transfer_data + thatHand_index * 2, (uint8_t*)&split_shmem->top_cal_data, thatHand_switch_num * 2);
        #endif
    } else {         // Transfer the bottom values
        memcpy((uint8_t*)&transfer_data + thisHand_index * 2, (uint8_t*)&calibration_data, switch_num * 2);
        #ifdef SPLIT_KEYBOARD
        memcpy((uint8_t*)&transfer_data + thatHand_index * 2, (uint8_t*)&split_shmem->cal_data, thatHand_switch_num * 2);
        #endif
    }

    memcpy(data, (uint8_t*)&transfer_data + start, end - start);
}

//MARK: Default config
void apply_default_config(am_keyboard_t* keyboard_data) {
    const am_keyboard_t default_data = {
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
        .right_mult = RIGHT_MULTIPLIER * 100,
        .slave_mult = SLAVE_MULTIPLIER * 100,
        #endif
        .keyboard_size = sizeof(am_keyboard_t),
    };

    memcpy(&keyboard_data, &default_data, sizeof(am_keyboard_t));
}

//MARK: HID handler
// Handles the VIA(L) app HID commands
void analog_matrix_handle_hid(uint8_t *data, uint8_t length) {
    // The 0 index contains the AM_PREFIX
    const uint8_t *command_id   = &(data[1]);
    uint8_t *command_data = &(data[2]);
    // Split keyboard transaction values
    __attribute__((unused)) uint8_t index = 255;
    __attribute__((unused)) uint8_t profile = 255;
    __attribute__((unused)) uint8_t split_id = split_no_transfer;
    __attribute__((unused)) uint16_t value = 0;

    switch(*command_id) {
        case get_keyboard_def_id: {
            // Get the active profile
            if(command_data[0] == 1) {
                command_data[0] = active_profile;
                break;
            }

            if(command_data[0] == 2) {
                void sync_calibration_values(bool init);
                sync_calibration_values(false);
                via_transfer_calibration(data, command_data[1], command_data[2]);
                break;
            }

            // Make sure the used bytes are clear
            memset(&command_data[0], 0, 20);

            const uint16_t kbsize = sizeof(am_keyboard_t);
            command_data[0] = (uint8_t)(kbsize & 0x00FF);
            command_data[1] = (uint8_t)((kbsize & 0xFF00) >> 8);

            // Set height options
            #ifdef USE_TRIGGER_HEIGHT
            command_data[2] |= 0b00000001;
            #endif
            #ifdef USE_RT_DISTANCE
            command_data[2] |= 0b00000010;
            #endif
            #ifdef USE_NONE
            command_data[2] |= 0b00000100;
            #endif
            #ifdef USE_RAPID_TRIGGER
            command_data[2] |= 0b00001000;
            #endif
            #ifdef USE_CONTINUOUS_RAPID_TRIGGER
            command_data[2] |= 0b00010000;
            #endif
            #ifdef USE_CONSTANT_RAPID_TRIGGER
            command_data[2] |= 0b00100000;
            #endif
            #ifdef HIGH_HEIGHT_RESOLUTION
            command_data[2] |= 0b01000000;
            #endif

            // Set internal feature options
            #ifdef DYNAMIC_CALIBRATION
            command_data[3] |= 0b00000001;
            #endif
            #ifdef USE_PRIORITY_MODE
            command_data[3] |= 0b00000010;
            #endif
            #ifdef MIXED_MATRIX_ENABLE
            command_data[3] |= 0b00000100;
            #endif
            #ifdef VIA_FILTER_STRENGTH
            command_data[3] |= 0b00001000;
            #endif
            #ifdef INVERT_ADC
            command_data[3] |= 0b00010000;
            #endif
            //TODO: Add SOCD config here

            // Set external feature options
            #ifdef JOYSTICK_ENABLE
            command_data[4] |= 0b00000001;
            #endif
            #ifdef MIDI_ENABLE
            command_data[4] |= 0b00000010;
            #endif
            #ifdef SPLIT_KEYBOARD
            command_data[4] |= 0b00000100;
            #endif

            //TODO: Prob don't need this at all
            #ifdef MIXED_MATRIX_ENABLE
            command_data[5] = RC_SWITCH_NUM;
            #endif

            command_data[6] = AM_PROFILE_NUM;
            command_data[7] = am_keyboard_data.profile_num;
            command_data[8] = TOTAL_SWITCH_NUM;
            command_data[9] = SWITCH_NUM;
            //TODO: Prob don't need this at all
            #ifdef RC_SWITCH_NUM
            command_data[10] = RC_SWITCH_NUM;
            #endif
            command_data[11] = sizeof(layer_state_t);
            command_data[12] = MATRIX_ROWS;
            command_data[13] = MATRIX_COLS;
            command_data[14] = (TRAVEL_DISTANCE * 100) & 0x00FF;
            command_data[15] = ((TRAVEL_DISTANCE * 100) & 0xFF00) >> 8;
            command_data[16] = active_profile;
            break;
        }

        case get_keyboard_data_id: {
            const uint16_t page = (command_data[1] << 8) | command_data[0];
            const uint16_t start = page * RAW_HID_SIZE;
            if(start >= sizeof(am_keyboard_t)) return;
            uint16_t end = start + RAW_HID_SIZE;
            if(end > sizeof(am_keyboard_t)) end = sizeof(am_keyboard_t);

            memcpy(data, ((uint8_t*)&am_keyboard_data) + start, end - start);
            break;
        }

        case get_matrix_to_num_id: {
            const uint16_t page = (command_data[1] << 8) | command_data[0];
            const uint16_t start = page * RAW_HID_SIZE;
            if(start >= sizeof(matrix_to_num)) return;
            uint16_t end = start + RAW_HID_SIZE;
            if(end > sizeof(matrix_to_num)) end = sizeof(matrix_to_num);

            memcpy(data, ((uint8_t*)&matrix_to_num) + start, end - start);
            break;
        }

        //TODO: Can definitely make this more efficient
        //TODO: Occasionally the switch value isn't updated on the left, when the right half is master
        // When a key is selected in the GUI, this transaction will transfer the current height of that switch
        case get_switch_value_id: {
            index = command_data[0];
            const uint8_t prev_index = via_scan_index;
            via_scan_index = index;
            // Convert the global index to the local master index
            if(via_scan_index != 255) via_scan_index -= switch_low;

            if(prev_index != via_scan_index) via_scan_value = 0;

            // Is the index on the master half?
            if((index < switch_num + switch_low) && index >= switch_low) {
                // If the new index is on the master but the previous index was on the slave, stop updating the slave
                if(!((prev_index < switch_num + switch_low) && prev_index >= switch_low)){
                    if(prev_index != via_scan_index) {
                        index = 255;
                        split_id = split_switch_index;
                    }
                }
            }

            // If the new index is on the slave half, get the value from there first
            else if(!((index < switch_num + switch_low) && index >= switch_low)) {
                // If the index is new, send it to the slave
                if(prev_index != via_scan_index) {
                    split_id = split_switch_index;
                } else if(index != 255) {
                    via_scan_value = get_slave_value();
                }
            }

            command_data[0] = via_scan_value;
            command_data[1] = via_scan_value >> 8;
            break;
        }

        //TODO: I can probably remove this transaction, and all other RC stuff in HID
        // case get_mixed_matrix_id: {
            // #ifdef MIXED_MATRIX_ENABLE
            // const uint8_t matrix_to_rc_num[MATRIX_ROWS][MATRIX_COLS] = MATRIX_TO_RC_NUM;

            // const uint16_t page = (command_data[1] << 8) | command_data[0];
            // const uint16_t start = page * RAW_HID_SIZE;
            // if(start >= sizeof(matrix_to_rc_num)) return;
            // uint16_t end = start + RAW_HID_SIZE;
            // if(end > sizeof(matrix_to_rc_num)) end = sizeof(matrix_to_rc_num);

            // memcpy(&data, ((uint8_t*)&matrix_to_rc_num) + start, end - start);
            // #endif
        //     break;
        // }

        case set_switch_height_id: {
            index = command_data[0];
            if(index == 255) return;
            profile = command_data[1];
            if(profile > (am_keyboard_data.profile_num & 0x0F)) return;

            const height_addr_t height_type = command_data[2];
            split_id = height_type;
            value = (command_data[3] | (command_data[4] << 8));
            set_switch_height(index, profile, height_type, value);
            break;
        }

        case set_switch_mode_id: {
            index = command_data[0];
            if(index == 255) return;
            profile = command_data[1];
            if(profile > (am_keyboard_data.profile_num)) return;

            split_id = split_key_mode;
            value = command_data[2];
            set_switch_mode(index, profile, value);
            break;
        }

        case set_switch_priority_id: {
            #ifdef USE_PRIORITY_MODE
            index = command_data[0];
            if(index == 255) return;
            profile = command_data[1];
            if(profile > (am_keyboard_data.profile_num)) return;

            split_id = split_key_priority;
            value = command_data[2];
            set_switch_priority_mode(index, profile, value);
            #endif
            break;
        }

        case set_profile_layers_id: {
            split_id = split_profile_layers;
            profile = command_data[0];
            value = (layer_state_t)(command_data[1] | (command_data[2] << 8));
            // value = (layer_state_t)(command_data[1] | (command_data[2] << 8) | (command_data[3] << 16) | (command_data[4]) << 24);
            set_profile_layer_state(profile, value);
            break;
        }

        case set_priority_profiles_id: {
            #ifdef USE_PRIORITY_MODE
            split_id = split_priority_profiles;
            value = (command_data[0] | command_data[1] << 8);
            set_priority_profiles(value);
            #endif
            break;
        }

        case set_dynamic_calibration_id: {
            #ifdef DYNAMIC_CALIBRATION
            split_id = split_dynamic_calibration;
            index = command_data[0];
            value = command_data[1];
            set_dynamic_calibration(index, value);
            #endif
            break;
        }

        case set_deadzone_id: {
            split_id = split_deadzone;
            index = command_data[0];
            value = command_data[1];
            set_deadzones(index, value);
            break;
        }

        case set_profile_lock_save_id: {
            // Since it shares space with profile_num, we can save on a transaction
            split_id = split_profile_num;
            value = command_data[0];
            set_profile_lock_save_state(value);
            value = am_keyboard_data.profile_num;
            break;
        }

        case set_profile_num_id: {
            split_id = split_profile_num;
            value = command_data[0];
            set_profile_num(value);
            value = am_keyboard_data.profile_num;
            break;
        }

        //TODO: Could maybe consolidate this with the profile num transaction
        case set_profile_config_id: {
            split_id = split_profile_config;
            value = command_data[0];
            set_profile_config(value);
            break;
        }

        case set_active_profile_id: {
            set_active_profile(command_data[0]);
            break;
        }

        case save_config_id: {
            split_id = split_save_config;
            nvm_set_analog_matrix_config(&am_keyboard_data);
            config_update_required = false;
            break;
        }

        case clear_calibration_data_id: {
            #ifdef SPLIT_KEYBOARD
            am_data_manual_transaction(clear_calibration_values);
            #endif
            clear_calibration();
            break;
        }

        case reset_keyboard_data_id: {
            split_id = split_reset_keyboard;
            apply_default_config(&am_keyboard_data);

            // Re-initialize all keys
            get_key_config();
            get_calibration_data();
            for(uint8_t idx = 0; idx < switch_num; idx++) translate_mm_to_value(idx, true);
            profile_state_changed(active_profile);
            #if defined SPLIT_LAYER_SYNC
            change_layer_settings(am_highest_layer);
            #endif

            config_update_required = true;
            break;
        }
    }
    // Split keyboards need to send the result of all set_* transactions to the slave as well
    #ifdef SPLIT_KEYBOARD
    if(split_id == split_no_transfer) return;

    am_via_manual_transaction(index, profile, split_id, value);
    #endif
}
