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

typedef struct Switch {
    uint8_t pressed;
#   if RAPID_TRIGGER_TYPE != NONE
    // Is rapid trigger enabled for this switch?
    uint8_t rt_enabled;
    // The switch is counted as pressed, when the rapid trigger crosses this threshold
    // As the switch is traveling downward, this value is constantly updated, so the release distance is simply checked against this value to determine if the switch should be released
    uint16_t rt_press_threshold;
    // The distance that the switch is required to travel downwards before it's registered as pressed
    uint16_t rt_press_value;
    // The distance that the switch is required to travel upwards before it's registered as released
    uint16_t rt_release_value;
#   endif
#   if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
    // The switch is counted as pressed below this value
    uint16_t trigger_value;
    // The switch is counted as released above this value
    uint16_t release_value;
#   endif
    uint16_t bottom_value;
    uint16_t top_value;
} Switch;

volatile static Switch he_matrix[SWITCH_NUM];
#ifdef MUX_PINS
volatile static const pin_t mux_pins[MUX_PIN_NUM] = MUX_PINS;
#endif
volatile static const pin_t adc_pins[ADC_PIN_NUM] = ADC_PINS;
volatile static adc_mux adc_pin_mux[ADC_PIN_NUM];
// Used to translate from the ADC pin/Mux combination to the switch number
volatile static const uint8_t mux_to_num[MUX_CHANNELS][ADC_PIN_NUM] = MUX_TO_NUM;
// Used to translate from the switch number to the QMK layout position
volatile static const uint8_t num_to_matrix[SWITCH_NUM][2] = NUM_TO_MATRIX;
#ifdef POWER_PINS
volatile static const pin_t power_pins[POWER_PIN_NUM] = POWER_PINS;
#endif

#if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
// volatile static const float trigger_height[SWITCH_NUM] = TRIGGER_HEIGHT;
//TODO: Change the definition in python to make this one define instead of 4
volatile static const float trigger_height[HE_PROFILE_NUM][SWITCH_NUM] = {TRIGGER_HEIGHT TRIGGER_HEIGHT1 TRIGGER_HEIGHT2 TRIGGER_HEIGHT3};
#if defined RELEASE_HEIGHT
// volatile static const float release_height[SWITCH_NUM] = RELEASE_HEIGHT;
volatile static const float release_height[HE_PROFILE_NUM][SWITCH_NUM] = {RELEASE_HEIGHT RELEASE_HEIGHT1 RELEASE_HEIGHT2 RELEASE_HEIGHT3};
#else
volatile static const float release_height[HE_PROFILE_NUM][SWITCH_NUM] = {TRIGGER_HEIGHT TRIGGER_HEIGHT1 TRIGGER_HEIGHT2 TRIGGER_HEIGHT3};
#endif
#endif
#if RAPID_TRIGGER_TYPE != NONE
volatile static const float rt_press_distance[HE_PROFILE_NUM][SWITCH_NUM] = {RT_PRESS_DISTANCE RT_PRESS_DISTANCE1 RT_PRESS_DISTANCE2 RT_PRESS_DISTANCE3};
#if defined RT_RELEASE_DISTANCE
volatile static const float rt_release_distance[HE_PROFILE_NUM][SWITCH_NUM] = {RT_RELEASE_DISTANCE RT_RELEASE_DISTANCE1 RT_RELEASE_DISTANCE2 RT_RELEASE_DISTANCE3};
#else
volatile static const float rt_release_distance[HE_PROFILE_NUM][SWITCH_NUM] = {RT_PRESS_DISTANCE RT_PRESS_DISTANCE1 RT_PRESS_DISTANCE2 RT_PRESS_DISTANCE3};
#endif
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
//TODO: I don't think these are needed
// #ifndef INVERT_ADC
// #   define INVERT_ADC FALSE
// #endif
// #ifndef POWER_BEFORE_SCAN
// #   define POWER_BEFORE_SCAN FALSE
// #endif
// #ifndef CUSTOM_POWER_BEFORE_SCAN
// #   define CUSTOM_POWER_BEFORE_SCAN FALSE
// #endif
// #ifndef DEBUG_SCAN_VALUE
// #   define DEBUG_SCAN_VALUE FALSE
// #endif
// #ifndef DEBUG_CALIBRATION
// #   define DEBUG_CALIBRATION FALSE
// #endif

#define HE_PROFILE_NUM 4
/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
#if HE_PROFILE_NUM > 1
uint8_t current_he_profile = HE_DEFAULT_PROFILE;
void switch_to_profile(uint8_t profile);
#if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
volatile static uint16_t trigger_value[HE_PROFILE_NUM][SWITCH_NUM];
volatile static uint16_t release_value[HE_PROFILE_NUM][SWITCH_NUM];
#endif // if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
#if RAPID_TRIGGER_TYPE != NONE
volatile static uint16_t rt_press_value[HE_PROFILE_NUM][SWITCH_NUM];
volatile static uint16_t rt_release_value[HE_PROFILE_NUM][SWITCH_NUM];
#endif // if RAPID_TRIGGER_TYPE != NONE

typedef struct Profile {
    //heights are stored in their own array
    layer_state_t layers;
    rapid_trigger_t rt_type;
    matrix_row_t rt_mask[MATRIX_COLS];
} Profile;

volatile static const Profile profiles[HE_PROFILE_NUM] = {{0, 0, {0,0,0}}, {2, 1, 5}};
// volatile static const Profile profiles[HE_PROFILE_NUM] = HE_PROFILE_CONFIG;
#endif // if HE_PROFILE_NUM > 1

/* Function defines */
uint8_t get_current_profile(void);

volatile void sensor_power_init_kb(void);
volatile void sensor_power_init_user(void);
volatile void sensor_power_high_kb(uint8_t mux_channel);
volatile void sensor_power_high_user(uint8_t mux_channel);
volatile void sensor_power_low_kb(uint8_t mux_channel);
volatile void sensor_power_low_user(uint8_t mux_channel);
