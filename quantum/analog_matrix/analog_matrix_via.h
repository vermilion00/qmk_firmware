#pragma once

#include <stdint.h>
#include "action_layer.h"
#include "via_bindings.h"

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

#define CLAMP8(value) (value > 255 ? 255 : value)

// The EEPROM data of one switch in one single profile
// typedef struct PACKED am_switch_profile_t {
//     uint8_t mode; // MSB contains priority_mode
//     union {
//         am_raw_t raw_height;
//         struct {
//             #ifdef USE_TRIGGER_HEIGHT
//             height_t trigger_height;
//             height_t release_height;
//             #endif
//             #ifdef USE_RT_DISTANCE
//             height_t rt_press_distance;
//             height_t rt_release_distance;
//             #endif
//         };
//     };
// } am_switch_profile_t;

// The EEPROM data of one switch across all profiles
// typedef struct PACKED am_switch_t {
//     am_switch_profile_t profile[AM_PROFILE_NUM];
//     // uint8_t priority_state[(TOTAL_SWITCH_NUM / 8) + 1]; // Each bit represents a switch
// } am_switch_t;

// The EEPROM data of the entire keyboard
// typedef struct PACKED am_keyboard_t {
//     am_switch_t key[TOTAL_SWITCH_NUM];
//     uint8_t smoothing; // If split multipliers are used, this is the non-multiplied value
//     uint8_t top_deadzone;
//     uint8_t bottom_deadzone;
//     uint8_t top_mult;
//     #ifdef SPLIT_KEYBOARD
//     uint8_t right_mult;
//     uint8_t slave_mult;
//     #endif
//     #if PROFILE_SWITCH_MODE != MANUAL_PROFILE
//     layer_state_t profile_layers[AM_PROFILE_NUM]; // Stores the assigned layers of each profile as a bitmap
//     #endif
//     #ifdef USE_PRIORITY_MODE
//     uint16_t priority_profiles; // Stores the priority status of each profile as a bitmap
//     #endif
//     #ifdef DYNAMIC_CALIBRATION
//     uint8_t dc_switch_num;
//     uint8_t dc_factor;
//     uint8_t dc_delta;
//     #endif
// } am_keyboard_t;

typedef struct am_keyboard_t {
    #ifdef USE_TRIGGER_HEIGHT
    height_t trigger_height[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
    height_t release_height[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
    #endif
    #ifdef USE_RT_DISTANCE
    height_t rt_press_distance[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
    height_t rt_release_distance[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
    #endif
    uint8_t key_mode[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
    uint8_t profile_num;
    uint8_t profile_config;
    layer_state_t profile_layers[AM_PROFILE_NUM]; // Stores the assigned layers of each profile as a bitmap

    // The deadzone values are only the USER_DEADZONES, not the ADC_DEADZONES
    uint8_t top_deadzone; // Base values pre multiplication
    uint8_t bottom_deadzone;
    uint8_t smoothing;
    uint8_t top_mult; // Multiplied by 100
    #ifdef VIA_FILTER_STRENGTH
    uint8_t filter_strength;
    #endif

    #ifdef USE_PRIORITY_MODE
    uint16_t priority_profiles; // Stores the priority status of each profile as a bitmap
    #endif
    #ifdef DYNAMIC_CALIBRATION
    uint8_t dc_switch_num;
    uint8_t dc_factor; // Multiplied by 100
    uint8_t dc_delta;
    #endif

    #ifdef SPLIT_KEYBOARD
    #ifdef VIA_FILTER_STRENGTH
    uint8_t split_filter_strength; // The first 4 bits show the slave strength, the right 4 the right strength
    #endif
    uint8_t right_mult; // Multiplied by 10
    uint8_t slave_mult; // Multiplied by 10
    #endif // ifdef SPLIT_KEYBOARD
    uint16_t keyboard_size;
} am_keyboard_t;

extern am_keyboard_t am_keyboard_data;

typedef enum height_addr_t {
    trigger_height_addr = 0,
    release_height_addr = 1,
    rt_press_addr = 2,
    rt_release_addr = 3
} height_addr_t;

typedef enum am_via_id {
    // Default data ends at 0x13, 0xFE and 0xFF are also taken
    get_keyboard_def_id = 0x20, // Get the the size of the keyboard data and enabled features
    get_keyboard_data_id, // Get the entirety of am_keyboard_data
    get_mixed_matrix_id, // Get the matrix positions of all mixed matrix keys
    get_switch_profile_id, // Probably unused
    set_switch_height_id, // Set any height for the index
    set_switch_mode_id,
    set_switch_priority_id,
    set_profile_layers_id,
    set_priority_profiles_id,
    set_dynamic_calibration_id,
    set_deadzone_id, // Set one of top_deadzone, bottom_deadzone, smoothing, or top mult value, according to the index passed
    clear_calibration_data_id, // Resets the calibration data
    reset_keyboard_data_id, // Resets all data to json defaults
} am_via_id;

//TODO: Remove this and just use the am_via_id instead
//      I'd have to use a default case to catch unused IDs, but probably still cleaner anyways
//      Would also allow me to just call a general transaction at the end of the HID handler, instead of the current impl
typedef enum am_via_split_id {
    split_trigger_height,
    split_release_height,
    split_rt_press,
    split_rt_release,
    split_key_mode,
    split_profile_layers,
    split_priority_profiles,
    split_deadzone,
    split_keyboard_data,
    split_reset_keyboard
} am_via_split_id;

typedef struct _am_via_data_t {
    uint8_t index;
    am_via_split_id id;
    layer_state_t value;
} am_via_data_t;

extern am_keyboard_t am_keyboard_data;

#ifndef MATRIX_TO_NUM_DEF
extern SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS_PER_HAND][MATRIX_COLS];
// #   define MATRIX_TO_NUM_DEF
#endif

height_t get_switch_height(uint8_t key, uint8_t profile, height_addr_t height);
uint8_t get_switch_mode(uint8_t key, uint8_t profile);
bool get_switch_priority_mode(uint8_t key, uint8_t profile);
void set_switch_height(uint8_t key, uint8_t profile, height_addr_t height, height_t value);
void set_switch_mode(uint8_t key, uint8_t profile, key_mode_t mode);
void set_switch_priority_mode(uint8_t key, uint8_t profile, bool priority);
void set_priority_profiles(uint16_t value);
void set_profile_layer_state(uint8_t profile, layer_state_t value);
void set_deadzones(uint8_t index, uint16_t value);
void apply_default_config(am_keyboard_t* keyboard_data);

void analog_matrix_via_init(void);
// VIA(L) app HID command handling
void analog_matrix_handle_hid(uint8_t *data, uint8_t length);
