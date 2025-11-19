#include "matrix.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "analog.h"
#include "debug.h"
#include "gpio.h"
#include "hal_pal.h"
#include "hal_pal_lld.h"
#include "he_config.h"
#include "he_matrix.h"
#include "info_config.h"
#include "keycodes.h"
#include "multiplexer.h"
#include "atomic_util.h"
#include "matrix_ref.h"
#include "print.h"
#include "stm32_gpio.h"

// Checks which values need to be translated and calls translation function with the correct args
void translate_key_values(void);
// Translate the user defined trigger height etc into the equivalent ADC values
void translate_mm_to_value(uint8_t index);
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

//MARK: Init
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
    #if POWER_BEFORE_SCAN == TRUE
    for(uint8_t i = 0; i < POWER_PIN_NUM; i++) {
        gpio_set_pin_output_push_pull(power_pins[i]);
        gpio_write_pin_low(power_pins[i]);
    }
    #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
    sensor_power_init_kb();
    #endif

    // TODO: Load key matrix struct with calibration and distance data from the EEPROM
    // Get the min/max values of each switch from EEPROM
    if(!get_calibration_data()) {
    // If loading the calibration data fails, start calibration immediately
        calibrate_switches();
    }

    // Translate the trigger height etc into the equivalent ADC value
    for(uint8_t index = 0; index < SWITCH_NUM; index++) {
        translate_mm_to_value(index);
    }

    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}

//MARK: Scan
uint8_t matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;
    uint16_t adc_value;
    uint8_t matrix_index;

    for(uint8_t mux_channel = 0; mux_channel < MUX_CHANNELS; mux_channel++) {
        #if DEBUG_SCAN_VALUE == TRUE
        dprintf("Mux: %i\n", mux_channel);
        #endif
        #if POWER_BEFORE_SCAN == TRUE
        gpio_write_pin_high(power_pins[mux_channel]);
        //TODO: Add delay = POWER_DELAY_US;
        #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
        sensor_power_high_kb(mux_channel);
        #endif
        set_mux_channel(mux_channel);
        for(uint8_t adc_channel = 0; adc_channel < ADC_PIN_NUM; adc_channel++) {
            // Translate matrix mux and adc channels to matrix position
            matrix_index = mux_to_num[mux_channel][adc_channel];
            // Check if a switch is at the position (matrix index > 0), and scan if so
            if(matrix_index > 0){
                matrix_index -= 1;
                adc_value = adc_read(adc_pin_mux[adc_channel]);
                #if DEBUG_SCAN_VALUE == TRUE
                dprintf("%i: %i,  ", adc_channel, adc_value);
                #endif

                // Check if key is pressed/released and set matrix_has_changed, returns true if the switch state has changed
                if(evaluate_value(matrix_index, adc_value)) {
                    matrix_has_changed = true;
                    translate_num_to_matrix(current_matrix, matrix_index);
                }
            }
        }
        #if DEBUG_SCAN_VALUE == TRUE
        dprint("\n\n");
        #endif
        #if POWER_BEFORE_SCAN == TRUE
        gpio_write_pin_low(power_pins[mux_channel]);
        #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
        sensor_power_low_kb(mux_channel);
        #endif
    }

    // This *must* be called for correct keyboard behavior
    matrix_scan_kb();

    return matrix_has_changed;
}

//MARK: Translate mm
// At initialization, translate the trigger height etc into the corresponding ADC values
void translate_mm_to_value(uint8_t index) {
    uint16_t bottom_value = he_matrix[index].bottom_value;
    uint16_t top_value = he_matrix[index].top_value;

    #if INVERT_ADC == FALSE
    // ADC count per mm of travel
    uint16_t travel_unit = floor((top_value - bottom_value) / TRAVEL_DISTANCE);
    #if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
    // he_matrix[index].trigger_value = travel_unit * trigger_height[index] + bottom_value;
    // he_matrix[index].release_value = travel_unit * release_height[index] + bottom_value;
    he_matrix[index].trigger_value = top_value - (travel_unit * trigger_height[index]);
    he_matrix[index].release_value = top_value - (travel_unit * release_height[index]);
    #endif
    #else //Inverted ADC -> Lower switch means higher value
    uint16_t travel_unit = floor((bottom_value - top_value) / TRAVEL_DISTANCE);
    #if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
    he_matrix[index].trigger_value = top_value + (travel_unit * trigger_height[index]);
    he_matrix[index].release_value = top_value + (travel_unit * release_height[index]);
    #endif
    #endif //else INVERT ADC == TRUE
    #if RAPID_TRIGGER_TYPE != NONE
    he_matrix[index].rt_press_value = travel_unit * rt_press_distance[index];
    he_matrix[index].rt_release_value = travel_unit * rt_release_distance[index];
    #endif
}

