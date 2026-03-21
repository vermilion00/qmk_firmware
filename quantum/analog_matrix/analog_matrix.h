#pragma once

#include "matrix.h"
#include <stdint.h>
#include "action_layer.h"
#include "analog_matrix.h"
#include "eeconfig.h"
#include "info_config.h"
#include "analog.h"
#include "util.h"
#include "multiplexer.h"

#define SMAX(var) MAX(var, var##_R)

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

#ifdef RC_INIT_KEYS_R
#undef RC_INIT_KEYS
#define RC_INIT_KEYS RC_INIT_KEYS_R
#undef RC_INIT_FUNCTIONS
#define RC_INIT_FUNCTIONS_R
#undef RC_INIT_KEY_NUM
#undef RC_INIT_KEY_NUM RC_INIT_KEY_NUM_R
#endif

#else // SPLIT_KEYBOARD defined but side is unknown at init
#define SPLIT_MUTABLE
#define CONFIG_MUTABLE
#endif // ifdef SPLIT_KEYBOARD && KEYBOARD_SIDE != UNKNOWN else

#ifndef RC_INIT_KEY_NUM_R
#define RC_INIT_KEY_NUM_R 0
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

// User-definable option to add extra parameters
#ifndef AM_USER_PARAMS
#   define AM_USER_PARAMS
#endif

#if (COL_PIN_NUM <= 8)
typedef uint8_t mech_row_t;
#elif (COL_PIN_NUM <= 16)
typedef uint16_t mech_row_t;
#elif (COL_PIN_NUM <= 32)
typedef uint32_t mech_row_t;
#else
#    error "COL_PIN_NUM: invalid value"
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
    //TODO: When the priority mode define is fully implemented, update this
    #if defined PRIORITY_INDICES || defined SLAVE_LOW_PRIORITY || defined USE_PRIORITY_MODE
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
    #ifdef ADJUST_TRAVEL
    uint16_t travel_mult;
    #endif
    uint8_t row;
    uint8_t col;
    // Can be defined in config.h to add extra parameters
    AM_USER_PARAMS
} analog_key_t;

#if defined MCU_STM32
typedef stm32_gpio_t gpio_port_t;
#elif defined MCU_AT32
typedef at32_gpio_t gpio_port_t;
//TODO: How do I do this for RP2040?
#endif

/* Configuration defaults */
#if defined ADC_RESOLUTION && ADC_RESOLUTION < 12
#   warning "ADC_RESOLUTION will be forced to 12 bits for the analog matrix!"
#undef ADC_RESOLUTION
#define ADC_RESOLUTION 12
#endif
#define MAX_ADC_VALUE 4095

#ifndef ADC_DEADZONE
#   define ADC_DEADZONE 100
#endif
#ifndef ADC_SMOOTHING
#   define ADC_SMOOTHING 60
#endif
#ifndef ADC_TOP_DEADZONE
#   define ADC_TOP_DEADZONE ADC_DEADZONE
#endif
#ifndef ADC_BOTTOM_DEADZONE
#   define ADC_BOTTOM_DEADZONE ADC_DEADZONE
#endif
#ifndef SCANS_WITHOUT_CHANGE
#   define SCANS_WITHOUT_CHANGE 5000
#endif
#ifndef AM_STARTUP_DELAY
#   define AM_STARTUP_DELAY 20 // In ms
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
//TODO: RM this once it's set by the script
//      Also use the same indexes and funcions for both halves if only one is set
//      -> I need to make sure that the resulting mux position exists on the other half, or else it will always count as triggered
#ifndef AM_INIT_KEY_NUM_R
#define AM_INIT_KEY_NUM_R AM_INIT_KEY_NUM
#endif

#ifndef CAL_THRESHOLD
#   define CAL_THRESHOLD 7
#endif
#ifndef INIT_THRESHOLD
#   define INIT_THRESHOLD 4
#endif

/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
extern bool manual_profile_lock;
#if AM_PROFILE_NUM > 1
#define PROFILE_MUTABLE
#else
#define PROFILE_MUTABLE const
#endif // if AM_PROFILE_NUM > 1

