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
#include "info_config.h"
#include "keycodes.h"
#include "multiplexer.h"
// #include "debounce.h"
#include "atomic_util.h"
#include "matrix_ref.h"
#include "print.h"
//TODO: Does this need to be included?
#include "stm32_gpio.h"

// Checks which values need to be translated and calls translation function with the correct args
void translate_key_values(void);
// Translate the user defined trigger height etc into the equivalent ADC values
void translate_mm_to_value(uint8_t index, translation_type_t type, bool all_switches);
// Gets the previously calibrated min/max values for each switch from the EEPROM
//TODO: Allow saving to flash
bool get_calibration_data(void);
// Saves the newly calibrated min/max values to the EEPROM
bool set_calibration_data(void);
// Runs a full keyboard calibration to get the ADC values for the bottom out and unpressed positions
void calibrate_switches(void);
// Checks if the switch is pressed or not, returns true if the state has changed
//TODO: Dynamically change type of arg based on matrix size
//      Create a matrix_index_t enum with nested #if statements checking the matrix sizes
static inline bool evaluate_value(uint8_t index, uint16_t value);
// Translates the HE matrix index to the QMK matrix format and sets the pressed state
static inline void translate_num_to_matrix(matrix_row_t current_matrix[], uint8_t index);

//MARK: Ini
void matrix_init_custom(void) {
    // TODO: initialize hardware and global matrix state here
    // Set mux pins to output, HE pins to analog input
    for(uint8_t i = 0; i < ADC_PIN_NUM; i++) {
        palSetLineMode(adc_pins[i], PAL_MODE_INPUT_ANALOG);
        // Convert adc pins to adc mux combination
        adc_pin_mux[i] = pinToMux(adc_pins[i]);
    }
    #ifdef MUX_PINS
    for(uint8_t i = 0; i < MUX_PIN_NUM; i++) {
        gpio_set_pin_output_push_pull(mux_pins[i]);
    }
    #endif
    #if defined POWER_BEFORE_SCAN
    for(uint8_t i = 0; i < POWER_PIN_NUM; i++) {
        gpio_set_pin_output_push_pull(power_pins[i]);
        gpio_write_pin_low(power_pins[i]);
    }
    #elif defined CUSTOM_POWER_BEFORE_SCAN
    sensor_power_init_kb();
    #endif

    // TODO: Load key matrix struct with calibration and distance data from the EEPROM
    // Get the min/max values of each switch from EEPROM
    if(!get_calibration_data()) {
    // If loading the calibration data fails, start calibration immediately
        calibrate_switches();
    }

    // Translate the trigger height etc into the equivalent ADC value
    translate_key_values();

    //TODO: Initialize ADC driver

    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}

//MARK: Scan
uint8_t matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;
    uint16_t adc_value;
    uint8_t matrix_index;

    for(uint8_t mux_channel = 0; mux_channel < MUX_CHANNELS; mux_channel++) {
        #if defined POWER_BEFORE_SCAN
        gpio_write_pin_high(power_pins[mux_channel]);
        //TODO: Add delay = POWER_DELAY_US;
        #elif defined CUSTOM_POWER_BEFORE_SCAN
        sensor_power_high_kb(mux_channel);
        #endif
        set_mux_channel(mux_channel);
        for(uint8_t adc_channel = 0; adc_channel < ADC_PIN_NUM; adc_channel++) {
            //TODO: Add possibility to toggle pins to power sensor rows/cols
            // Translate matrix mux and adc channels to matrix position
            matrix_index = mux_to_num[mux_channel][adc_channel];
            // Check if a switch is at the position (matrix index > 0), and scan if so
            if(matrix_index > 0){
                matrix_index -= 1;
                adc_value = adc_read(adc_pin_mux[adc_channel]);
                // Check if key is pressed/released and set matrix_has_changed, returns true if the switch state has changed
                if(evaluate_value(matrix_index, adc_value)) {
                    matrix_has_changed = true;
                    //TODO: Transform matrix index into row/col combination and adjust current_matrix[] accordingly
                    // Use the layout macro or something equivalent?
                    translate_num_to_matrix(current_matrix, matrix_index);
                }

                // print statements only work if debug mode is enabled in keymap and json
                // uprintf("C%i: %i,  ", mux_channel, adc_value);
            }
        }
        #if defined POWER_BEFORE_SCAN
        gpio_write_pin_low(power_pins[mux_channel]);
        #elif defined CUSTOM_POWER_BEFORE_SCAN
        sensor_power_low_kb(mux_channel);
        #endif
    }
    // print("\n");

    // This *must* be called for correct keyboard behavior
    matrix_scan_kb();

    return matrix_has_changed;
}