//MARK: Get calibration
bool get_calibration_data(void) {
#   if defined NO_EEPROM
    const uint16_t top_values[] = TOP_VALUES;
    const uint16_t bottom_values[] = BOTTOM_VALUES;

    for(uint8_t key = 0; key < SWITCH_NUM; key++) {
        he_matrix[key].top_value = top_values[key];
        he_matrix[key].bottom_value = bottom_values[key];
    }

    return true;
    // return false;
#   else //if defined NO_EEPROM
    //TODO: Add eeprom support
    // Read the calibration data from EEPROM

    // Check if the data aligns with configured matrix size

    // Start calibration if necessary
    calibrate_switches();
#   endif //else defined NO_EEPROM
}

//MARK: Calibrate
void calibrate_switches(void) {
    uint16_t adc_value;
    uint8_t matrix_index;
    // Save config values after x scans without a change
    uint32_t scans_without_change = 0;

    while(true) {
        for(uint8_t mux_channel = 0; mux_channel < MUX_CHANNELS; mux_channel++) {
            #if DEBUG_CALIBRATION == TRUE || DEBUG_SCAN_VALUE == TRUE
            dprintf("Mux: %i\n", mux_channel);
            #endif
            #if POWER_BEFORE_SCAN == TRUE
            gpio_write_pin_high(power_pins[mux_channel]);
            //TODO: Add delay = POWER_DELAY_US;
            #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
            sensor_power_high_kb(mux_channel);
            #endif
            set_mux_channel(mux_channel);
            for(uint8_t adc_channel = 0; adc_channel < ADC_PIN_NUM; adc_channel++) {
                // Translate matrix mux and adc channels to matrix position
                matrix_index = mux_to_num[mux_channel][adc_channel];
                // Check if a switch is at the position (matrix index > 0), and scan if so
                if(matrix_index > 0){
                    matrix_index -= 1;
                    adc_value = adc_read(adc_pin_mux[adc_channel]);
                    #if DEBUG_CALIBRATION == true || DEBUG_SCAN_VALUE == true
                    dprintf("%i: %i,  ", adc_channel, adc_value);
                    #endif

                    #if INVERT_ADC == TRUE
                    if(adc_value < he_matrix[matrix_index].top_value){
                        he_matrix[matrix_index].top_value = adc_value;
                        scans_without_change = 0;
                    } else if (adc_value > he_matrix[matrix_index].bottom_value) {
                        he_matrix[matrix_index].bottom_value = adc_value;
                        scans_without_change = 0;
                    }
                    #else //if INVERT_ADC == TRUE
                    if(adc_value > he_matrix[matrix_index].top_value){
                        he_matrix[matrix_index].top_value = adc_value;
                        scans_without_change = 0;
                        // dprintf("key %i: new top value %i\n", matrix_index, adc_value);
                    } else if (adc_value < he_matrix[matrix_index].bottom_value) {
                        he_matrix[matrix_index].bottom_value = adc_value;
                        scans_without_change = 0;
                        // dprintf("key %i: new bottom value %i\n", matrix_index, adc_value);
                    }
                    #endif//else INVERT_ADC == TRUE
                }
            }
            #if DEBUG_CALIBRATION == TRUE || DEBUG_SCAN_VALUE == TRUE
            dprint("\n\n");
            #endif
            #if POWER_BEFORE_SCAN == TRUE
            gpio_write_pin_low(power_pins[mux_channel]);
            #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
            sensor_power_low_kb(mux_channel);
            #endif
        }
        scans_without_change += 1;
        // Default is set to 10000, so 5s assuming 2000 scans per s?
        if(scans_without_change > SCANS_WITHOUT_CHANGE){
            //TODO: Save calibration data to eeprom


            scans_without_change = 0;
            // break;
        }
    }
}

