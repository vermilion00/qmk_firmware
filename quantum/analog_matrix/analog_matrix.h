#pragma once

// #include "matrix.h"
#include <stdint.h>
// #include <stdbool.h>
// #include <sys/cdefs.h>
#include "action_layer.h"
// #include "gpio.h"
// #include "he_matrix.h"
#include "eeconfig.h"
#include "info_config.h"
#include "analog.h"
#include "constants.h"
// #include "util.h"
// #include "bootloader.h"
// #include "bootmagic/bootmagic.h"

//TODO: Decide what I'll do with this
//      Currently not very useful, might be nice to have if I allow associating a color with a profile
typedef struct Profile {
    layer_state_t layers;
} Profile;

//TODO: Bit fields are an option to cut down on space, but it will cause a performance hit
//      because all values will have to be bitshifted every access
typedef struct Switch {
    uint8_t pressed;
    // 0 = None, 1-3 = RT, 4-9 = Reserved, 10-255 = Special keys (Gamepad etc)
    uint8_t mode[AM_PROFILE_NUM];
    #if defined USE_CONTINUOUS_RAPID_TRIGGER
    uint8_t rt_active;
    #endif
    #if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
    // The switch is counted as pressed below this value
    uint16_t trigger_value[AM_PROFILE_NUM];
    // The switch is counted as released above this value
    uint16_t release_value[AM_PROFILE_NUM];
    #endif
    #if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
    // The switch is counted as pressed, when the rapid trigger crosses this threshold
    // As the switch is traveling downward, this value is constantly updated, so the release distance is simply checked against this value to determine if the switch should be released
    uint16_t rt_threshold;
    // The distance that the switch is required to travel downwards before it's registered as pressed
    uint16_t rt_press_value[AM_PROFILE_NUM];
    // The distance that the switch is required to travel upwards before it's registered as released
    uint16_t rt_release_value[AM_PROFILE_NUM];
    #endif
    uint16_t bottom_value;
    uint16_t top_value;
    uint8_t row;
    uint8_t col;
} Switch;

#ifndef AM_NO_EEPROM
typedef union {
    uint32_t raw;
    struct {
        uint16_t top_value;
        uint16_t bottom_value;
    };
} switch_data_t;
#endif

/* Configuration defaults */
#ifdef ADC_RESOLUTION
#   warning "ADC_RESOLUTION will be forced to 10 bits for the analog matrix!"
#undef ADC_RESOLUTION
#endif
#define ADC_RESOLUTION 10
#define MAX_ADC_VALUE (1 << ADC_RESOLUTION) - 1

#ifndef ADC_TOP_DEADZONE
#   define ADC_TOP_DEADZONE ADC_DEADZONE
#endif
#ifndef ADC_BOTTOM_DEADZONE
#   define ADC_BOTTOM_DEADZONE ADC_DEADZONE
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
#ifndef AM_DC_DELTA
#   define AM_DC_DELTA 6
#endif
// Needs to be less than one
#ifndef AM_DC_FACTOR
#   define AM_DC_FACTOR 0.8
#endif
#endif

/* Profile switching stuff */
// We always have one profile, but switching isn't needed until we have more
#if AM_PROFILE_NUM > 1
#define PROFILE_MUTABLE
// volatile uint8_t current_he_profile;
void switch_to_profile(uint8_t profile);
#else
#define PROFILE_MUTABLE const
#endif // if AM_PROFILE_NUM > 1
volatile static const Profile profiles[AM_PROFILE_NUM] = AM_PROFILE_CONFIG;



/* Function defines */
void analog_matrix_init(void);
uint8_t analog_matrix_scan(void);
uint8_t get_current_profile(void);

volatile void sensor_power_init_kb(void);
volatile void sensor_power_init_user(void);
volatile void set_sensor_power_high_kb(uint8_t mux_channel);
volatile void set_sensor_power_high_user(uint8_t mux_channel);
volatile void set_sensor_power_low_kb(uint8_t mux_channel);
volatile void set_sensor_power_low_user(uint8_t mux_channel);
volatile void sensor_power_toggle_kb(uint8_t mux_channel, uint8_t adc_channel);
volatile void sensor_power_toggle_user(uint8_t mux_channel, uint8_t adc_channel);
