#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include "gpio.h"
#include "he_config.h"
#include "info_config.h"
#include "matrix_ref.h"
#include "analog.h"

#define RAPID_TRIGGER 0
#define CONTINUOUS_RAPID_TRIGGER 1
#define CONSTANT_RAPID_TRIGGER 2

typedef struct Switch {
    bool pressed;
#   if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
    // The switch is counted as pressed below this value
    uint16_t trigger_value;
    // The switch is counted as released above this value
    uint16_t release_value;
#   endif
#   if RAPID_TRIGGER_TYPE > 0
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


volatile Switch matrix[SWITCH_NUM];
#ifdef MUX_PINS
volatile static const pin_t mux_pins[] = MUX_PINS;
#endif
volatile static const pin_t adc_pins[] = ADC_PINS;
volatile static adc_mux adc_pin_mux[ADC_PIN_NUM];
volatile static const uint8_t mux_to_num[MUX_CHANNELS][ADC_PIN_NUM] = MUX_TO_NUM;
volatile static const uint8_t num_to_matrix[SWITCH_NUM][2] = NUM_TO_MATRIX;

#ifdef POWER_PINS
volatile static const pin_t power_pins[] = POWER_PINS;
#endif

//TODO: Convert automatically using a define?
//      Would have to first put all min/max values into an array to use them here, need to be known at compile time?
#if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
volatile static const float trigger_height[] = TRIGGER_HEIGHT;
#if defined RELEASE_HEIGHT
volatile static const float release_height[] = RELEASE_HEIGHT;
#else
volatile static const float release_height[] = TRIGGER_HEIGHT;
#endif
#endif
#if RAPID_TRIGGER_TYPE > 0
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
#ifndef SMOOTHING_LEVEL
#   define SMOOTHING_LEVEL 2
#endif
//TODO: Set these in python script to scale with adc resolution
#define ADC_SMOOTHING SMOOTHING_LEVEL * 2 + 1
#ifndef DEADZONE_LEVEL
#   define DEADZONE_LEVEL 4
#endif
#define ADC_DEADZONE DEADZONE_LEVEL * 5 + 5

/* Weak function defines */
volatile void sensor_power_high_kb(uint8_t mux_channel);
volatile void sensor_power_high_user(uint8_t mux_channel);
volatile void sensor_power_low_kb(uint8_t mux_channel);
volatile void sensor_power_low_user(uint8_t mux_channel);

volatile void sensor_power_init_kb(void);
volatile void sensor_power_init_user(void);

/* Random stuff */
//TODO: Do I need this?
typedef enum translation_type_t {
    translate_trigger_height = 0,
    translate_release_height,
    translate_rt_press,
    translate_rt_release,
    translate_all

} translation_type_t;
