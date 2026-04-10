#pragma once

#include "analog_matrix.h"

#ifdef HIGH_HEIGHT_RESOLUTION
#if defined USE_TRIGGER_HEIGHT && defined USE_RT_DISTANCE
typedef uint64_t am_raw_t;
#else
typedef uint32_t am_raw_t;
#endif
#else
#if defined USE_TRIGGER_HEIGHT && defined USE_RT_DISTANCE
typedef uint32_t am_raw_t;
#else
typedef uint16_t am_raw_t;
#endif
#endif

#define RAW_HID_SIZE 32

// The EEPROM data of one switch in one single profile
typedef struct PACKED am_switch_profile_t {
    uint8_t mode; // MSB contains priority_mode
    union {
        am_raw_t raw;
        struct {
            #ifdef USE_TRIGGER_HEIGHT
            height_t trigger_height;
            height_t release_height;
            #endif
            #ifdef USE_RT_DISTANCE
            height_t rt_press_distance;
            height_t rt_release_distance;
            #endif
        };
    };
} am_switch_profile_t;

// The EEPROM data of one switch across all profiles
typedef struct PACKED am_switch_t {
    am_switch_profile_t profile[AM_PROFILE_NUM];
    // uint8_t priority_state[(TOTAL_SWITCH_NUM / 8) + 1]; // Each bit represents a switch
} am_switch_t;

// The EEPROM data of the entire keyboard
typedef struct PACKED am_keyboard_t {
    am_switch_t key[TOTAL_SWITCH_NUM];
    #if PROFILE_SWITCH_MODE != MANUAL_PROFILE
    layer_state_t profile_layers[AM_PROFILE_NUM]; // Stores the assigned layers of each profile as a bitmap
    #endif
    #ifdef USE_PRIORITY_MODE
    uint16_t priority_profiles; // Stores the priority status of each profile as a bitmap
    #endif
} am_keyboard_t;

typedef enum height_addr_t {
    trigger_height_addr = 0,
    release_height_addr = 1,
    rt_press_addr = 2,
    rt_release_addr = 3
} height_addr_t;

typedef enum am_vial_id {
    // Default data ends at 0x13, 0xFE and 0xFF are also taken
    get_keyboard_size = 0x20, // Get the length of the keyboard data
    get_keyboard_data,
    get_switch_profile,
    set_switch_profile,
    set_profile_layers,
    clear_calibration_data,
    reset_keyboard_data, // Resets all data to json defaults
} am_vial_id;

typedef enum am_vial_split_id {
    set_trigger_height,
    set_release_height,
    set_rt_press,
    set_rt_release,
    set_key_mode,
    profile_layers,
    priority_profiles,
} am_vial_split_id;

extern am_keyboard_t am_keyboard_data;

height_t get_switch_height(uint8_t key, uint8_t profile, height_addr_t height);
uint8_t get_switch_mode(uint8_t key, uint8_t profile);
bool get_switch_priority_mode(uint8_t key, uint8_t profile);
void set_switch_height(uint8_t key, uint8_t profile, height_addr_t height, height_t value);
void set_switch_mode(uint8_t key, uint8_t profile, key_mode_t mode);
void set_switch_priority_mode(uint8_t key, uint8_t profile, bool priority);
// VIA(L) app HID command handling
void analog_matrix_handle_hid(uint8_t *data, uint8_t length);