//MARK: Translate all
//TODO: use assignment like trigger_height[] = {[0...SWITCH_NUM] = TRIGGER_HEIGHT}; using the build system
void translate_key_values(void) {

    // If the config arrays only contain one value, use it for all keys
    // #if !defined CONSTANT_RAPID_TRIGGER
    // bool same_trigger_height = false;
    // bool same_release_height = false;
    // if(sizeof(trigger_height)/sizeof(float) == 1) {
    //     same_trigger_height = true;
    // }
    // if(sizeof(release_height)/sizeof(float) == 1) {
    //     same_release_height = true;
    // }
    // #endif
    // #if defined RAPID_TRIGGER || defined CONSTANT_RAPID_TRIGGER || defined CONTINUOUS_RAPID_TRIGGER
    // bool same_press_distance = false;
    // bool same_release_distance = false;
    // if(sizeof(rt_press_distance)/sizeof(float) == 1) {
    //     same_press_distance = true;
    // }
    // if(sizeof(rt_release_distance)/sizeof(float) == 1) {
    //     same_release_distance = true;
    // }
    // #endif
    // if(same_trigger_height )
}

//MARK: Translate one
// At initialization, translate the trigger height etc into the corresponding ADC values
void translate_mm_to_value(uint8_t index, translation_type_t type, bool all_switches) {
    uint16_t bottom_value = matrix[index].bottom_value;
    uint16_t top_value = matrix[index].top_value;
    // ADC count per mm of travel
    //TODO: Add per key travel distance option
    uint16_t travel_unit = floor((top_value - bottom_value) / TRAVEL_DISTANCE);

    switch(type) {
        // Release height check happens in he_matrix.h
        case translate_trigger_height:
            matrix[index].trigger_value = travel_unit * trigger_height[index] + bottom_value;
            break;
        case translate_release_height:
            matrix[index].release_value = travel_unit * release_height[index] + bottom_value;
            break;
        case translate_rt_press:
            matrix[index].rt_press_value = travel_unit * rt_press_distance[index];
            break;
        case translate_rt_release:
            matrix[index].rt_release_value = travel_unit * rt_release_distance[index];
            break;
        case translate_all:
            #if RAPID_TRIGGER_TYPE == CONSTANT_RAPID_TRIGGER
            matrix[index].trigger_value = travel_unit * trigger_height[index] + bottom_value;
            matrix[index].release_value = travel_unit * release_height[index] + bottom_value;
            #endif
            #if RAPID_TRIGGER_TYPE > 0
            matrix[index].rt_press_value = travel_unit * rt_press_distance[index];
            matrix[index].rt_release_value = travel_unit * rt_release_distance[index];
            #endif
            break;
    }
}

//MARK: Get calibration
bool get_calibration_data(void) {
#   if defined NO_EEPROM
    const uint16_t top_values[] = TOP_VALUES;
    const uint16_t bottom_values[] = BOTTOM_VALUES;

    for(uint8_t key = 0; key < SWITCH_NUM; key++) {
        matrix[key].top_value = top_values[key];
        matrix[key].bottom_value = bottom_values[key];
    }

    return true;
#   else //if defined NO_EEPROM
    //TODO: Add eeprom support

#   endif //else defined NO_EEPROM
}

//MARK: Calibrate
void calibrate_switches(void) {

}

