#pragma once

// #include "gpio.h"
#include "matrix.h"
#include <stdint.h>
#include "action_layer.h"
#include "analog_matrix.h"
#include "eeconfig.h"
#include "info_config.h"
#include "analog.h"
#include "util.h"
// #include "math.h"

#define NONE 0
#define RAPID_TRIGGER 1
#define CONTINUOUS_RAPID_TRIGGER 2
#define CONSTANT_RAPID_TRIGGER 3

#define RIGHT 0
#define LEFT 1
#define UNKNOWN 2

// Define if keymap config is used
#if (!defined TRIGGER_HEIGHT && (defined USE_TRIGGER_HEIGHT)) || \
    (!defined RT_PRESS_DISTANCE && (defined USE_RT_DISTANCE))
#ifndef KEYMAP_CONFIG
#   define KEYMAP_CONFIG
#endif
#endif

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

// This is set up so that the normal definitions can be used, only need to assign if the side is set at init
#if !defined SPLIT_KEYBOARD || KEYBOARD_SIDE == LEFT
#if !defined KEYMAP_CONFIG
#define CONFIG_MUTABLE const
#else
#define CONFIG_MUTABLE
#endif
#define SPLIT_MUTABLE const

#elif KEYBOARD_SIDE == RIGHT
#if !defined KEYMAP_CONFIG
#define CONFIG_MUTABLE const
#else
#define CONFIG_MUTABLE
#endif
#define SPLIT_MUTABLE const

#undef ADC_PINS
#define ADC_PINS ADC_PINS_R
#ifdef MUX_PINS
#undef MUX_PINS
#define MUX_PINS MUX_PINS_R
#endif
#undef MUX_CHANNELS
#define MUX_CHANNELS MUX_CHANNELS_R
#ifndef MUX_PINS_RIGHT_CONTINUOUS
#undef MUX_PINS_CONTINUOUS
#else
#undef MUX_PIN_OFFSET
#define MUX_PIN_OFFSET MUX_PIN_RIGHT_OFFSET
#undef CONTINUOUS_MUX_PORT
#define CONTINUOUS_MUX_PORT CONTINUOUS_MUX_PORT_RIGHT
#endif

#ifdef POWER_PINS
#undef POWER_PINS
#define POWER_PINS POWER_PINS_R
#endif
#ifdef MATRIX_POWER_PIN_R
#undef MATRIX_POWER_PIN
#define MATRIX_POWER_PIN MATRIX_POWER_PIN_R
#endif

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

#ifndef KEYMAP_CONFIG
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
#endif

#ifdef AM_TOP_VALUES_R
#undef AM_TOP_VALUES
#define AM_TOP_VALUES AM_TOP_VALUES_R
#endif
#ifdef AM_BOTTOM_VALUES_R
#undef AM_BOTTOM_VALUES
#define AM_BOTTOM_VALUES AM_BOTTOM_VALUES_R
#endif

#ifdef DEBUG_MUX_POSITION_R
#undef DEBUG_MUX_POSITION
#define DEBUG_MUX_POSITION DEBUG_MUX_POSITION_R
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
#define SPLIT_MUTABLE
#define CONFIG_MUTABLE
#endif

#define DEFAULT_PROFILE 0
#define LAST_PROFILE 1
#define MANUAL_PROFILE 2

#if defined PRIORITY_INDICES || defined SLAVE_LOW_PRIORITY
#ifndef PRIORITY_LEVEL
#define PRIORITY_LEVEL 5
#endif
#endif

//TODO: Remove this before upload
#ifdef LED_PIN
#   ifndef LED_INVERTED
#define LED_ON \
gpio_set_pin_output_push_pull(LED_PIN); \
gpio_write_pin_high(LED_PIN)

#define LED_OFF gpio_write_pin_low(LED_PIN)
#   else
#define LED_ON \
gpio_set_pin_output_push_pull(LED_PIN); \
gpio_write_pin_low(LED_PIN)

#define LED_OFF gpio_write_pin_high(LED_PIN)
#   endif
#else
#define LED_ON
#define LED_OFF
#endif

typedef enum _key_mode_t: uint8_t {
    none = 0,
    rapid_trigger = 1,
    continuous_rapid_trigger = 2,
    constant_rapid_trigger = 3,
    joystick = 4
} key_mode_t;

typedef struct Profile {
    layer_state_t layers;
    #if defined PRIORITY_INDICES || defined SLAVE_LOW_PRIORITY
    bool priority_profile;
    #endif
} Profile;