//MARK: Evaluate
//TODO: Dynamically change type of arg based on matrix size
// Create a matrix_index_t enum with nested #if statements checking the matrix sizes
static inline bool evaluate_value(uint8_t index, uint16_t value) {
//TODO: Evtl remake this to be changeable at runtime (if performance is enough)
//Basically just check a var in the function call to check which to call, if via is not defined it'll be const
    bool prev_pressed = he_matrix[index].pressed;
#if INVERT_ADC == TRUE
#   if RAPID_TRIGGER_TYPE == RAPID_TRIGGER
    // Rapid trigger is only active when the switch is lower than the trigger and release height
    if(value > he_matrix[index].trigger_value + ADC_SMOOTHING) {
        // Set the new lowest value if needed
        if(value > he_matrix[index].rt_press_threshold + ADC_SMOOTHING) {
            he_matrix[index].pressed = true;
            he_matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value + he_matrix[index].rt_press_threshold + ADC_SMOOTHING) < he_matrix[index].rt_release_value) {
            he_matrix[index].pressed = false;
            // Set the new activation threshold
            he_matrix[index].rt_press_threshold = value + he_matrix[index].rt_press_value;
        }
    //TODO: Check if it is faster to check if the key state is released, and only set values if they're not set already
    } else if(value + ADC_SMOOTHING < he_matrix[index].release_value) {
        // If the switch is not pressed past the threshold, reset it
        he_matrix[index].pressed = false;
        he_matrix[index].rt_press_threshold = he_matrix[index].trigger_value;
    }

#   elif RAPID_TRIGGER_TYPE == CONTINUOUS_RAPID_TRIGGER
    static bool rt_active = false;
    // Rapid trigger activates below the trigger height, but only stops when fully released
    if(rt_active || (value > he_matrix[index].trigger_value + ADC_SMOOTHING)) {
        rt_active = true;
        // Set the new lowest value if needed
        if(value > he_matrix[index].rt_press_threshold + ADC_SMOOTHING) {
            he_matrix[index].pressed = true;
            he_matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value + he_matrix[index].rt_press_threshold + ADC_SMOOTHING) < he_matrix[index].rt_release_value) {
            he_matrix[index].pressed = false;
            // Set the new activation threshold
            he_matrix[index].rt_press_threshold = value + he_matrix[index].rt_press_value;
        }
    // Check if the switch has been released completely
    } else if(value < he_matrix[index].top_value + ADC_DEADZONE) {
        he_matrix[index].pressed = false;
        he_matrix[index].rt_press_threshold = he_matrix[index].trigger_value;
        rt_active = false;
    }

#   elif RAPID_TRIGGER_TYPE == CONSTANT_RAPID_TRIGGER
    // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
    if((value > he_matrix[index].rt_press_threshold + ADC_SMOOTHING) ||
        value > he_matrix[index].bottom_value - ADC_DEADZONE) {
        he_matrix[index].pressed = true;
        he_matrix[index].rt_press_threshold = value;
    // Check if the key has been released past the threshold
    } else if((value + he_matrix[index].rt_press_threshold + ADC_SMOOTHING) < he_matrix[index].rt_release_value) {
        he_matrix[index].pressed = false;
        // Set the new activation threshold
        he_matrix[index].rt_press_threshold = value + he_matrix[index].rt_press_value;
    // Check if the key has been completely released
    } else if(value < he_matrix[index].top_value + ADC_DEADZONE) {
        he_matrix[index].pressed = false;
        he_matrix[index].rt_press_threshold = value + he_matrix[index].rt_press_value;
    }

#   else //No rapid trigger
    if(value > he_matrix[index].trigger_value + ADC_SMOOTHING) {
        he_matrix[index].pressed = true;
    } else if(value + ADC_SMOOTHING < he_matrix[index].release_value) {
        he_matrix[index].pressed = false;
    } else {
        return false;
    }

#   endif //defined RAPID_TRIGGER

