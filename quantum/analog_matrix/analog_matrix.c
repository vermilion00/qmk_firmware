//TODO: Go through these and check which ones are needed

#include "analog_matrix.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
// #include <sys/cdefs.h>
// #include "_wait.h
// #include "analog.h"
#include "bootloader.h"
#include "debug.h"
#include "info_config.h"
#include "joystick.h"
#include "keyboard.h"
#include "keymap_introspection.h"
#include "matrix.h"
#include "multiplexer.h"
#include "nvm_eeconfig.h"
#include "print.h"
#include "suspend.h"
//TODO: Debouncing doesn't work currently, fix it just in case
#if DEBOUNCE > 0
#include "debounce.h"
#endif
#ifdef JOYSTICK_ENABLE
#include "analog_joystick.h"
#endif
#ifdef MIDI_ENABLE
#include "analog_midi.h"
#endif
//TODO: I think either this or nvm_eeconfig is unnecessary
#if AM_NO_EEPROM == FALSE
#include "eeconfig.h"
analog_switch_t calibration_data[MAX(SWITCH_NUM, SWITCH_NUM_R)];
#endif


// Get the switch data configured in the json
void get_switch_data(void);
// Translate the user defined trigger height etc into the equivalent ADC values
void translate_mm_to_value(void);
// Gets the previously calibrated min/max values for each switch from the EEPROM
//TODO: Allow saving to flash
bool get_calibration_data(void);
// Runs a full keyboard calibration to get the ADC values for the bottom out and unpressed positions
void calibrate_switches(void);
// Checks if the switch is pressed or not, returns true if the state has changed
//TODO: Dynamically change type of arg based on matrix size
//      Create a matrix_index_t enum with nested #if statements checking the matrix sizes
static inline bool evaluate_value(uint8_t index, uint16_t value);
// Empty loop for short delays
static inline void delay_ns(uint16_t delay);

extern matrix_row_t matrix[MATRIX_ROWS];
#if DEBOUNCE > 0
extern matrix_row_t raw_matrix[MATRIX_ROWS];
#endif
#ifdef SPLIT_KEYBOARD
// row offsets for each hand
extern uint8_t thisHand, thatHand;
#endif

#if DYNAMIC_CALIBRATION == TRUE
// Check if the switch boundaries need updating, and update them if necessary.
inline bool update_switch_bounds(uint8_t index, uint16_t value);
#endif

#if AM_INIT_KEY_NUM > 0
// Initialize the keys to be checked at initialization
static void scan_init_keys(void);
SPLIT_MUTABLE uint8_t init_keys[AM_INIT_KEY_NUM][2] = AM_INIT_KEYS;
void (*init_functions[AM_INIT_KEY_NUM])(void) = AM_INIT_FUNCTIONS;
#endif

#if defined DEBUG_MUX_VALUE
uint8_t debug_mux[2] = DEBUG_MUX_VALUE;
#endif

#ifdef POWER_PINS
// Set the sensor power pins and delay, if defined
static inline void set_sensor_power(uint8_t index);
#endif

PROFILE_MUTABLE uint8_t active_profile = AM_DEFAULT_PROFILE;
uint8_t highest_layer = 0;

SPLIT_MUTABLE uint8_t switch_num = SWITCH_NUM;

