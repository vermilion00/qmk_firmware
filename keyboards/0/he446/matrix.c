#include "matrix.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "analog.h"
#include "gpio.h"
#include "hal_pal.h"
#include "hal_pal_lld.h"
#include "he_config.h"
#include "he_matrix.h"
#include "keycodes.h"
#include "multiplexer.h"
// #include "debounce.h"
#include "atomic_util.h"
#include "matrix_ref.h"
#include "print.h"
//TODO: Does this need to be included?
// #include "stm32_gpio.h"

// Translate the user defined trigger height etc into the equivalent ADC values
void translate_mm_to_value(uint8_t index);
// Gets the previously calibrated min/max values for each switch from the EEPROM
//TODO: Allow saving to flash
bool get_calibration_data(void);
// Saves the newly calibrated min/max values to the EEPROM
bool set_calibration_data(void);
// Runs a full keyboard calibration to get the ADC values for the bottom out and unpressed positions
void calibrate_switches(void);

//TODO: Dynamically change type of arg based on matrix size
// Create a matrix_index_t enum with nested #if statements checking the matrix sizes
inline bool evaluate_value(uint8_t index, uint16_t value) {
//TODO: Evtl remake this to be changeable at runtime (if performance is enough)
//Basically just check a var in the function call to check which to call, if via is not defined it'll be const
    bool prev_pressed = matrix[index].pressed;
#if defined INVERT_ADC
//TODO: Test this
#   if defined(RAPID_TRIGGER)
    // Rapid trigger is only active when the switch is lower than the trigger and release height
    if(value > matrix[index].trigger_value + ADC_SMOOTHING){
        // Set the new lowest value if needed
        if(value > matrix[index].rt_press_threshold + ADC_SMOOTHING){
            matrix[index].pressed = true;
            matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value + matrix[index].rt_press_threshold + ADC_SMOOTHING) < matrix[index].rt_release_value){
            matrix[index].pressed = false;
            // Set the new activation threshold
            matrix[index].rt_press_threshold = value + matrix[index].rt_press_value;
        }
    //TODO: Check if it is faster to check if the key state is released, and only set values if they're not set already
    } else if(value + ADC_SMOOTHING < matrix[index].release_value) {
        // If the switch is not pressed past the threshold, reset it
        matrix[index].pressed = false;
        matrix[index].rt_press_threshold = matrix[index].trigger_value;
    }

#   elif defined(CONTINUOUS_RAPID_TRIGGER)
    static bool rt_active = false;
    // Rapid trigger activates below the trigger height, but only stops when fully released
    if(rt_active || (value > matrix[index].trigger_value + ADC_SMOOTHING)){
        rt_active = true;
        // Set the new lowest value if needed
        if(value > matrix[index].rt_press_threshold + ADC_SMOOTHING){
            matrix[index].pressed = true;
            matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value + matrix[index].rt_press_threshold + ADC_SMOOTHING) < matrix[index].rt_release_value){
            matrix[index].pressed = false;
            // Set the new activation threshold
            matrix[index].rt_press_threshold = value + matrix[index].rt_press_value;
        }
    // Check if the switch has been released completely
    } else if(value < matrix[index].top_value + ADC_DEADZONE) {
        matrix[index].pressed = false;
        matrix[index].rt_press_threshold = matrix[index].trigger_value;
        rt_active = false;
    }

#   elif defined(CONSTANT_RAPID_TRIGGER)
    // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
    if((value > matrix[index].rt_press_threshold + ADC_SMOOTHING) ||
        value > matrix[index].bottom_value - ADC_DEADZONE){
        matrix[index].pressed = true;
        matrix[index].rt_press_threshold = value;
    // Check if the key has been released past the threshold
    } else if((value + matrix[index].rt_press_threshold + ADC_SMOOTHING) < matrix[index].rt_release_value){
        matrix[index].pressed = false;
        // Set the new activation threshold
        matrix[index].rt_press_threshold = value + matrix[index].rt_press_value;
    // Check if the key has been completely released
    } else if(value < matrix[index].top_value + ADC_DEADZONE){
        matrix[index].pressed = false;
        matrix[index].rt_press_threshold = value + matrix[index].rt_press_value;
    }

#   else //No rapid trigger
    if(value > matrix[index].trigger_value + ADC_SMOOTHING){
        matrix[index].pressed = true;
    } else if(value + ADC_SMOOTHING < matrix[index].release_value){
        matrix[index].pressed = false;
    } else {
        return false;
    }

#   endif //defined RAPID_TRIGGER

