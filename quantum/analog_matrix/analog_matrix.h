#pragma once

#include "matrix.h"
#include <stdint.h>
#include "action_layer.h"
#include "eeconfig.h"
#include "info_config.h"
#include "analog.h"
#include "util.h"
#include "multiplexer.h"

#ifdef HIGH_HEIGHT_RESOLUTION
typedef uint16_t height_t;
#   define HEIGHT_MULT 1000.0
#   define AM_HEIGHT_MASK 0b1111111111111111
#else
typedef uint8_t height_t;
#   define HEIGHT_MULT 50.0
#   define AM_HEIGHT_MASK 0b11111111
#endif

typedef enum key_mode_t {
    none = 0,
    rapid_trigger = 1,
    continuous_rapid_trigger = 2,
    constant_rapid_trigger = 3,
    joystick = 4,
    midi = 5
} key_mode_t;

#if (defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN)
#   define SPLIT_MUTABLE
#else
#   define SPLIT_MUTABLE const
#endif

#ifdef VIA_ENABLE
#   include "via_bindings.h"
#   include "analog_matrix_via.h"
#else
#   define VIA_MUTABLE const
#   ifdef SPLIT_KEYBOARD
#       define SPLIT_VIA_MUT
#   else
#       define SPLIT_VIA_MUT const
#   endif
#endif

typedef void (* init_func_t)(bool init);
typedef uint16_t (* adc_filter_t)(uint16_t value, uint8_t index);

#define SMAX(var) MAX(var, var##_R)

#define RIGHT 0
#define LEFT 1
#define UNKNOWN 2

#if defined SIDE_LEFT || defined SIDE_RIGHT
#include "side_define.h"
#endif

#define NONE 0
#define RAPID_TRIGGER 1
#define CONTINUOUS_RAPID_TRIGGER 2
#define CONSTANT_RAPID_TRIGGER 3

#ifndef KEYBOARD_SIDE
#   define KEYBOARD_SIDE UNKNOWN
#endif

#ifndef RC_INIT_KEY_NUM_R
#define RC_INIT_KEY_NUM_R 0
#endif

#define DEFAULT_PROFILE 0
#define LAST_PROFILE 1
#define MANUAL_PROFILE 2

#ifdef USE_PRIORITY_MODE
#ifndef PRIORITY_LEVEL
#define PRIORITY_LEVEL 5
#endif
#endif

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

// User-definable option to add extra parameters to the key struct
#ifndef AM_USER_PARAMS
#   define AM_USER_PARAMS
#endif

//MARK: Structs
typedef enum transaction_type_t {
    normal_transaction = 0,
    calibration_started,
    top_calibration_started,
    clear_calibration_values
} transaction_type_t;

typedef struct Profile {
    layer_state_t layers;
    #ifdef USE_PRIORITY_MODE
    bool priority_profile;
    #endif
} Profile;