#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
analog_key_t key_config[MAX(SWITCH_NUM, SWITCH_NUM_R)];
uint8_t mux_to_num[MAX(MUX_CHANNELS, MUX_CHANNELS_R)][ADC_PIN_NUM] = MUX_TO_NUM;
const uint8_t mux_to_num_r[MAX(MUX_CHANNELS, MUX_CHANNELS_R)][ADC_PIN_NUM] = MUX_TO_NUM_R;
uint8_t num_to_matrix[MAX(SWITCH_NUM, SWITCH_NUM_R)][2] = NUM_TO_MATRIX;
const uint8_t num_to_matrix_r[MAX(SWITCH_NUM, SWITCH_NUM_R)][2] = NUM_TO_MATRIX_R;
uint8_t key_modes[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = KEY_MODES;
const uint8_t key_modes_r[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = KEY_MODES_R;

#if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
float trigger_height[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = TRIGGER_HEIGHT;
float release_height[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RELEASE_HEIGHT;
const float trigger_height_r[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = TRIGGER_HEIGHT_R;
const float release_height_r[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RELEASE_HEIGHT_R;
#endif
#if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
float rt_press_distance[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RT_PRESS_DISTANCE;
float rt_release_distance[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RT_RELEASE_DISTANCE;
const float rt_press_distance_r[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RT_PRESS_DISTANCE_R;
const float rt_release_distance_r[AM_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RT_RELEASE_DISTANCE_R;
#endif

//TODO: Currently, different amounts of pins per half is not supported, make sure the larger amount is defined
pin_t adc_pins[ADC_PIN_NUM] = ADC_PINS;
#if !defined EQUAL_ADC_PINS
const pin_t adc_pins_r[ADC_PIN_NUM] = ADC_PINS_R;
#endif
#if defined MUX_PINS
pin_t mux_pins[MUX_PIN_NUM] = MUX_PINS;
#if !defined EQUAL_MUX_PINS
const pin_t mux_pins_r[MUX_PIN_NUM] = MUX_PINS_R;
#endif
#endif
#if defined POWER_PINS
pin_t power_pins[POWER_PIN_NUM] = POWER_PINS;
#if !defined EQUAL_POWER_PINS
const pin_t power_pins_r[POWER_PIN_NUM] = POWER_PINS_R;
#endif
#endif

#if AM_INIT_KEY_NUM > 0
const uint8_t init_keys_r[AM_INIT_KEY_NUM_R][2] = AM_INIT_KEYS_R;
const void (*init_functions_r[AM_INIT_KEY_NUM_R])(void) = AM_INIT_FUNCTIONS_R;
#endif

//TODO: Implement right part of this
#if defined DEBUG_MUX_VALUE_R
uint8_t debug_mux_r[2] = DEBUG_MUX_VALUE_R;
#endif

#ifdef PRIORITY_INDICES_R
const uint8_t priority_indices_r[MAX(SWITCH_NUM, SWITCH_NUM_R)] = PRIORITY_INDICES_R;
const uint8_t priority_index_num_r = PRIORITY_INDEX_NUM_R;
#endif

#else // if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
analog_key_t key_config[SWITCH_NUM];
#ifdef MUX_PINS
SPLIT_MUTABLE pin_t mux_pins[MUX_PIN_NUM] = MUX_PINS;
#endif
SPLIT_MUTABLE pin_t adc_pins[ADC_PIN_NUM] = ADC_PINS;
#ifdef POWER_PINS
SPLIT_MUTABLE pin_t power_pins[POWER_PIN_NUM] = POWER_PINS;
#endif

SPLIT_MUTABLE uint8_t key_modes[AM_PROFILE_NUM][SWITCH_NUM] = KEY_MODES;

// Used to translate from the ADC pin/Mux combination to the switch number
SPLIT_MUTABLE uint8_t mux_to_num[MUX_CHANNELS][ADC_PIN_NUM] = MUX_TO_NUM;
// Used to translate from the switch number to the QMK layout position
SPLIT_MUTABLE uint8_t num_to_matrix[SWITCH_NUM][2] = NUM_TO_MATRIX;

#if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
SPLIT_MUTABLE float trigger_height[AM_PROFILE_NUM][SWITCH_NUM] = TRIGGER_HEIGHT;
SPLIT_MUTABLE float release_height[AM_PROFILE_NUM][SWITCH_NUM] = RELEASE_HEIGHT;
#endif

#if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
SPLIT_MUTABLE float rt_press_distance[AM_PROFILE_NUM][SWITCH_NUM] = RT_PRESS_DISTANCE;
SPLIT_MUTABLE float rt_release_distance[AM_PROFILE_NUM][SWITCH_NUM] = RT_RELEASE_DISTANCE;
#endif
#endif

// During initialization, the adc pins are translated to the adc mux combination that the adc_read function uses
adc_mux adc_pin_mux[ADC_PIN_NUM];

//TODO: Remove this
// #define SPLIT_KEYBOARD
#ifdef SPLIT_KEYBOARD
__attribute((unused)) uint8_t calibration_done = false;
// slave_to_master_t slave_data;

#if KEYBOARD_SIDE == UNKNOWN
bool keyboard_left;
void assign_split_side(bool side);
#endif
#endif // if defined SPLIT_KEYBOARD

//TODO: Assign correct side at init
#ifdef PRIORITY_MUXES
uint8_t scan_amt = 0;
uint8_t priority_muxes[PRIORITY_MUX_NUM][3] = PRIORITY_MUXES;
uint8_t matrix_scan_priority(matrix_row_t current_matrix[]);
#endif

//TODO: Maybe allow assigning them on a per layer basis?
#ifdef PRIORITY_INDICES
uint8_t scan_amt = 0;
uint8_t priority_indices[MAX(SWITCH_NUM, SWITCH_NUM_R)] = PRIORITY_INDICES;
SPLIT_MUTABLE uint8_t priority_index_num = PRIORITY_INDEX_NUM;
#endif

//MARK: Init
void analog_matrix_init(void) {
    //TODO: Abstract this (Enables FPU)
    // SCB->CPACR |= ((3UL << 20U)|(3UL << 22U));  /* set CP10 and CP11 Full Access */

    // eeconfig_read_keyboard((analog_switch_t*)&calibration_data);
    // for(uint8_t key = 0; key < SWITCH_NUM; key++) {
    //     printf("%u: Top: %u, Bottom: %u\n", key, calibration_data[key].top_value, calibration_data[key].bottom_value);
    // }

    #ifdef SPLIT_KEYBOARD
    // Determine keyboard half
    #if KEYBOARD_SIDE == UNKNOWN
    assign_split_side(is_keyboard_left());
    #endif // if KEYBOARD_SIDE == UNKNOWN
    #endif // defined SPLIT_KEYBOARD

    for(uint8_t i = 0; i < ADC_PIN_NUM; i++) {
        palSetLineMode(adc_pins[i], PAL_MODE_INPUT_ANALOG);
        // Convert adc pins to adc mux combination
        adc_pin_mux[i] = pinToMux(adc_pins[i]);
    }

    #ifdef MUX_PINS
    for(uint8_t i = 0; i < MUX_PIN_NUM; i++) {
        gpio_set_pin_output_push_pull(mux_pins[i]);
        gpio_write_pin_low(mux_pins[i]);
    }
    #endif

    #if POWER_BEFORE_SCAN == TRUE
    for(uint8_t i = 0; i < POWER_PIN_NUM; i++) {
        gpio_set_pin_output_push_pull(power_pins[i]);
        gpio_write_pin_low(power_pins[i]);
    }

    #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
    set_sensor_power_init_kb();

    #elif defined MATRIX_POWER_PIN
    gpio_set_pin_output_push_pull(MATRIX_POWER_PIN);
    #ifndef INVERT_MATRIX_POWER
    gpio_write_pin_high(MATRIX_POWER_PIN);
    #else
    gpio_write_pin_low(MATRIX_POWER_PIN);
    #endif
    #endif

    get_switch_data();

    // Get the min/max values of each switch
    if(!get_calibration_data()) {
    // If loading the calibration data fails, start calibration
        calibrate_switches();
    }

    // Check keys like the calibration or bootmagic key before scanning begins
    #if AM_INIT_KEY_NUM > 0 || AM_INIT_KEY_NUM_R > 0
    scan_init_keys();
    #endif

    // Translate the trigger height etc into the equivalent ADC value
    translate_mm_to_value();

    #if defined JOYSTICK_ENABLE || defined MIDI_ENABLE
    // Create masks for special key modes
    create_layer_masks(highest_layer);
    #endif

    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}


//MARK: Init keys
#if AM_INIT_KEY_NUM > 0
static void scan_init_keys(void) {
    uint16_t adc_value;
    // Long delay needed for correct init key reading after being plugged in
    delay_ns(20000);

    for(uint8_t idx = 0; idx < AM_INIT_KEY_NUM; idx++) {
        #if POWER_BEFORE_SCAN == TRUE
        gpio_write_pin_high(power_pins[init_keys[idx][1]]);
        #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
        set_sensor_power_high_kb(init_keys[idx][1]);
        #endif // CUSTOM_POWER_BEFORE_SCAN

        set_mux_channel(init_keys[idx][1]);
        #ifdef MUX_SELECT_DELAY
        delay_ns(MUX_SELECT_CYCLES);
        #endif // MUX_SELECT_DELAY

        uint8_t matrix_index = mux_to_num[init_keys[idx][1]][init_keys[idx][0]] - 1;
        delay_ns(7000);
        adc_value = adc_read(adc_pin_mux[init_keys[idx][0]]);

        #if POWER_BEFORE_SCAN == TRUE
        gpio_write_pin_low(power_pins[init_keys[idx][1]]);
        #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
        set_sensor_power_low_kb(init_keys[idx][1]);
        #endif // CUSTOM_POWER_BEFORE_SCAN

        // If the key is activated, call the respective function
        // These functions are set in the INIT_FUNCTIONS dict at the top of he_config_h.py
        #ifndef INVERT_ADC
        if(adc_value < key_config[matrix_index].bottom_value + 50) {
            LED_ON;
            (*init_functions[idx])();
        }
        #else
        if(adc_value > key_config[matrix_index].bottom_value - 50) {
            LED_ON;
            (*init_functions[idx])();
        }
        #endif
    }
}
#else
#define scan_init_keys();
#endif // AM_INIT_KEY NUM > 0


//TODO: Would it be faster to save the mux info to the switch, loop through the key_config indices,
//      and sort the key_config by mux_channel, like the priority muxes?
//      Maybe even sort the adc channels to be ascending, then descending,
//      so at least one adc channel is used twice in a row, if that makes a difference
//MARK: Scan
uint8_t analog_matrix_scan() {
    bool matrix_has_changed = false;
    uint16_t adc_value;
    uint8_t index;

    #ifdef PRIORITY_MUXES
    if(scan_amt < PRIORITY_LEVEL) {
        scan_amt += 1;
        return matrix_scan_priority(current_matrix);
    } else { scan_amt = 0; }
    #endif

    for(uint8_t mux_channel = 0; mux_channel < MUX_CHANNELS; mux_channel++) {
        #if POWER_BEFORE_SCAN == TRUE
        set_sensor_power(mux_channel);
        #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
        set_sensor_power_high_kb(mux_channel);
        #endif

        set_mux_channel(mux_channel);
        #ifdef MUX_SELECT_DELAY
        delay_ns(MUX_SELECT_CYCLES);
        #endif

        for(uint8_t adc_channel = 0; adc_channel < ADC_PIN_NUM; adc_channel++) {
            // Translate matrix mux and adc channels to matrix position
            index = mux_to_num[mux_channel][adc_channel];
            // Check if a switch is at the position (matrix index > 0), and scan if so
            if(index > 0) {

                index -= 1;
                #ifdef PRIORITY_INDICES
                if(scan_amt < PRIORITY_LEVEL) {
                    if(!priority_indices[index]) { continue; }
                }
                #endif

                adc_value = adc_read(adc_pin_mux[adc_channel]);

                #ifdef DEBUG_MUX_VALUE
                if(adc_channel == debug_mux[0] && mux_channel == debug_mux[1]) {
                    dprintf("%u\n", adc_value);
                }
                #elif DEBUG_SCAN_VALUES == TRUE
                dprintf("%u/%2u: %3u, ", adc_channel, mux_channel, adc_value);
                #endif
                #ifdef ADC_SCAN_DELAY
                delay_ns(ADC_SCAN_CYCLES);
                #endif

                // Check if the value has changed enough to warrant an evaluation
                //TODO: This could perhaps cause issues around the borders, with missing updates?
                if ((adc_value < (key_config[index].scan_value + ADC_SMOOTHING)) && (adc_value > (key_config[index].scan_value - ADC_SMOOTHING))) {
                    continue;
                }
                else {
                    // key_config[index].scan_value = adc_value;
                    // Check if key is pressed/released and set matrix_has_changed, returns true if the switch state has changed
                    if(evaluate_value(index, adc_value)) {
                        matrix_has_changed = true;
                        #if DEBUG_SCAN_NO_INPUT != TRUE
                        matrix[key_config[index].row] ^= 1 << key_config[index].col;
                        #endif
                        // If dynamic calibration is enabled, check if the boundaries need updating
                        #if DYNAMIC_CALIBRATION == TRUE
                        if(update_switch_bounds(index, adc_value)) {
                            // If the bounds have been updated, translate the heights and save the new bounds
                            translate_mm_to_value(index);
                            //TODO: Update this
                            #if AM_NO_EEPROM == FALSE
                            save_calibration_data();
                            #endif
                        }
                        #endif // DYNAMIC_CALIBRATION == TRUE
                    }
                    key_config[index].scan_value = adc_value;
                }
            }
            #if DEBUG_SCAN_VALUES == TRUE
            else {
                dprintf("%u/%2u:    , ",adc_channel, mux_channel);
            }
            #endif
        }
        #if CUSTOM_POWER_BEFORE_SCAN == TRUE
        set_sensor_power_low_kb(mux_channel);
        #endif
    }

    #if DEBUG_SCAN_VALUES == TRUE
    dprint("\n");
    #endif

    #ifdef PRIORITY_INDICES
    if(scan_amt < PRIORITY_LEVEL) {
        scan_amt += 1;
    } else { scan_amt = 0; }
    #endif

    //TODO: Add debounce support, in case some people need it. Currently doesn't work for some reason
    #if DEBOUNCE > 0
    #error "Debouncing currently doesn't work with ANALOG_MATRIX, set debounce to 0 or leave the parameter out to disable it!"
    #ifdef SPLIT_KEYBOARD
    matrix_has_changed = debounce(raw_matrix, matrix + thisHand, MATRIX_ROWS_PER_HAND, matrix_has_changed) | matrix_post_scan();
    #else
    matrix_has_changed = debounce(raw_matrix, matrix, MATRIX_ROWS_PER_HAND, changed);
    matrix_scan_kb();
    #endif

    #else // if DEBOUNCE > 0
    #ifdef SPLIT_KEYBOARD
    #ifndef SLAVE_LOW_PRIORITY
    matrix_has_changed |= matrix_post_scan();
    #else // Only synchronise the matrices during the full scans if the slave is low priority (doesn't contain priority keys)
    if(scan_amt == 0) {
        matrix_has_changed |= matrix_post_scan();
    }
    #endif // ifndef SLAVE_LOW_PRIORITY
    #else // ifdef SPLIT_KEYBOARD
    matrix_scan_kb();
    #endif // ifdef SPLIT_KEYBOARD
    #endif // if DEBOUNCE > 0

    return matrix_has_changed;
}


//MARK: Translate
// Translate the trigger height etc of all keys into the corresponding ADC values
void translate_mm_to_value(void) {
    uint8_t key_modes[][SWITCH_NUM] = KEY_MODES;

    for(uint8_t index = 0; index < switch_num; index++) {
        uint16_t bottom_value = key_config[index].bottom_value;
        uint16_t top_value = key_config[index].top_value;
        #if INVERT_ADC == FALSE
        // ADC count per mm of travel
        uint16_t travel_unit = floor((top_value - bottom_value) / TRAVEL_DISTANCE);
        #else
        uint16_t travel_unit = floor((bottom_value - top_value) / TRAVEL_DISTANCE);
        #endif

        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            #if INVERT_ADC == FALSE
            #if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
            #if DISTANCE_FROM_BOTTOM == FALSE
            key_config[index].trigger_value[profile] = top_value - (travel_unit * trigger_height[profile][index]) - ADC_SMOOTHING;
            key_config[index].release_value[profile] = top_value - (travel_unit * release_height[profile][index]) + ADC_SMOOTHING;

            #else // if DISTANCE_FROM_BOTTOM == FALSE
            key_config[index].trigger_value[profile] = travel_unit * trigger_height[profile][index] + bottom_value - ADC_SMOOTHING;
            key_config[index].release_value[profile] = travel_unit * release_height[profile][index] + bottom_value + ADC_SMOOTHING;
            #endif // else DISTANCE_FROM_BOTTOM == FALSE
            #endif

            #else //Inverted ADC -> Lower switch means higher value

            #if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
            #if DISTANCE_FROM_BOTTOM == FALSE
            //TODO: Is this correct
            key_config[index].trigger_value[profile] = top_value + (travel_unit * trigger_height[profile][index]) + ADC_SMOOTHING;
            key_config[index].release_value[profile] = top_value + (travel_unit * release_height[profile][index]) - ADC_SMOOTHING;

            #else // if DISTANCE_FROM_BOTTOM == FALSE
            key_config[index].trigger_value[profile] = bottom_value - (travel_unit * trigger_height[profile][index]) + ADC_SMOOTHING;
            key_config[index].release_value[profile] = bottom_value - (travel_unit * release_height[profile][index]) - ADC_SMOOTHING;
            #endif // else DISTANCE_FROM_BOTTOM == FALSE
            #endif // if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
            #endif //else INVERT ADC == TRUE

            #if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
            key_config[index].rt_press_value[profile] = travel_unit * rt_press_distance[profile][index];
            key_config[index].rt_release_value[profile] = travel_unit * rt_release_distance[profile][index] + ADC_SMOOTHING;
            #endif

            // Assign the switch mode
            key_config[index].mode[profile] = key_modes[profile][index];
        }
    }
}


//MARK: Get calibration
bool get_calibration_data(void) {
    #if FORCE_CALIBRATE == TRUE
    return false;
    #endif

#   if AM_NO_EEPROM == TRUE
    #if !defined AM_TOP_VALUES || !defined AM_BOTTOM_VALUES
    return false;
    #endif

    #if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
    uint16_t top_values[MAX(SWITCH_NUM, SWITCH_NUM_R)] = AM_TOP_VALUES;
    uint16_t bottom_values[MAX(SWITCH_NUM, SWITCH_NUM_R)] = AM_BOTTOM_VALUES;

    if(!is_keyboard_left()) {
        const uint16_t top_values_r[MAX(SWITCH_NUM, SWITCH_NUM_R)] = AM_TOP_VALUES_R;
        const uint16_t bottom_values_r[MAX(SWITCH_NUM, SWITCH_NUM_R)] = AM_BOTTOM_VALUES_R;
        memcpy(&top_values, &top_values_r, sizeof(top_values_r));
        memcpy(&bottom_values, &bottom_values_r, sizeof(top_values_r));
    }
    #else

    #   if defined AM_TOP_VALUES && defined AM_BOTTOM_VALUES
    const uint16_t top_values[SWITCH_NUM] = AM_TOP_VALUES;
    const uint16_t bottom_values[SWITCH_NUM] = AM_BOTTOM_VALUES;
    #   else
    const uint16_t top_values[SWITCH_NUM];
    const uint16_t bottom_values[SWITCH_NUM];
    return false;
    #   endif
    #endif // if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN

    #if DYNAMIC_CALIBRATION == FALSE
    #if INVERT_ADC == FALSE
    for(uint8_t key = 0; key < switch_num; key++) {
        key_config[key].top_value = top_values[key] - ADC_TOP_DEADZONE;
        key_config[key].bottom_value = bottom_values[key] + ADC_BOTTOM_DEADZONE;
        key_config[key].pressed = false;
    }
    #else // INVERT_ADC == FALSE
    for(uint8_t key = 0; key < switch_num; key++) {
        key_config[key].top_value = top_values[key] + ADC_TOP_DEADZONE;
        key_config[key].bottom_value = bottom_values[key] - ADC_BOTTOM_DEADZONE;

        key_config[key].pressed = false;
    }
    #endif // else INVERT_ADC == FALSE

    #else // DYNAMIC_CALIBRATION == FALSE
    #if INVERT_ADC == FALSE
    for(uint8_t key = 0; key < switch_num; key++) {
        // AM_DC_FACTOR should be defined as < 1
        key_config[key].top_value = AM_DC_FACTOR * top_values[key] - ADC_TOP_DEADZONE;
        key_config[key].bottom_value = (2 - AM_DC_FACTOR) * bottom_values[key] + ADC_BOTTOM_DEADZONE;
    }
    #else // INVERT_ADC == TRUE
    for(uint8_t key = 0; key < switch_num; key++) {
        key_config[key].top_value = (2 - AM_DC_FACTOR) * top_values[key] + ADC_TOP_DEADZONE;
        key_config[key].bottom_value = AM_DC_FACTOR * bottom_values[key] - ADC_BOTTOM_DEADZONE;
    }
    #endif // INVERT_ADC
    #endif // DYNAMIC_CALIBRATION

    return true;

#   else //if AM_NO_EEPROM == TRUE

    // Read the calibration data from EEPROM
    //TODO: I don't think i need the cast here
    eeconfig_read_keyboard((analog_switch_t*)&calibration_data);

    //TODO: Add dynamic calibration stuff here
    for(uint8_t key = 0; key < switch_num; key++) {
        //TODO: Remove these when it works
        printf("%u: Top: %u, Bottom: %u\n", key, calibration_data[key].top_value, calibration_data[key].bottom_value);
        // TODO: Check if this is fine
        // Check if the switch data makes sense, start calibration if not
        if(calibration_data[key].top_value < 10 || calibration_data[key].bottom_value < 10) return false;
        if(calibration_data[key].top_value > 1000 || calibration_data[key].bottom_value > 1000) return false;
        #if INVERT_ADC == FALSE
        if(calibration_data[key].top_value - calibration_data[key].bottom_value < 50) return false;
        if(calibration_data[key].top_value <= calibration_data[key].bottom_value) return false;
        #else
        if(calibration_data[key].bottom_value - calibration_data[key].top_value < 50) return false;
        if(calibration_data[key].top_value >= calibration_data[key].bottom_value) return false;
        #endif

        print("Ok\n");

        // All checks passed for this switch
        //TODO: Check if deadzones are applied here or not
        key_config[key].top_value = calibration_data[key].top_value;
        key_config[key].bottom_value = calibration_data[key].bottom_value;
    }

    return true;
    #endif //else AM_NO_EEPROM == TRUE
}


//MARK: Calibrate
//TODO: Check if the difference between top and bottom value is at least ~50 or so, before allowing calibration to finish
void calibrate_switches(void) {
    uint16_t adc_value;
    uint8_t matrix_index;
    // Save config values after x scans without a change
    uint32_t scans_without_change = SCANS_WITHOUT_CHANGE;
    bool first_scan = true;
    //TODO: Calibration sync needs to be added
    // __attribute__((unused)) bool calibration_done = false;
    #if AM_NO_EEPROM == true
    char side[7] = "";
    #endif

    #ifdef SPLIT_KEYBOARD
    // if(is_keyboard_master()) {
    //     //TODO: Check if NULL size works
    //     transaction_rpc_send(AM_CALIBRATION_M2S_SYNC, 8, &first_scan);
    // }
    if(!is_keyboard_left()) {
        char right[] = "_right";
        memcpy(&side, right, sizeof(right));
    }
    #endif

    while(true) {
        LED_ON;
        for(uint8_t mux_channel = 0; mux_channel < MUX_CHANNELS; mux_channel++) {
            #if DEBUG_CALIBRATION == TRUE || DEBUG_SCAN_VALUES == TRUE
            dprintf("Mux: %i\n", mux_channel);
            #endif
            #if POWER_BEFORE_SCAN == TRUE
            set_sensor_power(mux_channel);
            #ifdef POWER_SELECT_DELAY
            delay_ns(POWER_SELECT_CYCLES);
            #endif
            #elif CUSTOM_POWER_BEFORE_SCAN == TRUE
            set_sensor_power_high_kb(mux_channel);
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
                    dprintf("%u/%2u: %i, ", adc_channel, mux_channel, adc_value);
                    #endif

                    if(first_scan) {
                        #if INVERT_ADC == FALSE
                        key_config[matrix_index].top_value = adc_value - ADC_TOP_DEADZONE;
                        key_config[matrix_index].bottom_value = adc_value + ADC_BOTTOM_DEADZONE;

                        #else
                        key_config[matrix_index].top_value = adc_value + ADC_TOP_DEADZONE;
                        key_config[matrix_index].bottom_value = adc_value - ADC_BOTTOM_DEADZONE;
                        #endif

                        continue;
                    }

                    #if INVERT_ADC == TRUE
                    if(adc_value < key_config[matrix_index].top_value - ADC_TOP_DEADZONE){
                        key_config[matrix_index].top_value = adc_value + ADC_TOP_DEADZONE;
                        #if AM_NO_EEPROM == FALSE
                        calibration_data[matrix_index].top_value = adc_value + ADC_TOP_DEADZONE;
                        #endif
                        scans_without_change = 0;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new top value %i\n", matrix_index, adc_value);
                        #endif

                    } else if (adc_value > key_config[matrix_index].bottom_value + ADC_BOTTOM_DEADZONE) {
                        key_config[matrix_index].bottom_value = adc_value - ADC_BOTTOM_DEADZONE;
                        #if AM_NO_EEPROM == FALSE
                        calibration_data[matrix_index].bottom_value = adc_value - ADC_BOTTOM_DEADZONE;
                        #endif
                        scans_without_change = 0;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new bottom value %i\n", matrix_index, adc_value);
                        #endif

                    } else if(key_config[matrix_index].bottom_value - key_config[matrix_index].top_value < 50) {
                        scans_without_change = SCANS_WITHOUT_CHANGE;
                    }

                    #else // if INVERT_ADC == TRUE
                    if(adc_value > key_config[matrix_index].top_value + ADC_TOP_DEADZONE){
                        key_config[matrix_index].top_value = adc_value - ADC_TOP_DEADZONE;
                        #if AM_NO_EEPROM == FALSE
                        calibration_data[matrix_index].top_value = adc_value - ADC_TOP_DEADZONE;
                        #endif
                        scans_without_change = SCANS_WITHOUT_CHANGE;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new top value %i\n", matrix_index, adc_value);
                        #endif

                    } else if (adc_value < key_config[matrix_index].bottom_value - ADC_BOTTOM_DEADZONE) {
                        key_config[matrix_index].bottom_value = adc_value + ADC_BOTTOM_DEADZONE;
                        #if AM_NO_EEPROM == FALSE
                        calibration_data[matrix_index].bottom_value = adc_value + ADC_BOTTOM_DEADZONE;
                        #endif
                        scans_without_change = SCANS_WITHOUT_CHANGE;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new bottom value %i\n", matrix_index, adc_value);
                        #endif

                    // Check if new valid calibration values have been saved for this key
                    //TODO: Check more stuff here
                    } else if(key_config[matrix_index].top_value - key_config[matrix_index].bottom_value < 50) {
                        scans_without_change = SCANS_WITHOUT_CHANGE;
                    }
                    #endif // else INVERT_ADC == TRUE
                    // Dummy evaluation, so that a calibration cycle takes about as long as a scan cycle
                    evaluate_value(matrix_index, adc_value);
                }
            }

            #if DEBUG_CALIBRATION == TRUE || DEBUG_SCAN_VALUES == TRUE
            dprint("\n");
            #endif
            #if CUSTOM_POWER_BEFORE_SCAN == TRUE
            set_sensor_power_low_kb(mux_channel);
            #endif
        }

        first_scan = false;
        scans_without_change += 1;
        //TODO: Change this to a more precise method
        //TODO: I don't think the qmk tick is incremented here, but if I can do that I could use it to keep time precisely
        if(scans_without_change/2 >= SCANS_WITHOUT_CHANGE){
            LED_OFF;
            #if AM_NO_EEPROM == FALSE
            //TODO: Save calibration data to eeprom
            // save_calibration();
            //TODO: Test this
            eeconfig_update_keyboard((analog_switch_t*)&calibration_data);
            #else //AM_NO_EEPROM == FALSE
            // Print the calibration values of each switch so that they can be adjusted in the config
            printf("\"top_values%s\": [ %u", side, key_config[0].top_value);
            // Reset the pressed states on all switches
            key_config[0].pressed = false;
            if(switch_num > 1) {
                for(uint8_t index = 1; index < switch_num; index++){
                    printf(", %u", key_config[index].top_value);
                    key_config[index].pressed = false;
                }
            }

            printf(" ],\n\"bottom_values%s\": [ %u", side, key_config[0].bottom_value);
            if(switch_num > 1) {
                for(uint8_t index = 1; index < switch_num; index++){
                    printf(", %u", key_config[index].bottom_value);
                }
            }
            print(" ]\nYou can paste these lines into keyboard.json under hall_effect.config\n\n");
            #endif //else NO_EEPROM == FALSE

            scans_without_change = 0;
            #ifdef SPLIT_KEYBOARD
            // if(is_keyboard_master()) {
                // uint8_t slave_switch_num;
                // if(is_keyboard_left()) {
                //     slave_switch_num = SWITCH_NUM_R;
                // } else {
                //     slave_switch_num = SWITCH_NUM_L;
                // }
                // uint8_t slave_state = 2;
                // if(transaction_rpc_recv(AM_CALIBRATION_STATE_SYNC, 8, &slave_state)) {
                //     printf("Received state %u\n", slave_state);
                // } else { print("Failed state sync\n"); }
                // if(slave_state == true) {
                //     #if AM_NO_EEPROM == TRUE
                //     transaction_rpc_recv(AM_CALIBRATION_S2M_SYNC, sizeof(slave_data), &slave_data);
                //     // Print the calibration values of each switch so that they can be adjusted in the config
                //     printf("\"top_values%s\": [ %u", side, slave_data.top_values[0]);
                //     //TODO: If only one key is used (per half), this will cause issues
                //     for(uint8_t index = 1; index < slave_switch_num; index++){
                //         printf(", %u", slave_data.top_values[index]);
                //     }

                //     printf(" ],\n\"bottom_values%s\": [ %u", side, slave_data.bottom_values[0]);
                //     for(uint8_t index = 1; index < slave_switch_num; index++){
                //         printf(", %u", slave_data.bottom_values[index]);
                //     }
                //     print(" ]\nYou can paste these lines into keyboard.json under hall_effect.config\n\n");
                //     #endif //else NO_EEPROM == FALSE

                //     break;
                // }
            // }
            #endif
            break;
        }
    }
}


//MARK: Evaluate
static inline bool evaluate_value(uint8_t index, uint16_t value) {
    #if defined JOYSTICK_ENABLE
    if(joystick_layer){
        if(key_config[index].axis_index != -1) {
            if(!translate_joystick_axis(index)) { return false; }
            if(!evaluate_joystick_axis(key_config[index].axis_index)) { return false; }
            return true;
        }
    }
    #endif

    bool prev_pressed = key_config[index].pressed;

#if INVERT_ADC == TRUE
    switch(key_config[index].mode[active_profile]) {
        case none:
        #if defined USE_NONE
        if(value > key_config[index].trigger_value[active_profile]) {
            key_config[index].pressed = true;
        } else if(value < key_config[index].release_value[active_profile]) {
            key_config[index].pressed = false;
        } else {
            return false;
        }
        #endif //defined RAPID_TRIGGER
        break;

        case rapid_trigger:
        #if defined USE_RAPID_TRIGGER
        // Rapid trigger is only active when the switch is lower than the trigger and release height
        if(value > key_config[index].trigger_value[active_profile]) {
            // Set the new lowest value if needed
            if(value > key_config[index].rt_threshold + ADC_SMOOTHING) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value + key_config[index].rt_threshold) < key_config[index].rt_release_value[active_profile]) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value + key_config[index].rt_press_value[active_profile];
            }
        } else if(value < key_config[index].release_value[active_profile]) {
            // If the switch is not pressed past the threshold, reset it
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
        }
        #endif
        break;

        case continuous_rapid_trigger:
        #if defined USE_CONTINUOUS_RAPID_TRIGGER
        // Rapid trigger activates below the trigger height, but only stops when fully released
        if(key_config[index].rt_active || (value > key_config[index].trigger_value[active_profile] + ADC_SMOOTHING)) {
            key_config[index].rt_active = true;
            // Set the new lowest value if needed
            if(value > key_config[index].rt_threshold + ADC_SMOOTHING || value > key_config[index].bottom_value) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value + key_config[index].rt_threshold) < key_config[index].rt_release_value[active_profile]) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value + key_config[index].rt_press_value[active_profile];
            }  else if(value < key_config[index].top_value) {
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
            key_config[index].rt_active = false;
            }
        // Check if the switch has been released completely
        } else if(value < key_config[index].top_value) {
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
            key_config[index].rt_active = false;
        }
        #endif
        break;

        case constant_rapid_trigger:
        #if defined USE_CONSTANT_RAPID_TRIGGER
        // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
        if((value > key_config[index].rt_threshold + ADC_SMOOTHING) || value > key_config[index].bottom_value) {
            key_config[index].pressed = true;
            key_config[index].rt_threshold = value;
        // Check if the key has been released past the threshold or completely
        } else if((value + key_config[index].rt_threshold + ADC_SMOOTHING) < key_config[index].rt_release_value[active_profile]
                  || value < key_config[index].top_value) {
            key_config[index].pressed = false;
            // Set the new activation threshold
            key_config[index].rt_threshold = value + key_config[index].rt_press_value[active_profile];
        }
        #endif
        break;

        default:
        return false;
    }

#else //if INVERT_ADC == TRUE
    switch(key_config[index].mode[active_profile]) {
        case none:
        #if defined USE_NONE
        if(value < key_config[index].trigger_value[active_profile]) {
            key_config[index].pressed = true;
        } else if(value > key_config[index].release_value[active_profile]) {
            key_config[index].pressed = false;
        } else {
            return false;
        }
        #endif //defined USE_NONE
        break;

        case rapid_trigger:
        #if defined USE_RAPID_TRIGGER
        // Rapid trigger is only active when the switch is lower than the trigger and release height
        if(value < key_config[index].trigger_value[active_profile]) {
            // Set the new lowest value if needed
            if(value < key_config[index].rt_threshold - ADC_SMOOTHING) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value - key_config[index].rt_threshold) > key_config[index].rt_release_value[active_profile]) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value - key_config[index].rt_press_value[active_profile];
            }
        //TODO: Check if it is faster to check if the key state is released, and only set values if they're not set already
        } else if(value > key_config[index].release_value[active_profile]) {
            // If the switch is not pressed past the threshold, reset it
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
        }
        #endif // defined USE_RAPID_TRIGGER
        break;

        case continuous_rapid_trigger:
        #if defined USE_CONTINUOUS_RAPID_TRIGGER
        // Rapid trigger activates below the trigger height, but only stops when fully released
        if(key_config[index].rt_active || (value < key_config[index].trigger_value[active_profile] - ADC_SMOOTHING)) {
            key_config[index].rt_active = true;
            // Set the new lowest value if needed
            if(value < key_config[index].rt_threshold - ADC_SMOOTHING || value < key_config[index].bottom_value) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value - key_config[index].rt_threshold) > key_config[index].rt_release_value[active_profile]) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value - key_config[index].rt_press_value[active_profile];
            } else if(value > key_config[index].top_value) {
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
            key_config[index].rt_active = false;
            }
        // Check if the switch has been released completely
        } else if(value > key_config[index].top_value) {
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
            key_config[index].rt_active = false;
        }
        #endif // defined USE_CONTINUOUS_RAPID_TRIGGER
        break;

        case constant_rapid_trigger:
        #if defined USE_CONSTANT_RAPID_TRIGGER
        // Check if the key has been pressed far enough for rapid trigger to activate it, or pressed down completely
        if(value < key_config[index].rt_threshold - ADC_SMOOTHING || value < key_config[index].bottom_value) {
            key_config[index].pressed = true;
            key_config[index].rt_threshold = value;

        // Check if the key has been released far enough
        } else if((value - key_config[index].rt_threshold) > key_config[index].rt_release_value[active_profile]
                  || value > key_config[index].top_value) {
            key_config[index].pressed = false;
            // Set the new activation threshold
            key_config[index].rt_threshold = value - key_config[index].rt_press_value[active_profile];
        }
        #endif // defined USE_CONSTANT_RAPID_TRIGGER
        break;

        default:
        return false;
    }