//MARK: Evaluate
//TODO: Dynamically change type of arg based on matrix size
// Create a matrix_index_t enum with nested #if statements checking the matrix sizes
static inline bool evaluate_value(uint8_t index, uint16_t value) {
//TODO: Evtl remake this to be changeable at runtime (if performance is enough)
//Basically just check a var in the function call to check which to call, if via is not defined it'll be const
    bool prev_pressed = matrix[index].pressed;
#if defined INVERT_ADC
//TODO: Test this
#   if RAPID_TRIGGER_TYPE == RAPID_TRIGGER
    // Rapid trigger is only active when the switch is lower than the trigger and release height
    if(value > matrix[index].trigger_value + ADC_SMOOTHING) {
        // Set the new lowest value if needed
        if(value > matrix[index].rt_press_threshold + ADC_SMOOTHING) {
            matrix[index].pressed = true;
            matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value + matrix[index].rt_press_threshold + ADC_SMOOTHING) < matrix[index].rt_release_value) {
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

#   elif RAPID_TRIGGER_TYPE == CONTINUOUS_RAPID_TRIGGER
    static bool rt_active = false;
    // Rapid trigger activates below the trigger height, but only stops when fully released
    if(rt_active || (value > matrix[index].trigger_value + ADC_SMOOTHING)) {
        rt_active = true;
        // Set the new lowest value if needed
        if(value > matrix[index].rt_press_threshold + ADC_SMOOTHING) {
            matrix[index].pressed = true;
            matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value + matrix[index].rt_press_threshold + ADC_SMOOTHING) < matrix[index].rt_release_value) {
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

#   elif RAPID_TRIGGER_TYPE == CONSTANT_RAPID_TRIGGER
    // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
    if((value > matrix[index].rt_press_threshold + ADC_SMOOTHING) ||
        value > matrix[index].bottom_value - ADC_DEADZONE) {
        matrix[index].pressed = true;
        matrix[index].rt_press_threshold = value;
    // Check if the key has been released past the threshold
    } else if((value + matrix[index].rt_press_threshold + ADC_SMOOTHING) < matrix[index].rt_release_value) {
        matrix[index].pressed = false;
        // Set the new activation threshold
        matrix[index].rt_press_threshold = value + matrix[index].rt_press_value;
    // Check if the key has been completely released
    } else if(value < matrix[index].top_value + ADC_DEADZONE) {
        matrix[index].pressed = false;
        matrix[index].rt_press_threshold = value + matrix[index].rt_press_value;
    }

#   else //No rapid trigger
    if(value > matrix[index].trigger_value + ADC_SMOOTHING) {
        matrix[index].pressed = true;
    } else if(value + ADC_SMOOTHING < matrix[index].release_value) {
        matrix[index].pressed = false;
    } else {
        return false;
    }

#   endif //defined RAPID_TRIGGER

#else //defined INVERT_ADC
#   if RAPID_TRIGGER_TYPE == RAPID_TRIGGER
    // Rapid trigger is only active when the switch is lower than the trigger and release height
    if(value < matrix[index].trigger_value - ADC_SMOOTHING) {
        // Set the new lowest value if needed
        if(value < matrix[index].rt_press_threshold - ADC_SMOOTHING) {
            matrix[index].pressed = true;
            matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value - matrix[index].rt_press_threshold - ADC_SMOOTHING) > matrix[index].rt_release_value) {
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

#   elif RAPID_TRIGGER_TYPE == CONTINUOUS_RAPID_TRIGGER
    static bool rt_active = false;
    // Rapid trigger activates below the trigger height, but only stops when fully released
    if(rt_active || (value < matrix[index].trigger_value - ADC_SMOOTHING)) {
        rt_active = true;
        // Set the new lowest value if needed
        if(value < matrix[index].rt_press_threshold - ADC_SMOOTHING) {
            matrix[index].pressed = true;
            matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value - matrix[index].rt_press_threshold - ADC_SMOOTHING) > matrix[index].rt_release_value) {
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

#   elif RAPID_TRIGGER_TYPE == CONSTANT_RAPID_TRIGGER
    // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
    if((value < matrix[index].rt_press_threshold - ADC_SMOOTHING) ||
        value < matrix[index].bottom_value + ADC_DEADZONE) {
        matrix[index].pressed = true;
        matrix[index].rt_press_threshold = value;
    // Check if the key has been released past the threshold
    } else if((value - matrix[index].rt_press_threshold - ADC_SMOOTHING) > matrix[index].rt_release_value) {
        matrix[index].pressed = false;
        // Set the new activation threshold
        matrix[index].rt_press_threshold = value - matrix[index].rt_press_value;
    // Check if the key has been completely released
    } else if(value > matrix[index].top_value - ADC_DEADZONE) {
        matrix[index].pressed = false;
        matrix[index].rt_press_threshold = value - matrix[index].rt_press_value;
    }

#   else //No rapid trigger
    if(value < matrix[index].trigger_value - ADC_SMOOTHING) {
        matrix[index].pressed = true;
    } else if(value - ADC_SMOOTHING > matrix[index].release_value) {
        matrix[index].pressed = false;
    } else {
        return false;
    }

#   endif //defined RAPID_TRIGGER
#endif //defined INVERT_ADC
    return !(prev_pressed == matrix[index].pressed);
}

//MARK: Num to Matrix
static inline void translate_num_to_matrix(matrix_row_t current_matrix[], uint8_t index) {
    uint8_t row = num_to_matrix[index][0];
    uint8_t col = num_to_matrix[index][1];

    current_matrix[row] ^= 1 << col;
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
