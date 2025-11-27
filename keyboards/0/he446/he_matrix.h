#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include "action_layer.h"
#include "gpio.h"
#include "he_config.h"
#include "he_matrix.h"
#include "info_config.h"
#include "analog.h"
#include "constants.h"
#include "matrix.h"
#include "util.h"
#include "bootloader.h"
#include "bootmagic/bootmagic.h"

/* Current matrix implementation:
 * matrix is an array of Switch structs, which hold all information relevant to the switch
 * Each height param is an array holding the values for all profiles.
 * During scanning, the current_profile variable is checked to see which height to evaluate
 * To switch profiles, only the current_profile variable needs to be changed.
 */

typedef struct Profile {
    layer_state_t layers;
    // #if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
    // uint16_t trigger_value[SWITCH_NUM];
    // uint16_t release_value[SWITCH_NUM];
    // #endif
    // #if RAPID_TRIGGER_TYPE != NONE
    rt_type_t rt_type;
    // uint16_t rt_press_value[SWITCH_NUM];
    // uint16_t rt_release_value[SWITCH_NUM];
    uint16_t rt_mask[CEILING((SWITCH_NUM + SWITCH_NUM_R), 16)];
    // #endif
} Profile;

//TODO: Bit fields are an option to cut down on space, but it will cause a performance hit
//      because all values will have to be bitshifted every access
typedef struct Switch {
    uint8_t pressed;
    // Is rapid trigger enabled for this switch?
    //TODO: Maybe allow setting the mode for each individual switch, if it's already uint8
    rt_type_t rt_type[HE_PROFILE_NUM];
#   if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
    // The switch is counted as pressed, when the rapid trigger crosses this threshold
    // As the switch is traveling downward, this value is constantly updated, so the release distance is simply checked against this value to determine if the switch should be released
    uint16_t rt_press_threshold;
    // The distance that the switch is required to travel downwards before it's registered as pressed
    uint16_t rt_press_value[HE_PROFILE_NUM];
    // The distance that the switch is required to travel upwards before it's registered as released
    uint16_t rt_release_value[HE_PROFILE_NUM];
#   endif
#   if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
    // The switch is counted as pressed below this value
    uint16_t trigger_value[HE_PROFILE_NUM];
    // The switch is counted as released above this value
    uint16_t release_value[HE_PROFILE_NUM];
#   endif
    uint16_t bottom_value;
    uint16_t top_value;
    uint8_t row;
    uint8_t col;
} Switch;

volatile static Switch he_matrix[SWITCH_NUM];
#ifdef MUX_PINS
volatile static SPLIT_MUTABLE pin_t mux_pins[MUX_PIN_NUM] = MUX_PINS;
#endif
volatile static SPLIT_MUTABLE pin_t adc_pins[ADC_PIN_NUM] = ADC_PINS;
// During initialization, the adc pins are translated to the adc mux combination that the adc_read function uses
volatile static adc_mux adc_pin_mux[ADC_PIN_NUM];
#ifdef POWER_PINS
volatile static SPLIT_MUTABLE pin_t power_pins[POWER_PIN_NUM] = POWER_PINS;
#endif
// #if HE_INIT_KEY_NUM > 0
// volatile static SPLIT_MUTABLE uint8_t init_keys[HE_INIT_KEY_NUM][2] = HE_INIT_KEYS;
// volatile SPLIT_MUTABLE void (*init_functions[HE_INIT_KEY_NUM])(void) = HE_INIT_FUNCTIONS;
// #endif

// Used to translate from the ADC pin/Mux combination to the switch number
volatile static SPLIT_MUTABLE uint8_t mux_to_num[MUX_CHANNELS][ADC_PIN_NUM] = MUX_TO_NUM;
// Used to translate from the switch number to the QMK layout position
volatile static SPLIT_MUTABLE uint8_t num_to_matrix[SWITCH_NUM][2] = NUM_TO_MATRIX;
// Used to translate from the QMK layout position to the switch number
volatile static SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS][MATRIX_COLS] = MATRIX_TO_NUM;
// Used to translate from the matrix index to the ADC pin/Mux combination
volatile static SPLIT_MUTABLE uint8_t num_to_mux[SWITCH_NUM][2] = NUM_TO_MUX;

#if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
volatile static SPLIT_MUTABLE float trigger_height[HE_PROFILE_NUM][SWITCH_NUM] = TRIGGER_HEIGHT;
volatile static SPLIT_MUTABLE float release_height[HE_PROFILE_NUM][SWITCH_NUM] = RELEASE_HEIGHT;
#endif

#if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
volatile static SPLIT_MUTABLE float rt_press_distance[HE_PROFILE_NUM][SWITCH_NUM] = RT_PRESS_DISTANCE;
volatile static SPLIT_MUTABLE float rt_release_distance[HE_PROFILE_NUM][SWITCH_NUM] = RT_RELEASE_DISTANCE;
#endif

// Only need these for split keyboards
#ifdef SPLIT_KEYBOARD
// volatile static const uint8_t local_to_global_index[SWITCH_NUM][2] = L_TO_G_INDEX;

#else // defined SPLIT_KEYBOARD
#define SPLIT_MUTABLE const
#endif // else defined SPLIT_KEYBOARD


/* Configuration defaults */
#ifndef HE_ADC_RESOLUTION
#   define HE_ADC_RESOLUTION 10
#elif HE_ADC_RESOLUTION > 16
#   error "ADC_RESOLUTION can't be higher than 16 bits!"
#endif
#define ADC_RESOLUTION HE_ADC_RESOLUTION
#if HE_ADC_RESOLUTION <= 8
#   define ADC_BUFFER_DEPTH 1
#endif
#define MAX_ADC_VALUE 1 << ADC_RESOLUTION
#ifndef SMOOTHING_LEVEL
#   define SMOOTHING_LEVEL 1
#endif
//TODO: Set these in python script to scale with adc resolution
#ifndef ADC_SMOOTHING
#   define ADC_SMOOTHING SMOOTHING_LEVEL * 2 + 1
#endif
#ifndef DEADZONE_LEVEL
#   define DEADZONE_LEVEL 4
#endif
#ifndef ADC_DEADZONE
#   define ADC_DEADZONE DEADZONE_LEVEL * 5 + 5
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
#ifndef HE_DC_DELTA
#   define HE_DC_DELTA 7
#endif
// Needs to be less than one
#ifndef HE_DC_FACTOR
#   define HE_DC_FACTOR 0.9
#endif
#endif

/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
#if HE_PROFILE_NUM > 1
// volatile uint8_t current_he_profile;
void switch_to_profile(uint8_t profile);
#endif // if HE_PROFILE_NUM > 1
volatile static const Profile profiles[HE_PROFILE_NUM] = HE_PROFILE_CONFIG;

/* Function defines */
uint8_t get_current_profile(void);

volatile void sensor_power_init_kb(void);
volatile void sensor_power_init_user(void);
volatile void sensor_power_high_kb(uint8_t mux_channel);
volatile void sensor_power_high_user(uint8_t mux_channel);
volatile void sensor_power_low_kb(uint8_t mux_channel);
volatile void sensor_power_low_user(uint8_t mux_channel);
volatile void sensor_power_toggle_kb(uint8_t mux_channel, uint8_t adc_channel);
volatile void sensor_power_toggle_user(uint8_t mux_channel, uint8_t adc_channel);