#endif //if INVERT_ADC == TRUE else

    return !(prev_pressed == key_config[index].pressed);
}


//MARK: Update bounds
#if DYNAMIC_CALIBRATION == TRUE
inline bool update_switch_bounds(uint8_t index, uint16_t value) {
    #if INVERT_ADC == FALSE
    if(value > key_config[index].top_value + AM_DC_DELTA + ADC_TOP_DEADZONE){
        key_config[index].top_value = value - ADC_TOP_DEADZONE;
        return true;
    } else if (value < key_config[index].bottom_value - AM_DC_DELTA - ADC_BOTTOM_DEADZONE) {
        key_config[index].bottom_value = value + ADC_BOTTOM_DEADZONE;
        return true;
    }

    #else // if INVERT_ADC == FALSE
    if(value < key_config[index].top_value - AM_DC_DELTA - ADC_TOP_DEADZONE){
        key_config[index].top_value = value + ADC_TOP_DEADZONE;
        return true;
    } else if (value > key_config[index].bottom_value + AM_DC_DELTA + ADC_BOTTOM_DEADZONE) {
        key_config[index].bottom_value = value - ADC_BOTTOM_DEADZONE;
        return true;
    }
    #endif /// else INVERT_ADC == FALSE
    return false;
}
#endif


//MARK: Switch data
// Populates the key matrix with the static config params, heights are populated separately
void get_switch_data(void) {
    for(uint8_t key = 0; key < switch_num; key++) {
        uint8_t row = num_to_matrix[key][0];
        uint8_t col = num_to_matrix[key][1];
        key_config[key].row = row;
        key_config[key].col = col;

        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++){
            key_config[key].mode[profile] = key_modes[profile][key];
        }

        #if defined JOYSTICK_ENABLE
        key_config[key].axis_index = -1;
        #endif
    }
}