#else //defined INVERT_ADC
#   if RAPID_TRIGGER_TYPE == RAPID_TRIGGER
    // Rapid trigger is only active when the switch is lower than the trigger and release height
    if(value < he_matrix[index].trigger_value - ADC_SMOOTHING) {
        // Set the new lowest value if needed
        if(value < he_matrix[index].rt_press_threshold - ADC_SMOOTHING) {
            he_matrix[index].pressed = true;
            he_matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value - he_matrix[index].rt_press_threshold - ADC_SMOOTHING) > he_matrix[index].rt_release_value) {
            he_matrix[index].pressed = false;
            // Set the new activation threshold
            he_matrix[index].rt_press_threshold = value - he_matrix[index].rt_press_value;
        }
    //TODO: Check if it is faster to check if the key state is released, and only set values if they're not set already
    } else if(value - ADC_SMOOTHING > he_matrix[index].release_value) {
        // If the switch is not pressed past the threshold, reset it
        he_matrix[index].pressed = false;
        he_matrix[index].rt_press_threshold = he_matrix[index].trigger_value;
    }

#   elif RAPID_TRIGGER_TYPE == CONTINUOUS_RAPID_TRIGGER
    static bool rt_active = false;
    // Rapid trigger activates below the trigger height, but only stops when fully released
    if(rt_active || (value < he_matrix[index].trigger_value - ADC_SMOOTHING)) {
        rt_active = true;
        // Set the new lowest value if needed
        if(value < he_matrix[index].rt_press_threshold - ADC_SMOOTHING) {
            he_matrix[index].pressed = true;
            he_matrix[index].rt_press_threshold = value;
        // Check if the key has been released past the threshold
        } else if((value - he_matrix[index].rt_press_threshold - ADC_SMOOTHING) > he_matrix[index].rt_release_value) {
            he_matrix[index].pressed = false;
            // Set the new activation threshold
            he_matrix[index].rt_press_threshold = value - he_matrix[index].rt_press_value;
        }
    // Check if the switch has been released completely
    } else if(value > he_matrix[index].top_value - ADC_DEADZONE) {
        he_matrix[index].pressed = false;
        he_matrix[index].rt_press_threshold = he_matrix[index].trigger_value;
        rt_active = false;
    }

#   elif RAPID_TRIGGER_TYPE == CONSTANT_RAPID_TRIGGER
    // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
    if((value < he_matrix[index].rt_press_threshold - ADC_SMOOTHING) ||
        value < he_matrix[index].bottom_value + ADC_DEADZONE) {
        he_matrix[index].pressed = true;
        he_matrix[index].rt_press_threshold = value;
    // Check if the key has been released past the threshold
    } else if((value - he_matrix[index].rt_press_threshold - ADC_SMOOTHING) > he_matrix[index].rt_release_value) {
        he_matrix[index].pressed = false;
        // Set the new activation threshold
        he_matrix[index].rt_press_threshold = value - he_matrix[index].rt_press_value;
    // Check if the key has been completely released
    } else if(value > he_matrix[index].top_value - ADC_DEADZONE) {
        he_matrix[index].pressed = false;
        he_matrix[index].rt_press_threshold = value - he_matrix[index].rt_press_value;
    }

#   else //No rapid trigger
    if(value < he_matrix[index].trigger_value - ADC_SMOOTHING) {
        he_matrix[index].pressed = true;
    } else if(value - ADC_SMOOTHING > he_matrix[index].release_value) {
        he_matrix[index].pressed = false;
    } else {
        return false;
    }

#   endif //defined RAPID_TRIGGER
#endif //else defined INVERT_ADC
    return !(prev_pressed == he_matrix[index].pressed);
}

//MARK: Num to Matrix
static inline void translate_num_to_matrix(matrix_row_t current_matrix[], uint8_t index) {
    uint8_t row = num_to_matrix[index][0];
    uint8_t col = num_to_matrix[index][1];

    current_matrix[row] ^= 1 << col;
}

/* Weak defines, to allow a custom powering logic */
__attribute__((weak)) void sensor_power_init_kb(void) { sensor_power_init_user(); }

__attribute__((weak)) void sensor_power_high_kb(uint8_t mux_channel) { sensor_power_high_user(mux_channel); }

__attribute__((weak)) void sensor_power_low_kb(uint8_t mux_channel) { sensor_power_low_user(mux_channel); }

__attribute__((weak)) void sensor_power_init_user(void) {}

__attribute__((weak)) void sensor_power_high_user(uint8_t mux_channel) {}

__attribute__((weak)) void sensor_power_low_user(uint8_t mux_channel) {}





/* Standard weak defines */
__attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }

__attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }

__attribute__((weak)) void matrix_init_user(void) {}

__attribute__((weak)) void matrix_scan_user(void) {}