#else //defined INVERT_ADC
#   if defined(RAPID_TRIGGER)
    // Rapid trigger is only active when the switch is lower than the trigger and release height
    if(value < matrix[index].trigger_value - ADC_SMOOTHING){
        // Set the new lowest value if needed
        if(value < matrix[index].rt_press_threshold - ADC_SMOOTHING){
            matrix[index].pressed = true;
            matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value - matrix[index].rt_press_threshold - ADC_SMOOTHING) > matrix[index].rt_release_value){
            matrix[index].pressed = false;
            // Set the new activation threshold
            matrix[index].rt_press_threshold = value - matrix[index].rt_press_value;
        }
    //TODO: Check if it is faster to check if the key state is released, and only set values if they're not set already
    } else if(value - ADC_SMOOTHING > matrix[index].release_value) {
        // If the switch is not pressed past the threshold, reset it
        matrix[index].pressed = false;
        matrix[index].rt_press_threshold = matrix[index].trigger_value;
    }

#   elif defined(CONTINUOUS_RAPID_TRIGGER)
    static bool rt_active = false;
    // Rapid trigger activates below the trigger height, but only stops when fully released
    if(rt_active || (value < matrix[index].trigger_value - ADC_SMOOTHING)){
        rt_active = true;
        // Set the new lowest value if needed
        if(value < matrix[index].rt_press_threshold - ADC_SMOOTHING){
            matrix[index].pressed = true;
            matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value - matrix[index].rt_press_threshold - ADC_SMOOTHING) > matrix[index].rt_release_value){
            matrix[index].pressed = false;
            // Set the new activation threshold
            matrix[index].rt_press_threshold = value - matrix[index].rt_press_value;
        }
    // Check if the switch has been released completely
    } else if(value > matrix[index].top_value - ADC_DEADZONE) {
        matrix[index].pressed = false;
        matrix[index].rt_press_threshold = matrix[index].trigger_value;
        rt_active = false;
    }

#   elif defined(CONSTANT_RAPID_TRIGGER)
    // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
    if((value < matrix[index].rt_press_threshold - ADC_SMOOTHING) ||
        value < matrix[index].bottom_value + ADC_DEADZONE){
        matrix[index].pressed = true;
        matrix[index].rt_press_threshold = value;
    // Check if the key has been released past the threshold
    } else if((value - matrix[index].rt_press_threshold - ADC_SMOOTHING) > matrix[index].rt_release_value){
        matrix[index].pressed = false;
        // Set the new activation threshold
        matrix[index].rt_press_threshold = value - matrix[index].rt_press_value;
    // Check if the key has been completely released
    } else if(value > matrix[index].top_value - ADC_DEADZONE){
        matrix[index].pressed = false;
        matrix[index].rt_press_threshold = value - matrix[index].rt_press_value;
    }

#   else //No rapid trigger
    if(value < matrix[index].trigger_value - ADC_SMOOTHING){
        matrix[index].pressed = true;
    } else if(value - ADC_SMOOTHING > matrix[index].release_value){
        matrix[index].pressed = false;
    } else {
        return false;
    }

#   endif //defined RAPID_TRIGGER
#endif //defined INVERT_ADC
    return !(prev_pressed == matrix[index].pressed);
}