//MARK: Bootmagic
void _bootmagic(void) {
    eeconfig_disable();
    bootloader_jump();
}


//TODO: Remove this and the call in keyboard.c at the start
//MARK: Force bootloader
//TODO: Do actual init key checking here?
void _force_bootloader(void) {
    // palSetLineMode(A0, PAL_MODE_INPUT_ANALOG);
    // //TODO: This probably doesn't work with dynamic side assignment
    // #ifdef MUX_PINS
    // pin_t pins[4] = MUX_PINS;
    // for(uint8_t i = 0; i < 4; i++) {
    //     gpio_set_pin_output_push_pull(pins[i]);
    // }
    // delay_ns(3000);
    // set_mux_channel(FORCE_BOOTLOADER_CHANNEL);
    // #endif
    // // Dummy read, first read is way off
    // adc_read(pinToMux(FORCE_BOOTLOADER_PIN));
    // delay_ns(32000);
    // uint16_t val = adc_read(pinToMux(A0));
    // // printf("Val: %u", val);
    // // set_mux_channel(0);
    // if(val < 400) {
    //     LED_ON;
    //     bootloader_jump();nn
    // }
}


//MARK: Power pins
#ifdef POWER_PINS
static inline void set_sensor_power(uint8_t index) {
    #ifdef POWER_PINS_CONTINUOUS
    #ifdef USE_BSRR
    #ifdef AT32F415
    // Set action takes priority
    CONTINUOUS_POWER_PORT->SCR.W = (((1 << POWER_PIN_NUM) - 1) << POWER_PIN_OFFSET << 16) | (1 << (index + POWER_PIN_OFFSET));
    #else
    CONTINUOUS_POWER_PORT->BSRR.W = (((1 << POWER_PIN_NUM) - 1) << POWER_PIN_OFFSET << 16) | (1 << (index + POWER_PIN_OFFSET));
    #endif
    #else
    CONTINUOUS_POWER_PORT->ODR = (CONTINUOUS_POWER_PORT->ODR & ~(((1 << POWER_PIN_NUM) - 1) << POWER_PIN_OFFSET)) | (1 << (index + POWER_PIN_OFFSET));
    #endif
    #else

    if(index == 0) {
        gpio_write_pin_low(power_pins[POWER_PIN_NUM - 1]);
        gpio_write_pin_high(power_pins[0]);
    } else {
        gpio_write_pin_low(power_pins[index-1]);
        gpio_write_pin_high(power_pins[index]);
    }
    #endif

    #ifdef POWER_SELECT_DELAY
    delay_ns(POWER_SELECT_CYCLES);
    #endif
}
#endif


