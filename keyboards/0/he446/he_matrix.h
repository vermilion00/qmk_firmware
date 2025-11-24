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

/* Current matrix implementation:
 * matrix is an array of Switch structs, which hold all information relevant to the switch
 * Each height param is an array holding the values for all profiles.
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
    // uint16_t rt_mask[ceil(SWITCH_NUM/16)];
    uint16_t rt_mask[CEILING(SWITCH_NUM, 16)];
    // #endif
} Profile;

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
volatile static const pin_t mux_pins[MUX_PIN_NUM] = MUX_PINS;
#endif
volatile static const pin_t adc_pins[ADC_PIN_NUM] = ADC_PINS;
volatile static adc_mux adc_pin_mux[ADC_PIN_NUM];

//TODO: Make one user defined mux_to_matrix[MUX_CHANNELS][ADC_PIN_NUM][2] thing
//      and split it into these two using python
//      Add a parameter to the layout macro and derive it from there
// Used to translate from the ADC pin/Mux combination to the switch number
volatile static const uint8_t mux_to_num[MUX_CHANNELS][ADC_PIN_NUM] = MUX_TO_NUM;
// Used to translate from the switch number to the QMK layout position
volatile static const uint8_t num_to_matrix[SWITCH_NUM][2] = NUM_TO_MATRIX;
#ifdef POWER_PINS
volatile static const pin_t power_pins[POWER_PIN_NUM] = POWER_PINS;
#endif

#if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
volatile static const float trigger_height[HE_PROFILE_NUM][SWITCH_NUM] = TRIGGER_HEIGHT;
volatile static const float release_height[HE_PROFILE_NUM][SWITCH_NUM] = RELEASE_HEIGHT;
#endif

#if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
volatile static const float rt_press_distance[HE_PROFILE_NUM][SWITCH_NUM] = RT_PRESS_DISTANCE;
volatile static const float rt_release_distance[HE_PROFILE_NUM][SWITCH_NUM] = RT_RELEASE_DISTANCE;
#endif


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


/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
#if HE_PROFILE_NUM > 1
// volatile uint8_t current_he_profile;
void switch_to_profile(uint8_t profile);

//TODO: If NO_EEPROM is defined
// volatile static Profile profiles[HE_PROFILE_NUM] = HE_PROFILE_CONFIG;
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
