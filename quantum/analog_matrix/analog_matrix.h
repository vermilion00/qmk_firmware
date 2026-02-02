#pragma once

// #include "analog_joystick.h"
#include "matrix.h"
#include <stdint.h>
// #include <stdbool.h>
// #include <sys/cdefs.h>
#include "action_layer.h"
// #include "gpio.h"
// #include "he_matrix.h"
#include "analog_matrix.h"
#include "eeconfig.h"
#include "info_config.h"
#include "analog.h"
#include "util.h"
// #include "bootloader.h"
// #include "bootmagic/bootmagic.h"
#ifdef JOYSTICK_ENABLE
// #   include "joystick_aliases.h"
#endif

//TODO: Test if this works to add onto SPLIT_TRANSACTION_IDS_KB
// #ifdef SPLIT_TRANSACTION_IDS_KB
// #define KB_TRANSACTIONS SPLIT_TRANSACTION_IDS_KB
// #undef SPLIT_TRANSACTION_IDS_KB
// #define SPLIT_TRANSACTION_IDS_KB KB_TRANSACTIONS, AM_PROFILE_SYNC, AM_CALIBRATION_SYNC
// #else
// #define SPLIT_TRANSACTION_IDS_KB AM_PROFILE_SYNC, AM_CALIBRATION_SYNC
// #endif

#define NONE 0
#define RAPID_TRIGGER 1
#define CONTINUOUS_RAPID_TRIGGER 2
#define CONSTANT_RAPID_TRIGGER 3

#define RIGHT 0
#define LEFT 1
#define UNKNOWN 2

// Override KEYBOARD_SIDE if -s flag is used
#if defined SIDE_LEFT
#undef KEYBOARD_SIDE
#define KEYBOARD_SIDE LEFT

#elif defined SIDE_RIGHT
#undef KEYBOARD_SIDE
#define KEYBOARD_SIDE RIGHT
#endif

#ifndef KEYBOARD_SIDE
#   define KEYBOARD_SIDE UNKNOWN
#endif

// This is set up so that the normal definitions can be used, only need to assign if the side
// is set at init
#if !defined SPLIT_KEYBOARD || KEYBOARD_SIDE == LEFT
#define SPLIT_MUTABLE const

#elif KEYBOARD_SIDE == RIGHT
#define SPLIT_MUTABLE const

#undef ADC_PINS
#define ADC_PINS ADC_PINS_R
#ifdef MUX_PINS
#undef MUX_PINS
#define MUX_PINS MUX_PINS_R
#endif
#undef MUX_CHANNELS
#define MUX_CHANNELS MUX_CHANNELS_R
#ifdef POWER_PINS
#undef POWER_PINS
#define POWER_PINS POWER_PINS_R
#endif
#ifdef MATRIX_POWER_PIN_R
#undef MATRIX_POWER_PIN
#define MATRIX_POWER_PIN MATRIX_POWER_PIN_R
#endif

// Save the left switch num for the calibration sync
#define SWITCH_NUM_L SWITCH_NUM
#undef SWITCH_NUM
#define SWITCH_NUM SWITCH_NUM_R
#undef MUX_TO_NUM
#define MUX_TO_NUM MUX_TO_NUM_R
#undef NUM_TO_MATRIX
#define NUM_TO_MATRIX NUM_TO_MATRIX_R
#undef MATRIX_TO_NUM
#define MATRIX_TO_NUM MATRIX_TO_NUM_R
#undef NUM_TO_MUX
#define NUM_TO_MUX NUM_TO_MUX_R

#if AM_INIT_KEY_NUM_R > 0
#undef AM_INIT_KEY_NUM
#define AM_INIT_KEY_NUM AM_INIT_KEY_NUM_R
#undef AM_INIT_KEYS
#define AM_INIT_KEYS AM_INIT_KEYS_R
#undef AM_INIT_FUNCTIONS
#define AM_INIT_FUNCTIONS AM_INIT_FUNCTIONS_R
#endif

#undef TRIGGER_HEIGHT
#define TRIGGER_HEIGHT TRIGGER_HEIGHT_R
#undef RELEASE_HEIGHT
#define RELEASE_HEIGHT RELEASE_HEIGHT_R
#undef RT_PRESS_DISTANCE
#define RT_PRESS_DISTANCE RT_PRESS_DISTANCE_R
#undef RT_RELEASE_DISTANCE
#define RT_RELEASE_DISTANCE RT_RELEASE_DISTANCE_R