//MARK: Delay
//TODO: Scale this so that it's roughly correct
static inline void delay_ns(uint16_t delay) {
    // delay = (delay * 1000000000 / 180000000000);
    for(; delay > 0; delay--){
        __asm("");
    }
}


//MARK: Profiles
// If true, layer_state_set doesn't update the profile together with the layer. Toggled via AM_LOCP.
bool manual_profile_lock = false;

uint8_t get_active_profile(void) { return active_profile; }

void lock_profile(void) { manual_profile_lock = !manual_profile_lock; }

#if AM_PROFILE_NUM > 1
//TODO: Do I need this separation, since I'm not manually triggering the transaction anymore?
#ifndef SPLIT_KEYBOARD
void set_active_profile(uint8_t profile) { active_profile = profile; }
#else // ifndef SPLIT_KEYBOARD
void set_active_profile(uint8_t profile) {
    if(is_keyboard_master()) {
        active_profile = profile;
        //TODO: Manually trigger profile transaction instead of having it checked every scan
        // Send the new profile to the slave
        // transaction_rpc_send(AM_PROFILE_SYNC, 8, &active_profile);
    }
}
#endif // ifndef SPLIT_KEYBOARD else

//TODO: Move this to analog_joystick.c
#ifdef JOYSTICK_ENABLE
// Clear the matrix and switch state of all joystick keys
// void reset_joystick_keys(void) {
//     // Reset the pressed state of all joystick keys to avoid stuck keys
//     for(uint8_t index = 0; index < switch_num; index++) {
//         // Here I could also check if the joystick axis field is set
//         const uint8_t row = key_config[index].row;
//         const uint8_t col = key_config[index].col;