// If VIA isn't enabled, we only include these fields in the struct
#ifndef VIA_ENABLE
typedef struct am_keyboard_t {
    #ifdef USE_TRIGGER_HEIGHT
    SPLIT_VIA_MUT height_t trigger_height[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
    SPLIT_VIA_MUT height_t release_height[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
    #endif
    #ifdef USE_RT_DISTANCE
    SPLIT_VIA_MUT height_t rt_press_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
    SPLIT_VIA_MUT height_t rt_release_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
    #endif
    SPLIT_VIA_MUT uint8_t key_mode[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
    uint8_t profile_num; // Only needed for the profile_lock state
    const uint8_t profile_config; // 4 MSB = default_profile, 4 LSB = switch mode
    layer_state_t profile_layers[AM_PROFILE_NUM];

    #ifdef PRIORITY_PROFILES
    const uint16_t priority_profiles;
    #endif

    #ifdef DYNAMIC_CALIBRATION
    const uint8_t dc_switch_num;
    const uint8_t dc_factor;
    const uint8_t dc_delta;
    #endif
    const uint8_t top_deadzone;
    const uint8_t bottom_deadzone;
    const uint8_t smoothing;
    const uint8_t top_mult;
    #ifdef SPLIT_KEYBOARD
    const uint8_t right_mult;
    const uint8_t slave_mult;
    #endif
} am_keyboard_t;

extern SPLIT_VIA_MUT am_keyboard_t am_keyboard_data;
#endif // ifdef VIA_ENABLE

//MARK: Key struct
typedef struct analog_key_t {
    uint8_t pressed;
    // 0 = None, 1-3 = RT, 4-9 = Reserved, 10-255 = Special keys (Gamepad etc)
    uint8_t mode[AM_PROFILE_NUM];
    #if defined USE_CONTINUOUS_RAPID_TRIGGER
    uint8_t rt_active;
    #endif
    #if defined USE_TRIGGER_HEIGHT
    // The switch is counted as pressed below this value
    uint16_t trigger_value[AM_PROFILE_NUM];
    // The switch is counted as released above this value
    uint16_t release_value[AM_PROFILE_NUM];
    #endif
    //TODO: Test if it's worth to have fields for the travel diff, or just calculate it every time
    #if defined JOYSTICK_ENABLE
    uint16_t joystick_travel;
    uint8_t joystick_value;
    uint8_t axis_index;
    #endif
    #if defined MIDI_ENABLE
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

//MARK: Defaults
/* Configuration defaults */
#if defined ADC_RESOLUTION && ADC_RESOLUTION < 12
#   warning "ADC_RESOLUTION will be forced to 12 bits for the analog matrix!"
#undef ADC_RESOLUTION
#define ADC_RESOLUTION 12
#endif
#define MAX_ADC_VALUE 4095

#ifndef ADC_SMOOTHING
#   define ADC_SMOOTHING 30
#endif
#ifndef ADC_DEADZONE
#   define ADC_DEADZONE 100
#endif
#ifndef ADC_TOP_DEADZONE
#   define ADC_TOP_DEADZONE ADC_DEADZONE
#endif
#ifndef ADC_BOTTOM_DEADZONE
#   define ADC_BOTTOM_DEADZONE ADC_DEADZONE
#endif
#ifndef USER_DEADZONE
#   define USER_DEADZONE 50
#endif
#ifndef USER_TOP_DEADZONE
#   define USER_TOP_DEADZONE USER_DEADZONE
#endif
#ifndef USER_BOTTOM_DEADZONE
#   define USER_BOTTOM_DEADZONE USER_DEADZONE
#endif
#ifndef SCANS_WITHOUT_CHANGE
#   define SCANS_WITHOUT_CHANGE 3000
#endif
#ifndef AM_STARTUP_DELAY
#   define AM_STARTUP_DELAY 20
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
#   define AM_DC_FACTOR 0.1
#endif
#ifndef RECALIBRATED_SWITCHES
#   define RECALIBRATED_SWITCHES 5
#endif
#endif // ifdef DYNAMIC_CALIBRATION
//TODO: RM this once it's set by the script
//      Also use the same indexes and funcions for both halves if only one is set
//      -> I need to make sure that the resulting mux position exists on the other half, or else it will always count as triggered
#if !defined AM_INIT_KEY_NUM_R && defined AM_INIT_KEY_NUM
#define AM_INIT_KEY_NUM_R AM_INIT_KEY_NUM
#define AM_INIT_KEYS_R AM_INIT_KEYS
#define AM_INIT_FUNCTIONS_R AM_INIT_FUNCTIONS
#endif
// The minimum absolute difference in values for the switch to count as calibrated
#ifndef CAL_THRESHOLD
#   define CAL_THRESHOLD (5 * ADC_TOP_DEADZONE)
#endif
// If the switch reads a value below bottom_value + INIT_THRESHOLD, it is counted as pressed during initialization
#ifndef INIT_THRESHOLD
#   define INIT_THRESHOLD (4 * ADC_BOTTOM_DEADZONE)
#endif
#ifndef TOP_DEADZONE_MULT
#   define TOP_DEADZONE_MULT 1.3
#endif

/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
#if AM_PROFILE_NUM > 1
#define PROFILE_MUTABLE
#else
#define PROFILE_MUTABLE const
#endif // if AM_PROFILE_NUM > 1

//MARK: Filters
#ifndef FILTER_STRENGTH
#   define FILTER_STRENGTH 3
#endif
#ifndef SLAVE_FILTER_STRENGTH
#   ifdef RIGHT_FILTER_STRENGTH
    #   define SLAVE_FILTER_STRENGTH RIGHT_FILTER_STRENGTH
#   else
    #   define SLAVE_FILTER_STRENGTH FILTER_STRENGTH
#   endif
#endif

#if FILTER_STRENGTH != 0
uint16_t adc_filter_function(uint16_t value, uint8_t index);
#if FILTER_STRENGTH != SLAVE_FILTER_STRENGTH
uint16_t adc_slave_filter_function(uint16_t value, uint8_t index);
#endif
#else
#   define adc_filter(value, index) value
#endif

//MARK: Variables
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
extern SPLIT_VIA_MUT uint8_t switch_low;
extern SPLIT_MUTABLE pin_t adc_pins[ADC_PIN_NUM];
extern adc_mux adc_pin_mux[ADC_PIN_NUM];
#if defined MUX_PINS || defined MUX_PINS_R
extern SPLIT_MUTABLE pin_t mux_pins[MUX_PIN_NUM];
#endif
#ifdef POWER_PINS
extern SPLIT_MUTABLE uint8_t power_pin_num;
// Set the sensor power pins and delay, if defined
void set_sensor_power(uint8_t index);
#endif
extern uint8_t am_highest_layer;
extern PROFILE_MUTABLE uint8_t active_profile;

#ifdef USE_PRIORITY_MODE
extern bool priority_mode;
#endif

#ifndef AM_NO_EEPROM
#include "nvm_eeconfig.h"
//TODO: Why is this global?
extern uint16_t calibration_data[SMAX(SWITCH_NUM)];
#endif
extern uint8_t top_deadzones[SMAX(SWITCH_NUM)];

#ifdef ADJUSTMENT_FUNCTION
__attribute__((weak)) uint16_t adjust(uint16_t value);
#else
#   define adjust(value) value
#endif

#ifdef SPLIT_KEYBOARD
#ifdef AM_NO_EEPROM
extern uint8_t switch_num_slave;
#endif
#endif

//MARK: Functions
/* Global Functions */
// Readies the keyboard state before scanning begins
void analog_matrix_init(void);
// Scans the keys and calls the evaluation function on them
uint8_t matrix_scan(void);
// Evaluates the value against the configured heights, to see if the switch state has changed. Returns true if changed.
bool evaluate_value(uint8_t index, uint16_t value);
// Populates the calibrated values from eeprom or hardcoded values, and applies deadzones
bool get_calibration_data(void);
// Get the switch data configured in the json
void get_key_config(void);
// Scans the init keys defined in the info.json
//TODO: Does this work if no init keys have been defined? I should just set the top left key as default if none are defined tbh
bool scan_init_keys(void);
// Translate the user defined heights and distances into the equivalent ADC values
void translate_mm_to_value(uint8_t index, bool init);
// Starts calibration of the bottom value
void calibrate_switches(bool init);
// Starts calibration of the top deadzone
void calibrate_top_value(void);
// Assigns side and configuration at init
void assign_side(void);
// Runs whenever the profile has changed
#if AM_PROFILE_NUM > 1
void profile_state_changed(uint8_t profile);
#else
#   define profile_state_changed(profile)
#endif
// Activates the profile passed as the parameter (only on the master half on split keyboards)
void set_active_profile(uint8_t profile);
// Returns the active profile
uint8_t get_active_profile(void);

bool get_profile_lock_state(void);
// Sets the profile lock state to the passed value
void set_profile_lock_state(bool value);
// Run actions when the layer state changes
layer_state_t layer_state_set_am(layer_state_t state);
// Empty loop for short delays
#ifdef AM_USE_DELAY
void wait_cycles(uint16_t delay);
#else
#   define wait_cycles(delay)
#endif
#ifdef CUSTOM_MATRIX_LITE
bool matrix_scan_custom(matrix_row_t current_matrix[]);
#endif
#ifdef USE_INIT_KEYS
// Init key helpers
void _bootmagic(bool init);
void _bootloader_jump(bool init);
#endif
#ifdef AM_NO_EEPROM
#if defined SPLIT_KEYBOARD
void _sync_cal(void);
#else
#   define _sync_cal()
#endif
#else
#   define _sync_cal()
void clear_calibration(void);
#endif

#ifdef POWER_BEFORE_SCAN
#   define set_sensor_power_high(index) set_sensor_power(index)
#   define set_sensor_power_low(index);
#elif defined CUSTOM_POWER_BEFORE_SCAN
volatile void sensor_power_init(void);
volatile void sensor_power_init_user(void);
volatile void set_sensor_power_high(uint8_t mux_channel);
volatile void set_sensor_power_high_user(uint8_t mux_channel);
volatile void set_sensor_power_low(uint8_t mux_channel);
volatile void set_sensor_power_low_user(uint8_t mux_channel);
volatile void sensor_power_toggle(uint8_t mux_channel, uint8_t adc_channel);
volatile void sensor_power_toggle_user(uint8_t mux_channel, uint8_t adc_channel);
#else
#   define set_sensor_power_high(index)
#   define set_sensor_power_low(index)
#endif

//TODO: Similar to the LAYOUT macro, write a script to define this macro
/*
 #define MATRIX(k0A, k0B, k0C, k0D, k0E, k0F, k6A, k6B, k6C, k6D, k6E, k6F, k1A, k1B, k1C, k1D, k1E, k1F, k7A, k7B, k7C, k7D, k7E, k7F, k2A, k2B, k2C, k2D, k2E, k2F, k8A, k8B, k8C, k8D, k8E, k8F, k3A, k3B, k3C, k3D, k3E, k3F, k9A, k9B, k9C, k9D, k9E, k9F, k4C, k4D, kAC, kAD, k5D, k4E, k4F, kBA, kAA, kAB, k5E, k5F, kBB) \
               {k0A, k0B, k0C, k0D, k0E, k0F, k1A, k1B, k1C, k1D, k1E, k1F, k2A, k2B, k2C, k2D, k2E, k2F, k3A, k3B, k3C, k3D, k3E, k3F, k4C, k4D, k5D, k4E, k4F, k5E, k5F, k6A, k6B, k6C, k6D, k6E, k6F, k7A, k7B, k7C, k7D, k7E, k7F, k8A, k8B, k8C, k8D, k8E, k8F, k9A, k9B, k9C, k9D, k9E, k9F, kAC, kAD, kBA, kAA, kAB, kBB}
*/