// Guarded to allow overwriting the filter with a different implementation
#ifndef ADC_FILTER
// This filter seems more efficient than value -= (value - key_config[index].scan_value) >> 1
#if defined EXTREME_ADC_FILTER  // 1/4 new, 3/4 old
#   define ADC_FILTER(value, index) value = (value >> 2) + ((key_config[index].scan_value * 3) >> 2)
#elif defined STRONG_ADC_FILTER // 1/2 new, 1/2 old
#   define ADC_FILTER(value, index) value = (value >> 1) + (key_config[index].scan_value >> 1)
#elif defined WEAK_ADC_FILTER   // 7/8 new, 1/8 old
#   define ADC_FILTER(value, index) value = ((value * 7) >> 3) + (key_config[index].scan_value >> 3)
#elif defined NO_ADC_FILTER
#   define ADC_FILTER(value, index)
#else // Medium filter             3/4 new, 1/4 old
#   define ADC_FILTER(value, index) value = ((value * 3) >> 2) + (key_config[index].scan_value >> 2)
#endif
// #define ADC_FILTER(value, index) value -= (value - key_config[index].scan_value) >> 2
#endif

#ifdef DYNAMIC_CALIBRATION
// Keeps track of how many switches need updating, and saves new data once it exceeds RECALIBRATED_SWITCHES, to avoid writing to storage too often
extern uint8_t recalibrated_switches;
// Check if the switch boundaries need updating, and update them if necessary.
bool update_switch_bounds(uint8_t index, uint16_t value);
#endif

volatile static const Profile profiles[AM_PROFILE_NUM] = AM_PROFILE_CONFIG;
extern matrix_row_t matrix[MATRIX_ROWS];
extern analog_key_t key_config[];
extern SPLIT_MUTABLE uint8_t switch_num;
extern PROFILE_MUTABLE uint8_t active_profile;
extern SPLIT_MUTABLE pin_t adc_pins[ADC_PIN_NUM];
extern adc_mux adc_pin_mux[ADC_PIN_NUM];
#if defined MUX_PINS || defined MUX_PINS_R
extern SPLIT_MUTABLE pin_t mux_pins[MUX_PIN_NUM];
#endif
#ifdef POWER_PINS
extern SPLIT_MUTABLE uint8_t power_pin_num = POWER_PIN_NUM;
// Set the sensor power pins and delay, if defined
void set_sensor_power(uint8_t index);
#endif
extern uint8_t highest_layer;
extern PROFILE_MUTABLE uint8_t active_profile;

#ifdef USE_PRIORITY_MODE
extern bool priority_mode;
#endif

#ifndef AM_NO_EEPROM
#include "nvm_eeconfig.h"
extern analog_switch_t calibration_data[SMAX(SWITCH_NUM)];
#endif

#ifdef ADJUSTMENT_FUNCTION
__attribute__((weak)) uint16_t adjust(uint16_t value);
#else
#   define adjust(value) value
#endif

#ifdef SPLIT_KEYBOARD
extern bool calibration_started;
#ifdef AM_NO_EEPROM
extern uint8_t switch_num_slave;
#endif
#endif

/* Global Functions */
// Readies the keyboard state before scanning begins
void analog_matrix_init(void);
// Scans the keys and calls the evaluation function on them
uint8_t analog_matrix_scan(void);
// Evaluates the value against the configured heights, to see if the switch state has changed. Returns true if changed.
bool evaluate_value(uint8_t index, uint16_t value);
// Populates the calibrated values from eeprom or hardcoded values, and applies deadzones
bool get_calibration_data(void);
// Get the switch data configured in the json
void get_switch_data(void);
// Scans the init keys defined in the info.json
void scan_init_keys(void);
// Translate the user defined heights and distances into the equivalent ADC values
void translate_mm_to_value(uint8_t index);
// Gets the previously calibrated min/max values for each switch from the EEPROM
bool get_calibration_data(void);
// Immediately starts calibration
void calibrate_switches(bool init);
// Assigns side and configuration at init
void assign_config(bool side);
// Runs whenever the profile has changed
void profile_state_changed(uint8_t profile);
// Activates the profile passed as the parameter (only on the master half on split keyboards)
void set_active_profile(uint8_t profile);
// Returns the active profile
uint8_t get_active_profile(void);
// Toggles the state of the automatic profile switching
void toggle_profile_lock(void);
// Sets the profile lock state to the passed value
void set_profile_lock(bool value);
// Run actions when the layer state changes
layer_state_t layer_state_set_am(layer_state_t state);
// Empty loop for short delays
#ifdef AM_USE_DELAY
void delay_ns(uint16_t delay);
#else
#define delay_ns(delay)
#endif
#ifdef CUSTOM_MATRIX_LITE
bool matrix_scan_custom(matrix_row_t current_matrix[]);
#endif
#ifdef USE_INIT_KEYS
// Init key helpers
void _bootmagic(bool init);
void _bootloader_jump(bool init);
#endif

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