//         if(joystick_mask[row] & 1 << col) {
//             key_config[index].pressed = false;
//         }
//     }
//     // Reset the matrix state of all joystick keys
//     for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
//         matrix[row] &= ~joystick_mask[row];
//     }
// }
#endif

// void reset_key_states(void) {
//     for(uint8_t index = 0; index < switch_num; index++) {
//         key_config[index].pressed = false;
//     }
//     // memset(&matrix, 0, sizeof(matrix));
//     for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
//         matrix[row] = 0;
//     }
// }


//MARK: Layer state
layer_state_t layer_state_set_kb(layer_state_t state) {
    highest_layer = get_highest_layer(state);

    #if defined JOYSTICK_ENABLE && !defined USE_JOYSTICK
    if(joystick_layer) { reset_joystick_keys(); }
    #endif
    #if defined MIDI_ENABLE
    if(midi_layer) { reset_midi_keys(); }
    #endif
    create_layer_masks(highest_layer);

    #if PROFILE_SWITCH_MODE != MANUAL_PROFILE
    if (!manual_profile_lock) {
        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            // If the highest active layer is in the layers list of that profile, activate it
            if(profiles[profile].layers & (1 << highest_layer)) {
                set_active_profile(profile);

                // Only the lowest profile should apply
                return layer_state_set_user(state);
            }
        }
        // If profile switch mode is default, switch to the default profile if layer is not set for any profile
        #if PROFILE_SWITCH_MODE == DEFAULT_PROFILE
        set_active_profile(AM_DEFAULT_PROFILE);
        #endif
    }
    #endif

    // Need to call the user function
    return layer_state_set_user(state);
}
#endif // if AM_PROFILE_NUM > 1