#undef KEY_MODES
#define KEY_MODES KEY_MODES_R
#ifdef AM_TOP_VALUES_R
#undef AM_TOP_VALUES
#define AM_TOP_VALUES AM_TOP_VALUES_R
#endif
#ifdef AM_BOTTOM_VALUES_R
#undef AM_BOTTOM_VALUES
#define AM_BOTTOM_VALUES AM_BOTTOM_VALUES_R
#endif

#ifdef DEBUG_MUX_VALUE_R
#undef DEBUG_MUX_VALUE
#define DEBUG_MUX_VALUE DEBUG_MUX_VALUE_R
#endif

//TODO: Remove one of the following when I've decided
#ifdef PRIORITY_INDICES
#undef PRIORITY_INDICES
#define PRIORITY_INDICES PRIORITY_INDICES_R
#undef PRIORITY_INDEX_NUM
#define PRIORITY_INDEX_NUM PRIORITY_INDEX_NUM_R
#endif

#ifdef PRIORITY_MUXES
#undef PRIORITY_MUXES
#define PRIORITY_MUXES PRIORITY_MUXES_R
#undef PRIORITY_MUX_NUM
#define PRIORITY_MUX_NUM PRIORITY_MUX_NUM_R
#endif

#else // SPLIT_KEYBOARD defined but side is unknown at init
//TODO: Change the arrays to be const always, only the pointer that is used should be mutable
//      If I only need the pointer when side is unknown, then remove this
#define SPLIT_MUTABLE
#endif

#ifndef SWITCH_NUM_R
#   define SWITCH_NUM_R 0
#endif
#ifndef SWITCH_NUM_L
#   define SWITCH_NUM_L 0
#endif

#define DEFAULT_PROFILE 0
#define LAST_PROFILE 1

//TODO: Remove this before upload
#define LED_ON \
gpio_set_pin_output_push_pull(B2); \
gpio_write_pin_high(B2)

#define LED_OFF gpio_write_pin_low(B2)

typedef enum _key_mode_t: uint8_t {
    none = 0,
    rapid_trigger = 1,
    continuous_rapid_trigger = 2,
    constant_rapid_trigger = 3,
    joystick = 4
} key_mode_t;

//TODO: Decide what I'll do with this
//      Currently not very useful, might be nice to have if I allow associating a color with a profile
typedef struct Profile {
    layer_state_t layers;
} Profile;

typedef struct analog_key_t {
    uint8_t pressed;
    // 0 = None, 1-3 = RT, 4-9 = Reserved, 10-255 = Special keys (Gamepad etc)
    uint8_t mode[AM_PROFILE_NUM];
    #if defined USE_CONTINUOUS_RAPID_TRIGGER
    uint8_t rt_active;
    #endif
    #if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || (defined JOYSTICK_ENABLE && defined USE_JOYSTICK)
    // The switch is counted as pressed below this value
    uint16_t trigger_value[AM_PROFILE_NUM];
    // The switch is counted as released above this value
    uint16_t release_value[AM_PROFILE_NUM];
    #endif
    //TODO: Test if it's worth to have fields for the travel diff, or just calculate it every time
    #if defined JOYSTICK_ENABLE && !defined USE_JOYSTICK
    uint16_t joystick_travel;
    uint8_t joystick_value;
    //TODO: Add a field for the axis index
    int8_t axis_index;
    #endif
    #if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
    // The switch is counted as pressed, when the rapid trigger crosses this threshold
    // As the switch is traveling downward, this value is constantly updated, so the release distance is simply checked against this value to determine if the switch should be released
    uint16_t rt_threshold;
    // The distance that the switch is required to travel downwards before it's registered as pressed
    uint16_t rt_press_value[AM_PROFILE_NUM];
    // The distance that the switch is required to travel upwards before it's registered as released
    uint16_t rt_release_value[AM_PROFILE_NUM];
    #endif
    uint16_t scan_value;
    uint16_t bottom_value;
    uint16_t top_value;
    uint8_t row;
    uint8_t col;
} analog_key_t;

#ifndef AM_NO_EEPROM
typedef union {
    uint32_t raw;
    struct {
        uint16_t top_value;
        uint16_t bottom_value;
    };
} switch_data_t;
#endif

/* Configuration defaults */
#ifdef ADC_RESOLUTION
#   warning "ADC_RESOLUTION will be forced to 10 bits for the analog matrix!"
#undef ADC_RESOLUTION
#endif
#define ADC_RESOLUTION 10
#define MAX_ADC_VALUE 1023

