#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include "gpio.h"
#include "he_config.h"
#include "info_config.h"
#include "matrix_ref.h"
#include "analog.h"

typedef struct Switch {
    bool pressed;
#   ifndef CONSTANT_RAPID_TRIGGER
    // The switch is counted as pressed below this value
    uint16_t trigger_value;
    // The switch is counted as released above this value
    uint16_t release_value;
#   endif
#   if defined(RAPID_TRIGGER) || defined(CONTINUOUS_RAPID_TRIGGER) || defined(CONSTANT_RAPID_TRIGGER)
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

#ifndef DIRECT_ADC_PINS
volatile static const pin_t mux_pins[] = MUX_PINS;
#endif
volatile static const pin_t he_pins[] = HE_PINS;
//adc_mux is a struct of 1 uint16_t and 1 uint8_t
volatile static adc_mux he_mux[sizeof(he_pins)/sizeof(adc_mux)];
//pin_t is equal to uint32_t
volatile static const uint8_t mux_to_matrix[sizeof(he_pins)/sizeof(pin_t)][1 << (sizeof(mux_pins)/sizeof(pin_t))] = MUX_TO_MATRIX;

#ifdef POWER_PINS
volatile static const pin_t power_pins[] = POWER_PINS;
#endif

//TODO: Convert automatically using a define?
//      Would have to first put all min/max values into an array to use them here, need to be known at compile time?
#ifndef CONSTANT_RAPID_TRIGGER
volatile static const float trigger_height[] = TRIGGER_HEIGHT;
#if defined RELEASE_HEIGHT
volatile static const float release_height[] = RELEASE_HEIGHT;
#else
volatile static const float release_height[] = TRIGGER_HEIGHT;
#endif
#endif
#if defined(RAPID_TRIGGER) || defined(CONTINUOUS_RAPID_TRIGGER) || defined(CONSTANT_RAPID_TRIGGER)
volatile static const float rt_press_distance[] = RT_PRESS_DISTANCE;
#if defined RT_RELEASE_DISTANCE
volatile static const float rt_release_distance[] = RT_RELEASE_DISTANCE;
#else
volatile static const float rt_release_distance[] = RT_PRESS_DISTANCE;
#endif
#endif


// volatile Switch matrix[CMATRIX_ROWS][CMATRIX_COLS];
//TODO: Rework this into an array of rows, like standard qmk?
volatile Switch matrix[SWITCH_NUM];

volatile static const uint8_t he_to_qmk[SWITCH_NUM][2] = HE_TO_QMK;




/* Configuration defaults */
#ifndef ADC_RESOLUTION
#   define ADC_RESOLUTION 12
#elif ADC_RESOLUTION > 16
#   error "ADC_RESOLUTION can't be higher than 16 bits!"
#endif
#define MAX_ADC_VALUE 1 << ADC_RESOLUTION
#ifndef SMOOTHING_LEVEL
#   define SMOOTHING_LEVEL 1
#endif
#ifndef SMOOTHING_LEVEL
#   define SMOOTHING_LEVEL 2
#endif
#define ADC_SMOOTHING SMOOTHING_LEVEL * 2 + 1
#ifndef DEADZONE_LEVEL
#   define DEADZONE_LEVEL 4
#endif
#define ADC_DEADZONE DEADZONE_LEVEL * 5 + 5
#if !defined POWER_BEFORE_SCAN && !defined CUSTOM_POWER_BEFORE_SCAN && !defined CONSTANT_POWER
#   define CONSTANT_POWER
#endif
//TODO: Set this in build system
#if !defined MUX_PIN_NUM
#   define MUX_PIN_NUM 4
#endif

/* Weak function defines */
volatile void sensor_power_high_kb(uint8_t mux_channel);
volatile void sensor_power_high_user(uint8_t mux_channel);
volatile void sensor_power_low_kb(uint8_t mux_channel);
volatile void sensor_power_low_user(uint8_t mux_channel);

volatile void sensor_power_init_kb(void);
volatile void sensor_power_init_user(void);

/* Random stuff */
typedef enum translation_type_t {
    translate_trigger_height = 0,
    translate_release_height,
    translate_rt_press,
    translate_rt_release,
    translate_all

} translation_type_t;