#ifdef SPLIT_KEYBOARD
#if KEYBOARD_SIDE == UNKNOWN
//MARK: Split side
//TODO: Make sure they all have same size
void assign_split_side(bool side) {
    if(side == RIGHT) {
        memcpy(&mux_to_num, &mux_to_num_r, sizeof(mux_to_num_r));
        memcpy(&num_to_matrix, &num_to_matrix_r, sizeof(num_to_matrix_r));
        memcpy(&key_modes, &key_modes_r, sizeof(key_modes_r));

        #if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
        memcpy(&trigger_height, &trigger_height_r, sizeof(trigger_height_r));
        memcpy(&release_height, &release_height_r, sizeof(release_height_r));
        #endif
        #if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
        memcpy(&rt_press_distance, &rt_press_distance_r, sizeof(rt_press_distance_r));
        memcpy(&rt_release_distance, &rt_release_distance_r, sizeof(rt_release_distance_r));
        #endif

        #ifndef EQUAL_ADC_PINS
        memcpy(&adc_pins, &adc_pins_r, sizeof(adc_pins_r));
        #endif
        #if defined MUX_PINS && !defined EQUAL_MUX_PINS
        memcpy(&mux_pins, &mux_pins_r, sizeof(mux_pins_r));
        #endif
        #if defined POWER_PINS && !defined EQUAL_POWER_PINS
        memcpy(&mux_pins, &mux_pins_r, sizeof(mux_pins_r));
        #endif

        #if AM_INIT_KEY_NUM > 0
        memcpy(&init_keys, &init_keys_r, sizeof(init_keys_r));
        memcpy(&init_functions, &init_functions_r, sizeof(init_functions_r));
        #endif

        //TODO: Test this
        #ifdef PRIORITY_INDEXES
        memcpy(&priority_indexes, &priority_indexes_r, sizeof(priority_indexes));
        priority_index_num = priority_index_num_r;
        #endif
    }
}
#endif // if KEYBOARD_SIDE == UNKNOWN
#endif // SPLIT_KEYBOARD