typedef struct analog_key_t {
    uint8_t pressed;
    // 0 = None, 1-3 = RT, 4-9 = Reserved, 10-255 = Special keys (Gamepad etc)
    uint8_t mode[AM_PROFILE_NUM];
    #if defined USE_CONTINUOUS_RAPID_TRIGGER
    uint8_t rt_active;
    #endif
    #if defined USE_TRIGGER_HEIGHT || (defined JOYSTICK_ENABLE && defined USE_JOYSTICK)
    // The switch is counted as pressed below this value
    uint16_t trigger_value[AM_PROFILE_NUM];
    // The switch is counted as released above this value
    uint16_t release_value[AM_PROFILE_NUM];
    #endif
    //TODO: Test if it's worth to have fields for the travel diff, or just calculate it every time
    #if defined JOYSTICK_ENABLE && !defined USE_JOYSTICK
    uint16_t joystick_travel;
    uint8_t joystick_value;
    int8_t axis_index;
    #endif
    #if defined MIDI_ENABLE && !defined USE_MIDI
    uint8_t midi_velocity;
    #endif
    #if defined USE_RT_DISTANCE
    // The switch is counted as pressed when the rapid trigger crosses this threshold
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

/* Configuration defaults */
#if defined ADC_RESOLUTION && ADC_RESOLUTION < 12
#   warning "ADC_RESOLUTION will be forced to 12 bits for the analog matrix!"
#undef ADC_RESOLUTION
#define ADC_RESOLUTION 12
#endif
#define MAX_ADC_VALUE 4095

#ifndef ADC_DEADZONE
#   define ADC_DEADZONE 120
#endif
#ifndef ADC_SMOOTHING
#   define ADC_SMOOTHING 24
#endif
#ifndef ADC_TOP_DEADZONE
#   define ADC_TOP_DEADZONE ADC_DEADZONE
#endif
#ifndef ADC_BOTTOM_DEADZONE
#   define ADC_BOTTOM_DEADZONE ADC_DEADZONE
#endif
#ifndef SCANS_WITHOUT_CHANGE
#   define SCANS_WITHOUT_CHANGE 6000
#endif
#ifndef AM_STARTUP_DELAY
#   define AM_STARTUP_DELAY 20000
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
#ifdef DYNAMIC_CALIBRATION
// Absolute distance in adc counts
#ifndef AM_DC_DELTA
#ifdef ADC_SMOOTHING
#   define AM_DC_DELTA ADC_SMOOTHING
#else
#   define AM_DC_DELTA 24
#endif
#endif
// Percentage of the range that will be moved up/down
#ifndef AM_DC_FACTOR
#   define AM_DC_FACTOR 0.15
#endif
#ifndef RECALIBRATED_SWITCHES
#   define RECALIBRATED_SWITCHES 5
#endif
#endif // ifdef DYNAMIC_CALIBRATION

/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
extern bool manual_profile_lock;
#if AM_PROFILE_NUM > 1
#define PROFILE_MUTABLE
#else
#define PROFILE_MUTABLE const
#endif // if AM_PROFILE_NUM > 1

//TODO: Why does it work without this using stm32, but not using rp2040?
extern SPLIT_MUTABLE pin_t mux_pins[MUX_PIN_NUM];

volatile static const Profile profiles[AM_PROFILE_NUM] = AM_PROFILE_CONFIG;
extern analog_key_t key_config[];
extern SPLIT_MUTABLE uint8_t switch_num;
extern PROFILE_MUTABLE uint8_t active_profile;
extern uint8_t highest_layer;

#ifdef ADJUSTMENT_FUNCTION
__attribute__((weak)) uint16_t adjust(uint16_t value);
#else
#   define adjust(value) value
#endif

#ifdef SPLIT_KEYBOARD
extern bool calibration_started;
extern uint8_t switch_num_slave;
#endif

/* Global Functions */
void analog_matrix_init(void);
uint8_t analog_matrix_scan(void);
// Activates the profile passed as the parameter (only on the master half on split keyboards)
void set_active_profile(uint8_t profile);
// Returns the active profile
uint8_t get_active_profile(void);
// Toggles the state of the automatic profile switching
void toggle_profile_lock(void);
// Sets the profile lock state to the passed value
void set_profile_lock(bool value);
// Immediately starts calibration
void calibrate_switches(bool init);
//TODO: Remove
void _sync_cal(void);

typedef void (* init_func_t)(bool init);

volatile void sensor_power_init_kb(void);
volatile void sensor_power_init_user(void);
volatile void set_sensor_power_high_kb(uint8_t mux_channel);
volatile void set_sensor_power_high_user(uint8_t mux_channel);
volatile void set_sensor_power_low_kb(uint8_t mux_channel);
volatile void set_sensor_power_low_user(uint8_t mux_channel);
volatile void sensor_power_toggle_kb(uint8_t mux_channel, uint8_t adc_channel);
volatile void sensor_power_toggle_user(uint8_t mux_channel, uint8_t adc_channel);

//TODO: Similar to the LAYOUT macro, write a script to define this macro
/*
 #define MATRIX(k0A, k0B, k0C, k0D, k0E, k0F, k6A, k6B, k6C, k6D, k6E, k6F, k1A, k1B, k1C, k1D, k1E, k1F, k7A, k7B, k7C, k7D, k7E, k7F, k2A, k2B, k2C, k2D, k2E, k2F, k8A, k8B, k8C, k8D, k8E, k8F, k3A, k3B, k3C, k3D, k3E, k3F, k9A, k9B, k9C, k9D, k9E, k9F, k4C, k4D, kAC, kAD, k5D, k4E, k4F, kBA, kAA, kAB, k5E, k5F, kBB) \
               {k0A, k0B, k0C, k0D, k0E, k0F, k1A, k1B, k1C, k1D, k1E, k1F, k2A, k2B, k2C, k2D, k2E, k2F, k3A, k3B, k3C, k3D, k3E, k3F, k4C, k4D, k5D, k4E, k4F, k5E, k5F, k6A, k6B, k6C, k6D, k6E, k6F, k7A, k7B, k7C, k7D, k7E, k7F, k8A, k8B, k8C, k8D, k8E, k8F, k9A, k9B, k9C, k9D, k9E, k9F, kAC, kAD, kBA, kAA, kAB, kBB}
*/