#ifndef ADC_TOP_DEADZONE
#   define ADC_TOP_DEADZONE ADC_DEADZONE
#endif
#ifndef ADC_BOTTOM_DEADZONE
#   define ADC_BOTTOM_DEADZONE ADC_DEADZONE
#endif
#ifndef SCANS_WITHOUT_CHANGE
#   define SCANS_WITHOUT_CHANGE 20000
#endif
#ifdef ADC_SCAN_DELAY
#   define ADC_SCAN_CYCLES ADC_SCAN_DELAY
#endif
#ifdef MUX_SELECT_DELAY
#   define MUX_SELECT_CYCLES MUX_SELECT_DELAY
#endif
#ifdef POWER_SELECT_DELAY
#   define POWER_SELECT_CYCLES POWER_SELECT_DELAY
#endif
#if DYNAMIC_CALIBRATION == TRUE
// Absolute distance in adc counts
#ifndef AM_DC_DELTA
#   define AM_DC_DELTA 6
#endif
// Needs to be less than one
#ifndef AM_DC_FACTOR
#   define AM_DC_FACTOR 0.8
#endif
#endif

/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
extern bool manual_profile_lock;
#if AM_PROFILE_NUM > 1
#define PROFILE_MUTABLE
#else
#define PROFILE_MUTABLE const
#endif // if AM_PROFILE_NUM > 1

extern analog_key_t key_config[];
extern SPLIT_MUTABLE uint8_t switch_num;
extern PROFILE_MUTABLE uint8_t active_profile;
extern uint8_t highest_layer;
volatile static const Profile profiles[AM_PROFILE_NUM] = AM_PROFILE_CONFIG;

/* Function defines */
void analog_matrix_init(void);
uint8_t analog_matrix_scan(void);
void set_active_profile(uint8_t profile);
uint8_t get_active_profile(void);
void lock_profile(void);
void calibrate_switches(void);

volatile void sensor_power_init_kb(void);
volatile void sensor_power_init_user(void);
volatile void set_sensor_power_high_kb(uint8_t mux_channel);
volatile void set_sensor_power_high_user(uint8_t mux_channel);
volatile void set_sensor_power_low_kb(uint8_t mux_channel);
volatile void set_sensor_power_low_user(uint8_t mux_channel);
volatile void sensor_power_toggle_kb(uint8_t mux_channel, uint8_t adc_channel);
volatile void sensor_power_toggle_user(uint8_t mux_channel, uint8_t adc_channel);

//TODO: Implement a way to add this to default_keyboard.h if the heights aren't in the json
//This ain't gonna work since all profiles are currently in one 2D array, split by half
// #if !defined TRIGGER_HEIGHT || !defined TRIGGER_HEIGHT_R
/* #define TRIGGER_HEIGHT_LAYOUT(k0A, k0B, k0C, k0D, k0E, k0F, k6A, k6B, k6C, k6D, k6E, k6F, k1A, k1B, k1C, k1D, k1E, k1F, k7A, k7B, k7C, k7D, k7E, k7F, k2A, k2B, k2C, k2D, k2E, k2F, k8A, k8B, k8C, k8D, k8E, k8F, k3A, k3B, k3C, k3D, k3E, k3F, k9A, k9B, k9C, k9D, k9E, k9F, k4C, k4D, kAC, kAD, k5D, k4E, k4F, kBA, kAA, kAB, k5E, k5F, kBB) \
// const float trigger_height[] = {  \
//     k0A, k0B, k0C, k0D, k0E, k0F, \
//     k1A, k1B, k1C, k1D, k1E, k1F, \
//     k2A, k2B, k2C, k2D, k2E, k2F, \
//     k3A, k3B, k3C, k3D, k3E, k3F, \
//     XXX, XXX, k4C, k4D, k4E, k4F, \
//     XXX, XXX, XXX, k5D, k5E, k5F  \
// }; \
// const float trigger_height[] = {  \
//     k6A, k6B, k6C, k6D, k6E, k6F, \
//     k7A, k7B, k7C, k7D, k7E, k7F, \
//     k8A, k8B, k8C, k8D, k8E, k8F, \
//     k9A, k9B, k9C, k9D, k9E, k9F, \
//     kAA, kAB, kAC, kAD, XXX, XXX, \
//     kBA, kBB, XXX, XXX, XXX, XXX  \
// };
#endif */