void matrix_init_custom(void) {
    // TODO: initialize hardware and global matrix state here
    // Set mux pins to output, HE pins to analog input
    for(uint8_t i = 0; i < sizeof(mux_pins)/sizeof(pin_t); i++){
        gpio_set_pin_output_push_pull(mux_pins[i]);
    }
    for(uint8_t i = 0; i < sizeof(he_pins)/sizeof(pin_t); i++){
        palSetLineMode(he_pins[i], PAL_MODE_INPUT_ANALOG);
        // Convert adc pins to adc mux combination
        he_mux[i] = pinToMux(he_pins[i]);
    }
    #if defined POWER_BEFORE_SCAN
    for(uint8_t i = 0; i < sizeof(power_pins)/4; i++){
        gpio_set_pin_output_push_pull(power_pins[i]);
        gpio_write_pin_low(power_pins[i]);
    }
    #elif defined CUSTOM_POWER_BEFORE_SCAN
    sensor_power_init_kb();
    #endif

    // TODO: Load key matrix struct with calibration and distance data from the EEPROM
    // Get the min/max values of each switch from EEPROM
    if(!get_calibration_data()){
    // If loading the calibration data fails, start calibration immediately
        calibrate_switches();
    }

    // Translate the trigger height etc into the equivalent ADC value
    #if !defined CONSTANT_RAPID_TRIGGER
    for(uint8_t key = 0; key < (sizeof(trigger_height)/sizeof(float)); key++){
    #else
    for(uint8_t key = 0; key < (sizeof(rt_press_distance)/sizeof(float)); key++){
    #endif
        translate_mm_to_value(key);
    }

    //TODO: Initialize ADC driver

    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}

uint8_t matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;
    uint16_t adc_value;
    uint8_t matrix_index;

    for(uint8_t mux_channel = 0; mux_channel < (1 << (sizeof(mux_pins)/sizeof(pin_t))); mux_channel++){
        #if defined POWER_BEFORE_SCAN
        gpio_write_pin_high(power_pins[mux_channel]);
        //TODO: Add delay = POWER_DELAY_US;
        #elif defined CUSTOM_POWER_BEFORE_SCAN
        sensor_power_high_kb(mux_channel);
        #endif
        // set_mux_channel(mux_channel);
        set_mux_channel(0);
        for(uint8_t adc_channel = 0; adc_channel < sizeof(he_pins)/sizeof(pin_t); adc_channel++){
            //TODO: Add possibility to toggle pins to power sensor rows/cols
            // Translate matrix mux and adc channels to matrix position
            matrix_index = mux_to_matrix[mux_channel][adc_channel];
            // Check if a switch is at the position (matrix index > 0), and scan if so
            if(matrix_index > 0){
                // matrix_index = 1;
                adc_value = adc_read(he_mux[adc_channel]);
                // Check if key is pressed/released and set matrix_has_changed
                if(evaluate_value(matrix_index - 1, adc_value)){ matrix_has_changed = true; }
                // print statements only work if debug mode is enabled in keymap and json
                uprintf("C%i: %i,  ", mux_channel, adc_value);
            }
            //TODO: Add mode where keypress is evaluated only on master side?
        }
        #if defined POWER_BEFORE_SCAN
        gpio_write_pin_low(power_pins[mux_channel]);
        #elif defined CUSTOM_POWER_BEFORE_SCAN
        sensor_power_low_kb(mux_channel);
        #endif
    }
    print("\n");

    // This *must* be called for correct keyboard behavior
    matrix_scan_kb();

    return matrix_has_changed;
}

// At initialization, translate the trigger height etc into the corresponding ADC values
void translate_mm_to_value(uint8_t index){
    uint16_t bottom_value = matrix[index].bottom_value;
    uint16_t top_value = matrix[index].top_value;
    // ADC count per mm of travel
    uint16_t travel_unit = floor((top_value - bottom_value) / TRAVEL_DISTANCE);

    #if !defined CONSTANT_RAPID_TRIGGER
    matrix[index].trigger_value = travel_unit * trigger_height[index] + bottom_value;
    matrix[index].release_value = travel_unit * release_height[index] + bottom_value;
    #endif
    #if defined RAPID_TRIGGER || defined CONSTANT_RAPID_TRIGGER || defined CONTINUOUS_RAPID_TRIGGER
    matrix[index].rt_press_value = travel_unit * rt_press_distance[index];
    matrix[index].rt_release_value = travel_unit * rt_release_distance[index];
    #endif
}

bool get_calibration_data(void){
#   if defined NO_EEPROM
    uint16_t top_values[] = TOP_VALUES;
    uint16_t bottom_values[] = BOTTOM_VALUES;

    for(uint8_t key = 0; key < sizeof(top_values)/sizeof(uint16_t); key++){
        matrix[key].top_value = top_values[key];
        matrix[key].bottom_value = bottom_values[key];
    }
    return true;
#   else //if defined NO_EEPROM
    //TODO: Add eeprom support

#   endif //else defined NO_EEPROM
}

void calibrate_switches(void) {

}

/* Weak defines, to allow a custom powering logic */
__attribute__((weak)) void sensor_power_init_kb(void) { sensor_power_init_user(); }

__attribute__((weak)) void sensor_power_init_user(void) {}

__attribute__((weak)) void sensor_power_high_kb(uint8_t mux_channel) { sensor_power_high_user(mux_channel); }

__attribute__((weak)) void sensor_power_high_user(uint8_t mux_channel) {}

__attribute__((weak)) void sensor_power_low_kb(uint8_t mux_channel) { sensor_power_low_user(mux_channel); }

__attribute__((weak)) void sensor_power_low_user(uint8_t mux_channel) {}





/* Standard weak defines */
__attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }

__attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }

__attribute__((weak)) void matrix_init_user(void) {}

__attribute__((weak)) void matrix_scan_user(void) {}
