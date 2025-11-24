#include "matrix.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/cdefs.h>
#include "action_layer.h"
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
#include "print.h"
#include "stm32_gpio.h"

// Get the switch data configured in the json
static void get_switch_data(void);
// Checks which values need to be translated and calls translation function with the correct args
void translate_key_values(void);
// Translate the user defined trigger height etc into the equivalent ADC values
void translate_mm_to_value(uint8_t index);
// Gets the previously calibrated min/max values for each switch from the EEPROM
//TODO: Allow saving to flash
bool get_calibration_data(void);
// Saves the newly calibrated min/max values to the EEPROM
bool save_calibration_data(void);
// Runs a full keyboard calibration to get the ADC values for the bottom out and unpressed positions
void calibrate_switches(void);
// Checks if the switch is pressed or not, returns true if the state has changed
//TODO: Dynamically change type of arg based on matrix size
//      Create a matrix_index_t enum with nested #if statements checking the matrix sizes
static inline bool evaluate_value(uint8_t index, uint16_t value);
// Empty loop for short delays
static inline void delay_ns(uint16_t delay);

uint8_t current_he_profile = HE_DEFAULT_PROFILE;

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
    #if POWER_BEFORE_SCAN == TRUE || POWER_BEFORE_SCAN_MIDDLE == TRUE
    for(uint8_t i = 0; i < POWER_PIN_NUM; i++) {
        gpio_set_pin_output_push_pull(power_pins[i]);
        gpio_write_pin_low(power_pins[i]);
    }
    #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
    sensor_power_init_kb();
    #endif
    //TODO: Remove this after testing, only is an issue since qmk sets all unused pins to high,
    //      and I have all mux select pins connected
    GPIOB->MODER = 0b01010101010101010101010101010101;
    GPIOB->OTYPER = 0x0000;
    GPIOB->OSPEEDR = 0b10101010101010101010101010101010;
    GPIOB->ODR = 0x0000;

    get_switch_data();

    // TODO: Load key matrix struct with calibration and distance data from the EEPROM
    // Get the min/max values of each switch from EEPROM
    if(!get_calibration_data()) {
    // If loading the calibration data fails, start calibration immediately
        calibrate_switches();
        // Check calibration key, if it is pressed down at this point then start calibration
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
    uint8_t index;

    for(uint8_t mux_channel = 0; mux_channel < MUX_CHANNELS; mux_channel++) {
        #if POWER_BEFORE_SCAN == TRUE
        gpio_write_pin_high(power_pins[mux_channel]);
        #ifdef POWER_SELECT_DELAY
        delay_ns(POWER_SELECT_CYCLES);
        #endif
        #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
        sensor_power_high_kb(mux_channel);
        #endif
        set_mux_channel(mux_channel);
        #ifdef MUX_SELECT_DELAY
        delay_ns(MUX_SELECT_CYCLES);
        #endif

        for(uint8_t adc_channel = 0; adc_channel < ADC_PIN_NUM; adc_channel++) {
            // Translate matrix mux and adc channels to matrix position
            index = mux_to_num[mux_channel][adc_channel];
            // Check if a switch is at the position (matrix index > 0), and scan if so
            if(index > 0){
                index -= 1;
                adc_value = adc_read(adc_pin_mux[adc_channel]);
                #if DEBUG_SCAN_VALUES == TRUE
                dprintf("%2i/%2i: %3i,  ",mux_channel, adc_channel, adc_value);
                #endif
                #ifdef ADC_SCAN_DELAY
                delay_ns(ADC_SCAN_CYCLES);
                #endif

                // Check if key is pressed/released and set matrix_has_changed, returns true if the switch state has changed
                if(evaluate_value(index, adc_value)) {
                    matrix_has_changed = true;
                    #if DEBUG_SCAN_NO_INPUT == FALSE
                    current_matrix[he_matrix[index].row] ^= 1 << he_matrix[index].col;
                    #endif
                }
            }
            #if DEBUG_SCAN_VALUES == TRUE
            else {
                dprintf("%2i/XX: XXX,  ", mux_channel);
            }
            #endif
        }
        #if DEBUG_SCAN_VALUES == TRUE
        dprint("\n");
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


//MARK: Translate
// Translate the trigger height etc of one key into the corresponding ADC values
void translate_mm_to_value(uint8_t index) {
    uint16_t bottom_value = he_matrix[index].bottom_value;
    uint16_t top_value = he_matrix[index].top_value;

    #if INVERT_ADC == FALSE
    // ADC count per mm of travel
    uint16_t travel_unit = floor((top_value - bottom_value) / TRAVEL_DISTANCE);

    for(uint8_t profile = 0; profile < HE_PROFILE_NUM; profile++) {
    #   if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
        #if DISTANCE_FROM_BOTTOM == FALSE
        he_matrix[index].trigger_value[profile] = top_value - (travel_unit * trigger_height[profile][index]);
        he_matrix[index].release_value[profile] = top_value - (travel_unit * release_height[profile][index]);

        #else // if DISTANCE_FROM_BOTTOM == FALSE
        he_matrix[index].trigger_value[profile] = travel_unit * trigger_height[profile][index] + bottom_value;
        he_matrix[index].release_value[profile] = travel_unit * release_height[profile][index] + bottom_value;
        #endif // else DISTANCE_FROM_BOTTOM == FALSE
        #endif // if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER

        #else //Inverted ADC -> Lower switch means higher value
        uint16_t travel_unit = floor((bottom_value - top_value) / TRAVEL_DISTANCE);

    #   if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
        #if DISTANCE_FROM_BOTTOM == FALSE
        he_matrix[index].trigger_value[profile] = top_value + (travel_unit * trigger_height[profile][index]);
        he_matrix[index].release_value[profile] = top_value + (travel_unit * release_height[profile][index]);

        #else // if DISTANCE_FROM_BOTTOM == FALSE
        he_matrix[index].trigger_value[profile] = bottom_value - (travel_unit * trigger_height[profile][index]);
        he_matrix[index].release_value[profile] = bottom_value - (travel_unit * release_height[profile][index]);
        #endif // else DISTANCE_FROM_BOTTOM == FALSE
        #endif // if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
        #endif //else INVERT ADC == TRUE

    #   if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
        he_matrix[index].rt_press_value[profile] = travel_unit * rt_press_distance[profile][index];
        he_matrix[index].rt_release_value[profile] = travel_unit * rt_release_distance[profile][index];
        #endif
    }
}


//MARK: Get calibration
bool get_calibration_data(void) {
#   if HE_NO_EEPROM == TRUE
    #ifdef HE_TOP_VALUES
    const uint16_t top_values[] = HE_TOP_VALUES;
    #else
    //TODO: Add proper checks and instructions here
    #   error "hall_effect.config.top_values needs to be defined!"
    #endif
    #ifdef HE_BOTTOM_VALUES
    const uint16_t bottom_values[] = HE_BOTTOM_VALUES;
    #else
    #   error "hall_effect.config.bottom_values needs to be defined!"
    #endif

    for(uint8_t key = 0; key < SWITCH_NUM; key++) {
        he_matrix[key].top_value = top_values[key];
        he_matrix[key].bottom_value = bottom_values[key];
    }

    return true;
    // return false;
#   else //if HE_NO_EEPROM == TRUE
    //TODO: Add eeprom support
    // Read the calibration data from EEPROM

    // Check if the data aligns with configured matrix size

    // Start calibration if necessary
    calibrate_switches();
    return true;
#   endif //else HE_NO_EEPROM == TRUE
}


//MARK: Calibrate
void calibrate_switches(void) {
    uint16_t adc_value;
    uint8_t matrix_index;
    // Save config values after x scans without a change
    uint32_t scans_without_change = 0;
    bool not_first_scan[MUX_CHANNELS][ADC_PIN_NUM];

    while(true) {
        for(uint8_t mux_channel = 0; mux_channel < MUX_CHANNELS; mux_channel++) {
            #if DEBUG_CALIBRATION == TRUE || DEBUG_SCAN_VALUES == TRUE
            dprintf("Mux: %i\n", mux_channel);
            #endif
            #if POWER_BEFORE_SCAN == TRUE
            gpio_write_pin_high(power_pins[mux_channel]);
            #ifdef POWER_SELECT_DELAY
            delay_ns(POWER_SELECT_CYCLES);
            #endif
            #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
            sensor_power_high_kb(mux_channel);
            #endif
            set_mux_channel(mux_channel);
            #ifdef MUX_SELECT_DELAY
            delay_ns(MUX_SELECT_CYCLES);
            #endif

            for(uint8_t adc_channel = 0; adc_channel < ADC_PIN_NUM; adc_channel++) {
                // Translate matrix mux and adc channels to matrix position
                matrix_index = mux_to_num[mux_channel][adc_channel];
                // Check if a switch is at the position (matrix index > 0), and scan if so
                if(matrix_index > 0){
                    matrix_index -= 1;
                    adc_value = adc_read(adc_pin_mux[adc_channel]);
                    #if DEBUG_CALIBRATION == true || DEBUG_SCAN_VALUES == true
                    dprintf("%i: %i,  ", adc_channel, adc_value);
                    #endif

                    if(!not_first_scan[mux_channel][adc_channel]) {
                        he_matrix[matrix_index].top_value = adc_value;
                        he_matrix[matrix_index].bottom_value = adc_value;
                        not_first_scan[mux_channel][adc_channel] = true;
                        continue;
                    }

                    #if INVERT_ADC == TRUE
                    if(adc_value < he_matrix[matrix_index].top_value){
                        he_matrix[matrix_index].top_value = adc_value;
                        scans_without_change = 0;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new top value %i\n", matrix_index, adc_value);
                        #endif
                    } else if (adc_value > he_matrix[matrix_index].bottom_value) {
                        he_matrix[matrix_index].bottom_value = adc_value;
                        scans_without_change = 0;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new bottom value %i\n", matrix_index, adc_value);
                        #endif
                    }
                    #else //if INVERT_ADC == TRUE
                    if(adc_value > he_matrix[matrix_index].top_value){
                        he_matrix[matrix_index].top_value = adc_value;
                        scans_without_change = 0;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new top value %i\n", matrix_index, adc_value);
                        #endif
                    } else if (adc_value < he_matrix[matrix_index].bottom_value) {
                        he_matrix[matrix_index].bottom_value = adc_value;
                        scans_without_change = 0;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new bottom value %i\n", matrix_index, adc_value);
                        #endif
                    }
                    #endif//else INVERT_ADC == TRUE
                }
            }
            #if DEBUG_CALIBRATION == TRUE || DEBUG_SCAN_VALUES == TRUE
            dprint("\n");
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
            #if HE_NO_EEPROM == FALSE
            #else //NO_EEPROM == FALSE
            // Print the calibration values of each switch so that they can be adjusted in the config
            print("\"top_values\": [ ");
            for(uint8_t index = 0; index < SWITCH_NUM; index++){
                if(index < SWITCH_NUM - 1){
                    printf("%u, ", he_matrix[index].top_value);
                }else{
                    printf("%u ],\n", he_matrix[index].top_value);
                }
            }
            print("\"bottom_values\": [ ");
            for(uint8_t index = 0; index < SWITCH_NUM; index++){
                // dprintf("%2u | %3u | %3u\n", index, he_matrix[index].top_value, he_matrix[index].bottom_value);
                if(index < SWITCH_NUM - 1){
                    printf("%u, ", he_matrix[index].bottom_value);
                }else{
                    printf("%u ]\n", he_matrix[index].bottom_value);
                }
            }
            print("You can paste these lines into keyboard.json\n");
            #endif //else NO_EEPROM == FALSE

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
    if(he_matrix[index].rt_type[current_he_profile]) {
        switch(profiles[current_he_profile].rt_type) {
            case rapid_trigger:
#           if defined USE_RAPID_TRIGGER
            // Rapid trigger is only active when the switch is lower than the trigger and release height
            if(value > he_matrix[index].trigger_value + ADC_SMOOTHING) {
                // Set the new lowest value if needed
                if(value > he_matrix[index].rt_press_threshold + ADC_SMOOTHING) {
                    he_matrix[index].pressed = true;
                    he_matrix[index].rt_press_threshold = value;
                // Check if the key has been released past the threshold
                } else if((value + he_matrix[index].rt_press_threshold + ADC_SMOOTHING) < he_matrix[index].rt_release_value[current_he_profile]) {
                    he_matrix[index].pressed = false;
                    // Set the new activation threshold
                    he_matrix[index].rt_press_threshold = value + he_matrix[index].rt_press_value[current_he_profile];
                }
            } else if(value + ADC_SMOOTHING < he_matrix[index].release_value[current_he_profile]) {
                // If the switch is not pressed past the threshold, reset it
                he_matrix[index].pressed = false;
                he_matrix[index].rt_press_threshold = he_matrix[index].trigger_value[current_he_profile];
            }
            break;
#           endif
            case continuous_rapid_trigger:

#           if defined USE_CONTINUOUS_RAPID_TRIGGER
            static bool rt_active[SWITCH_NUM] = {[0 ... SWITCH_NUM-1] = false };
            // Rapid trigger activates below the trigger height, but only stops when fully released
            if(rt_active[index] || (value > he_matrix[index].trigger_value[current_he_profile] + ADC_SMOOTHING)) {
                rt_active[index] = true;
                // Set the new lowest value if needed
                if(value > he_matrix[index].rt_press_threshold + ADC_SMOOTHING) {
                    he_matrix[index].pressed = true;
                    he_matrix[index].rt_press_threshold = value;
                // Check if the key has been released past the threshold
                } else if((value + he_matrix[index].rt_press_threshold + ADC_SMOOTHING) < he_matrix[index].rt_release_value[current_he_profile]) {
                    he_matrix[index].pressed = false;
                    // Set the new activation threshold
                    he_matrix[index].rt_press_threshold = value + he_matrix[index].rt_press_value[current_he_profile];
                }
            // Check if the switch has been released completely
            } else if(value < he_matrix[index].top_value + ADC_DEADZONE) {
                he_matrix[index].pressed = false;
                he_matrix[index].rt_press_threshold = he_matrix[index].trigger_value[current_he_profile];
                rt_active[index] = false;
            }
            break;
#           endif
            case constant_rapid_trigger:

#           if defined USE_CONSTANT_RAPID_TRIGGER
            // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
            if((value > he_matrix[index].rt_press_threshold + ADC_SMOOTHING) ||
                value > he_matrix[index].bottom_value - ADC_DEADZONE) {
                he_matrix[index].pressed = true;
                he_matrix[index].rt_press_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value + he_matrix[index].rt_press_threshold + ADC_SMOOTHING) < he_matrix[index].rt_release_value[current_he_profile]) {
                he_matrix[index].pressed = false;
                // Set the new activation threshold
                he_matrix[index].rt_press_threshold = value + he_matrix[index].rt_press_value[current_he_profile];
            // Check if the key has been completely released
            } else if(value < he_matrix[index].top_value + ADC_DEADZONE) {
                he_matrix[index].pressed = false;
                he_matrix[index].rt_press_threshold = value + he_matrix[index].rt_press_value[current_he_profile];
            }
            break;
#           endif
            case none:

#           if defined USE_NONE
            if(value > he_matrix[index].trigger_value[current_he_profile] + ADC_SMOOTHING) {
                he_matrix[index].pressed = true;
            } else if(value + ADC_SMOOTHING < he_matrix[index].release_value[current_he_profile]) {
                he_matrix[index].pressed = false;
            } else {
                return false;
            }
            break;
#           endif //defined RAPID_TRIGGER
        }
    } else { // rt_type[current_he_profile]
#   if defined USE_NONE
        if(value < he_matrix[index].trigger_value[current_he_profile] - ADC_SMOOTHING) {
            he_matrix[index].pressed = true;
        } else if(value - ADC_SMOOTHING > he_matrix[index].release_value[current_he_profile]) {
            he_matrix[index].pressed = false;
        } else {
            return false;
        }
#   endif //defined USE_NONE
    }

#else //defined INVERT_ADC
// #   if RAPID_TRIGGER_TYPE == RAPID_TRIGGER
    //TODO: The current implementation here uses one mode per profile
    if(he_matrix[index].rt_type[current_he_profile]) {
        switch(profiles[current_he_profile].rt_type) {
            case rapid_trigger:
#           if defined USE_RAPID_TRIGGER
            // Rapid trigger is only active when the switch is lower than the trigger and release height
            if(value < he_matrix[index].trigger_value[current_he_profile] - ADC_SMOOTHING) {
                // Set the new lowest value if needed
                if(value < he_matrix[index].rt_press_threshold - ADC_SMOOTHING) {
                    he_matrix[index].pressed = true;
                    he_matrix[index].rt_press_threshold = value;
                // Check if the key has been released past the threshold
                } else if((value - he_matrix[index].rt_press_threshold - ADC_SMOOTHING) > he_matrix[index].rt_release_value[current_he_profile]) {
                    he_matrix[index].pressed = false;
                    // Set the new activation threshold
                    he_matrix[index].rt_press_threshold = value - he_matrix[index].rt_press_value[current_he_profile];
                }
            //TODO: Check if it is faster to check if the key state is released, and only set values if they're not set already
            } else if(value - ADC_SMOOTHING > he_matrix[index].release_value[current_he_profile]) {
                // If the switch is not pressed past the threshold, reset it
                he_matrix[index].pressed = false;
                he_matrix[index].rt_press_threshold = he_matrix[index].trigger_value[current_he_profile];
            }
            break;
#           endif // defined USE_RAPID_TRIGGER

            case continuous_rapid_trigger:
#           if defined USE_CONTINUOUS_RAPID_TRIGGER
            static bool rt_active[SWITCH_NUM] = {[0 ... SWITCH_NUM-1] = false };
            // Rapid trigger activates below the trigger height, but only stops when fully released
            if(rt_active[index] || (value < he_matrix[index].trigger_value[current_he_profile] - ADC_SMOOTHING)) {
                rt_active[index] = true;
                // Set the new lowest value if needed
                if(value < he_matrix[index].rt_press_threshold - ADC_SMOOTHING) {
                    he_matrix[index].pressed = true;
                    he_matrix[index].rt_press_threshold = value;
                // Check if the key has been released past the threshold
                } else if((value - he_matrix[index].rt_press_threshold - ADC_SMOOTHING) > he_matrix[index].rt_release_value[current_he_profile]) {
                    he_matrix[index].pressed = false;
                    // Set the new activation threshold
                    he_matrix[index].rt_press_threshold = value - he_matrix[index].rt_press_value[current_he_profile];
                }
            // Check if the switch has been released completely
            } else if(value > he_matrix[index].top_value - ADC_DEADZONE) {
                he_matrix[index].pressed = false;
                he_matrix[index].rt_press_threshold = he_matrix[index].trigger_value[current_he_profile];
                rt_active[index] = false;
            }
            break;
#           endif // defined USE_CONTINUOUS_RAPID_TRIGGER

            case constant_rapid_trigger:
#           if defined USE_CONSTANT_RAPID_TRIGGER
            // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
            if((value < he_matrix[index].rt_press_threshold - ADC_SMOOTHING) ||
                value < he_matrix[index].bottom_value + ADC_DEADZONE) {
                he_matrix[index].pressed = true;
                he_matrix[index].rt_press_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value - he_matrix[index].rt_press_threshold - ADC_SMOOTHING) > he_matrix[index].rt_release_value[current_he_profile]) {
                he_matrix[index].pressed = false;
                // Set the new activation threshold
                he_matrix[index].rt_press_threshold = value - he_matrix[index].rt_press_value[current_he_profile];
            // Check if the key has been completely released
            } else if(value > he_matrix[index].top_value - ADC_DEADZONE) {
                he_matrix[index].pressed = false;
                he_matrix[index].rt_press_threshold = value - he_matrix[index].rt_press_value[current_he_profile];
            }
            break;
#           endif // defined USE_CONSTANT_RAPID_TRIGGER

            case none:
#   if defined USE_NONE
            if(value < he_matrix[index].trigger_value[current_he_profile] - ADC_SMOOTHING) {
                he_matrix[index].pressed = true;
            } else if(value - ADC_SMOOTHING > he_matrix[index].release_value[current_he_profile]) {
                he_matrix[index].pressed = false;
            } else {
                return false;
            }
            break;
#   endif //defined USE_NONE
        }
    } else { // rt_type[current_he_profile]
#   if defined USE_NONE
        if(value < he_matrix[index].trigger_value[current_he_profile] - ADC_SMOOTHING) {
            he_matrix[index].pressed = true;
        } else if(value - ADC_SMOOTHING > he_matrix[index].release_value[current_he_profile]) {
            he_matrix[index].pressed = false;
        } else {
            return false;
        }
#   endif //defined USE_NONE
    }
#endif //else defined INVERT_ADC
    return !(prev_pressed == he_matrix[index].pressed);
}


//MARK: Switch data
// Populates the key matrix with the static config params, heights are populated separately
static void get_switch_data(void) {
    for(uint8_t key = 0; key < SWITCH_NUM; key++) {
        uint8_t row = num_to_matrix[key][0];
        uint8_t col = num_to_matrix[key][1];
        he_matrix[key].row = row;
        he_matrix[key].col = col;
        #define USE_RAPID_TRIGGER
        #if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
        //TODO: Check if this works
        // Use 2*col if setting mode is allowed per key, if one mode per profile then use col and smaller type
        // he_matrix[key].rt_type = (profiles[HE_DEFAULT_PROFILE].rt_mask[row] & (3 << (2*col)));
        // he_matrix[key].rt_type[HE_DEFAULT_PROFILE] = (profiles[HE_DEFAULT_PROFILE].rt_mask[key/16] & (1 << key%16));
        if(profiles[HE_DEFAULT_PROFILE].rt_mask[key/16] & (1 << (key % 16))) {
            he_matrix[key].rt_type[HE_DEFAULT_PROFILE] = 1;
        }
        // } else {
        //     he_matrix[key].rt_type[HE_DEFAULT_PROFILE] = 0;
        // }
        #endif
    }
}


//MARK: Delay
//TODO: Scale this so that it's roughly correct
static inline void delay_ns(uint16_t delay) {
    for(; delay > 0; delay--){
        __asm("");
    }
}


//MARK: Profiles
#if HE_PROFILE_NUM > 1
void switch_to_profile(uint8_t profile) {
    // Keep these in case I switch to the replacement method later
    // if(profile == current_he_profile) { return; }

    // for(uint8_t key = 0; key < SWITCH_NUM; key++){
    //     //TODO: Add per key rapid trigger here as well
    //     #if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
    //     he_matrix[key].trigger_value = trigger_value[profile][key];
    //     he_matrix[key].release_value = release_value[profile][key];
    //     #endif
    //     #if RAPID_TRIGGER_TYPE != NONE
    //     he_matrix[key].rt_press_value = rt_press_value[profile][key];
    //     he_matrix[key].rt_release_value = rt_release_value[profile][key];
    //     #endif
    // }
    current_he_profile = profile;
}


//MARK: Layer state
//TODO: Update this
layer_state_t layer_state_set_kb(layer_state_t state) {
    uint8_t highest_layer = get_highest_layer(state);
    for(uint8_t profile = 0; profile < HE_PROFILE_NUM; profile++) {
        // Profile layers is a bitmap, where a 1 means that the profile should be used if on that layer
        //TODO: Make this a thing, use python
        // if(profile_layers[profile] & (1 << highest_layer)){
        //     // A check if the profile is active already happens in the function, since it can be called from elsewhere
        //     switch_to_profile(profile);
        // }
        // If the highest active layer is in the layers list of that profile, activate it
        if(profiles[profile].layers & (1 << highest_layer)) {
            switch_to_profile(profile);
            // Only the lowest number profile should apply
            break;
        }
    }


    // Need to return state for it to work correctly
    return state;
}


//MARK: Get profile
uint8_t get_current_profile(void) { return current_he_profile; }
#endif // if HE_PROFILE_NUM > 1



/* Weak defines, to allow a custom powering logic */
__attribute__((weak)) void sensor_power_init_kb(void) { sensor_power_init_user(); }

__attribute__((weak)) void sensor_power_high_kb(uint8_t mux_channel) { sensor_power_high_user(mux_channel); }

__attribute__((weak)) void sensor_power_low_kb(uint8_t mux_channel) { sensor_power_low_user(mux_channel); }

__attribute__((weak)) void sensor_power_toggle_kb(uint8_t mux_channel, uint8_t adc_channel) { sensor_power_toggle_user(mux_channel, adc_channel); }

__attribute__((weak)) void sensor_power_init_user(void) {}

__attribute__((weak)) void sensor_power_high_user(uint8_t mux_channel) {}

__attribute__((weak)) void sensor_power_low_user(uint8_t mux_channel) {}

__attribute__((weak)) void sensor_power_toggle_user(uint8_t mux_channel, uint8_t adc_channel) {}





/* Standard weak defines */
__attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }

__attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }

__attribute__((weak)) void matrix_init_user(void) {}

__attribute__((weak)) void matrix_scan_user(void) {}