//MARK: Priority scan
#ifdef PRIORITY_MUXES
uint8_t matrix_scan_priority(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;
    uint8_t last_channel = 0;
    uint16_t adc_value = 0;
    set_mux_channel(0);

    //Priority mux is an array of adc_channel_index, mux_channel, matrix_index
    for(uint8_t index = 0; index < PRIORITY_MUX_NUM; index++) {
        uint8_t adc_channel = priority_muxes[index][0];
        uint8_t mux_channel = priority_muxes[index][1];
        uint8_t matrix_index = priority_muxes[index][2];

        #if CUSTOM_POWER_BEFORE_SCAN == TRUE
        set_sensor_power_high_kb(mux_channel);
        #endif
        if(mux_channel != last_channel) {
            #if POWER_BEFORE_SCAN == TRUE
            set_sensor_power(mux_channel);
            #endif
            #ifdef POWER_SELECT_DELAY
            delay_ns(POWER_SELECT_CYCLES);
            #endif
            set_mux_channel(mux_channel);
            #ifdef MUX_SELECT_DELAY
            delay_ns(MUX_SELECT_CYCLES);
            #endif
            last_channel = mux_channel;
        }

        adc_value = adc_read(adc_pin_mux[adc_channel]);
        #ifdef DEBUG_MUX_VALUE
        if(adc_channel == debug_mux[0] && mux_channel == debug_mux[1]) {
            dprintf("%u\n", adc_value);
        }
        #endif
        #ifdef ADC_SCAN_DELAY
        delay_ns(ADC_SCAN_CYCLES);
        #endif

        if(evaluate_value(matrix_index, adc_value)) {
            matrix_has_changed = true;
            #if DEBUG_SCAN_NO_INPUT != TRUE
            current_matrix[key_config[matrix_index].row] ^= 1 << key_config[matrix_index].col;
            #endif
        }

        #if CUSTOM_POWER_BEFORE_SCAN == TRUE
        set_sensor_power_low_kb(mux_channel);
        #endif
    }
    matrix_scan_kb();

    return matrix_has_changed;
}
#endif


//MARK: Suspend
void suspend_power_down_kb(void) {
    #ifdef MATRIX_POWER_PIN
    #ifndef INVERT_MATRIX_POWER
    gpio_write_pin_low(MATRIX_POWER_PIN);
    #else
    gpio_write_pin_high(MATRIX_POWER_PIN);
    #endif
    #endif

    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    #ifdef MATRIX_POWER_PIN
    #ifndef INVERT_MATRIX_POWER
    gpio_write_pin_high(MATRIX_POWER_PIN);
    #else
    gpio_write_pin_low(MATRIX_POWER_PIN);
    #endif
    wait_ms(5);
    #endif

    suspend_wakeup_init_user();
}





/* Weak defines, to allow a custom powering logic */
__attribute__((weak)) void sensor_power_init_kb(void) { sensor_power_init_user(); }

__attribute__((weak)) void set_sensor_power_high_kb(uint8_t mux_channel) { set_sensor_power_high_user(mux_channel); }

__attribute__((weak)) void set_sensor_power_low_kb(uint8_t mux_channel) { set_sensor_power_low_user(mux_channel); }

__attribute__((weak)) void sensor_power_toggle_kb(uint8_t mux_channel, uint8_t adc_channel) { sensor_power_toggle_user(mux_channel, adc_channel); }

__attribute__((weak)) void sensor_power_init_user(void) {}

__attribute__((weak)) void set_sensor_power_high_user(uint8_t mux_channel) {}

__attribute__((weak)) void set_sensor_power_low_user(uint8_t mux_channel) {}

__attribute__((weak)) void sensor_power_toggle_user(uint8_t mux_channel, uint8_t adc_channel) {}


//TODO: I think I only need these for the full replacement
/* Standard weak defines */
// __attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }

// __attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }

// __attribute__((weak)) void matrix_init_user(void) {}

// __attribute__((weak)) void matrix_scan_user(void) {}
