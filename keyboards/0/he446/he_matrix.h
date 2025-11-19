#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include "gpio.h"
#include "he_config.h"
#include "info_config.h"
#include "matrix_ref.h"
#include "analog.h"

#define NONE 0
#define RAPID_TRIGGER 1
#define CONTINUOUS_RAPID_TRIGGER 2
#define CONSTANT_RAPID_TRIGGER 3

typedef struct Switch {
    uint8_t pressed;
#   if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
    // The switch is counted as pressed below this value
    uint16_t trigger_value;
    // The switch is counted as released above this value
    uint16_t release_value;
#   endif
#   if RAPID_TRIGGER_TYPE != NONE
    // The switch is counted as pressed, when the rapid trigger crosses this threshold
    // As the switch is traveling downward, this value is constantly updated, so the release distance is simply checked against this value to determine if the switch should be released
    uint16_t rt_press_threshold;
    // The distance that the switch is required to travel downwards before it's registered as pressed
    uint16_t rt_press_value;
    // The distance that the switch is required to travel upwards before it's registered as released
    uint16_t rt_release_value;
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
volatile static const float trigger_height[] = TRIGGER_HEIGHT;
#if defined RELEASE_HEIGHT
volatile static const float release_height[] = RELEASE_HEIGHT;
#else
volatile static const float release_height[] = TRIGGER_HEIGHT;
#endif
#endif
#if RAPID_TRIGGER_TYPE != NONE
volatile static const float rt_press_distance[] = RT_PRESS_DISTANCE;
#if defined RT_RELEASE_DISTANCE
volatile static const float rt_release_distance[] = RT_RELEASE_DISTANCE;
#else
volatile static const float rt_release_distance[] = RT_PRESS_DISTANCE;
#endif
#endif




/* Configuration defaults */
//TODO: Just use a default mapping value
#ifndef ADC_RESOLUTION
#   define ADC_RESOLUTION 12
#elif ADC_RESOLUTION > 16
#   error "ADC_RESOLUTION can't be higher than 16 bits!"
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

/* Weak function defines */
volatile void sensor_power_init_kb(void);
volatile void sensor_power_init_user(void);
volatile void sensor_power_high_kb(uint8_t mux_channel);
volatile void sensor_power_high_user(uint8_t mux_channel);
volatile void sensor_power_low_kb(uint8_t mux_channel);
volatile void sensor_power_low_user(uint8_t mux_channel);
