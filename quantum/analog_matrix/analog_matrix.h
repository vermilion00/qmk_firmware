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

#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
#   define SPLIT_MUTABLE
#   define CONFIG_MUTABLE
#else
#   if !defined SPLIT_KEYBOARD || KEYBOARD_SIDE == LEFT
    #   define CONFIG_MUTABLE const
#   endif
#   define SPLIT_MUTABLE const
#endif

#ifndef SLAVE_DEADZONE_MULT
#   define SLAVE_DEADZONE_MULT 2.5
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
    #ifdef USE_PRIORITY_MODE
    bool priority_profile;
    #endif
} Profile;

// #ifdef VIA_ENABLE
// // Switch data for one profile stored in EEPROM
// // Heights are multiplied by 100 -> 2.5 mm becomes 250
// typedef struct height_data_t {
//     uint8_t mode;
//     uint16_t trigger_height;
//     uint16_t release_height;
//     uint16_t rt_press_distance;
//     uint16_t rt_release_distance;
// } height_data_t;
// #endif

//MARK: Key struct
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

//MARK: Defaults
/* Configuration defaults */
#if defined ADC_RESOLUTION && ADC_RESOLUTION < 12
#   warning "ADC_RESOLUTION will be forced to 12 bits for the analog matrix!"
#undef ADC_RESOLUTION
#define ADC_RESOLUTION 12
#endif
#define MAX_ADC_VALUE 4095

#ifndef ADC_DEADZONE
#   define ADC_DEADZONE 140
#endif
#ifndef ADC_SMOOTHING
#   define ADC_SMOOTHING 80
#endif
#ifndef ADC_TOP_DEADZONE
#   define ADC_TOP_DEADZONE ADC_DEADZONE
#endif
#ifndef ADC_BOTTOM_DEADZONE
#   define ADC_BOTTOM_DEADZONE ADC_DEADZONE
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
#   define AM_DC_FACTOR 0.15
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

/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
extern bool manual_profile_lock;
#if AM_PROFILE_NUM > 1
#define PROFILE_MUTABLE
#else
#define PROFILE_MUTABLE const
#endif // if AM_PROFILE_NUM > 1

//MARK: Filters
#ifndef ADC_FILTER_STRENGTH
#   define ADC_FILTER_STRENGTH 3
#endif
#ifndef ADC_SLAVE_FILTER_STRENGTH
#   define ADC_SLAVE_FILTER_STRENGTH ADC_FILTER_STRENGTH
#endif

#if ADC_FILTER_STRENGTH != 0
uint16_t adc_filter_function(uint16_t value, uint8_t index);
#if ADC_FILTER_STRENGTH != ADC_SLAVE_FILTER_STRENGTH
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

//MARK: Functions
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

//TODO: Remove
void _sync_cal(void);

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

