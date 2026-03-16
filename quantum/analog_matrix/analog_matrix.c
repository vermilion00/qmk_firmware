//TODO: Go through these and check which ones are needed

#include "analog_matrix.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "_wait.h"
#include "action_layer.h"
#include "bootloader.h"
#include "debug.h"
#include "info_config.h"
#include "joystick.h"
#include "keyboard.h"
#include "keycodes.h"
#include "keymap_introspection.h"
#include "matrix.h"
#include "multiplexer.h"
#include "print.h"
#include "suspend.h"
#include "math.h"
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
#ifndef AM_NO_EEPROM
analog_switch_t calibration_data[SMAX(SWITCH_NUM)];
#endif

//TODO: Testing, remove if unnecessary
#ifdef SPLIT_KEYBOARD
#include "transactions.h"
#endif

extern matrix_row_t matrix[MATRIX_ROWS];
#if DEBOUNCE > 0
extern matrix_row_t raw_matrix[MATRIX_ROWS];
#endif
#ifdef SPLIT_KEYBOARD
// Row offsets for each hand
extern uint8_t thisHand, thatHand;
bool calibration_started = false;
#endif

#ifdef DYNAMIC_CALIBRATION
// Keeps track of how many switches need updating, and saves new data once it exceeds RECALIBRATED_SWITCHES, to avoid writing to storage too often
__attribute__((unused)) uint8_t recalibrated_switches = 0;
#endif

#if AM_INIT_KEY_NUM > 0
// Initialize the keys to be checked at initialization
void _bootloader_jump(bool init);
SPLIT_MUTABLE uint8_t init_keys[AM_INIT_KEY_NUM][2] = AM_INIT_KEYS;
init_func_t init_functions[AM_INIT_KEY_NUM] = AM_INIT_FUNCTIONS;
#endif

#if defined DEBUG_MUX_POSITION
uint8_t debug_mux[2] = DEBUG_MUX_POSITION;
#endif

PROFILE_MUTABLE uint8_t active_profile = AM_DEFAULT_PROFILE;
uint8_t highest_layer = 0;

SPLIT_MUTABLE uint8_t switch_num = SWITCH_NUM;
SPLIT_MUTABLE uint8_t adc_pin_num = ADC_PIN_NUM;
SPLIT_MUTABLE uint8_t mux_channel_num = MUX_CHANNELS;
#ifdef MUX_PINS
//TODO: Add this stuff
SPLIT_MUTABLE uint8_t mux_pin_num = MUX_PIN_NUM;
#ifdef MUX_PINS_CONTINUOUS
SPLIT_MUTABLE uint8_t mux_offset = MUX_PIN_OFFSET;
#ifndef MCU_RP
SPLIT_MUTABLE gpio_port_t* mux_port = CONTINUOUS_MUX_PORT;
#endif
#endif
#endif


#ifdef POWER_PINS
SPLIT_MUTABLE uint8_t power_pin_num = POWER_PIN_NUM;
// Set the sensor power pins and delay, if defined
void set_sensor_power(uint8_t index);
#endif

#if defined SPLIT_KEYBOARD && defined AM_NO_EEPROM
uint8_t switch_num_slave = SWITCH_NUM_R;
#endif

//TODO: I'm pretty sure the right half can just be set to SWITCH_NUM_R, since we only copy SWITCH_NUM_R idxs over anyway
//      I think it's currently done like that to be able to copy them over easily, but I should still know how much to copy over anyway SWITCH_NUM_R * sizeof(float)
//      Also pretty sure it'd be less hassle defining stuff __attribute__((weak)) to let them be overridden with the MATRIX macros
#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
analog_key_t key_config[SMAX(SWITCH_NUM)];
uint8_t mux_to_num[SMAX(MUX_CHANNELS)][ADC_PIN_NUM] = MUX_TO_NUM;
const uint8_t mux_to_num_r[SMAX(MUX_CHANNELS)][ADC_PIN_NUM] = MUX_TO_NUM_R;
uint8_t num_to_matrix[SMAX(SWITCH_NUM)][2] = NUM_TO_MATRIX;
const uint8_t num_to_matrix_r[SMAX(SWITCH_NUM)][2] = NUM_TO_MATRIX_R;

#if defined KEY_MODES
__attribute__((weak)) uint8_t key_modes[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = KEY_MODES;
const uint8_t key_modes_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = KEY_MODES_R;
#else
// If they're not defined, assume they're set using the MATRIX macro, and copy stuff over from there at init
__attribute__((weak)) uint8_t key_modes[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
__attribute__((weak)) const uint8_t key_modes_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
#endif

#if defined USE_TRIGGER_HEIGHT
#if defined TRIGGER_HEIGHT
float trigger_height[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = TRIGGER_HEIGHT;
float release_height[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RELEASE_HEIGHT;
const float trigger_height_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = TRIGGER_HEIGHT_R;
const float release_height_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RELEASE_HEIGHT_R;
#else
float trigger_height[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
float release_height[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
__attribute__((weak)) const float release_height_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
#endif
#endif // if defined USE_TRIGGER_HEIGHT
#if defined USE_RT_DISTANCE
#if defined RT_PRESS_DISTANCE
float rt_press_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RT_PRESS_DISTANCE;
float rt_release_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RT_RELEASE_DISTANCE;
const float rt_press_distance_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RT_PRESS_DISTANCE_R;
const float rt_release_distance_r[AM_PROFILE_NUM][SMAX(SWITCH_NUM)] = RT_RELEASE_DISTANCE_R;
#else
float rt_press_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
float rt_release_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
__attribute__((weak)) const float rt_release_distance_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
#endif
#endif // if defined USE_RT_DISTANCE

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
const uint8_t init_keys_r[SMAX(AM_INIT_KEY_NUM)][2] = AM_INIT_KEYS_R;
const init_func_t init_functions_r[SMAX(AM_INIT_KEY_NUM)] = AM_INIT_FUNCTIONS_R;
#endif

//TODO: Does this even work? Has it been tested?
#ifdef PRIORITY_INDICES_R
const uint8_t priority_indices_r[SMAX(SWITCH_NUM)] = PRIORITY_INDICES_R;
const uint8_t priority_index_num_r = PRIORITY_INDEX_NUM_R;
#endif

#else // if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
// Either the keyboard isn't split, or the side has been set using the -s flag

analog_key_t key_config[SWITCH_NUM];
#ifdef MUX_PINS
SPLIT_MUTABLE pin_t mux_pins[MUX_PIN_NUM] = MUX_PINS;
#endif
SPLIT_MUTABLE pin_t adc_pins[ADC_PIN_NUM] = ADC_PINS;
#ifdef POWER_PINS
SPLIT_MUTABLE pin_t power_pins[POWER_PIN_NUM] = POWER_PINS;
#endif

// Used to translate from the ADC pin/Mux combination to the switch number
SPLIT_MUTABLE uint8_t mux_to_num[MUX_CHANNELS][ADC_PIN_NUM] = MUX_TO_NUM;
// Used to translate from the switch number to the QMK layout position
SPLIT_MUTABLE uint8_t num_to_matrix[SWITCH_NUM][2] = NUM_TO_MATRIX;

#ifdef KEY_MODES
// Define stuff as weak so that it can be overridden by defining it in the keymap using the MATRIX macro
// Split keyboards still need to set trigger_height_config etc instead of defining trigger_height directly
__attribute__((weak)) CONFIG_MUTABLE uint8_t key_modes[AM_PROFILE_NUM][SWITCH_NUM] = KEY_MODES;
#else
//TODO: How do I fix declaration before definiton?
//      I hope I don't need to put the side assign function into keymap introspection, too many issues with declarations
__attribute__((weak)) CONFIG_MUTABLE uint8_t key_modes[AM_PROFILE_NUM][SWITCH_NUM] = { [0 ... AM_PROFILE_NUM-1] = {[0 ... SWITCH_NUM-1] = 0} };
// __attribute__((weak)) const uint8_t key_modes_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
__attribute__((weak)) const uint8_t key_modes_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM] = { [0 ... AM_PROFILE_NUM-1] = {[0 ... TOTAL_SWITCH_NUM-1] = 0} };
#endif

#if defined USE_TRIGGER_HEIGHT
#if defined TRIGGER_HEIGHT
__attribute__((weak)) CONFIG_MUTABLE float trigger_height[AM_PROFILE_NUM][SWITCH_NUM] = TRIGGER_HEIGHT;
__attribute__((weak)) CONFIG_MUTABLE float release_height[AM_PROFILE_NUM][SWITCH_NUM] = RELEASE_HEIGHT;
#else
__attribute__((weak)) float trigger_height[AM_PROFILE_NUM][SWITCH_NUM];
__attribute__((weak)) float release_height[AM_PROFILE_NUM][SWITCH_NUM];
//TODO: Either add release height config or check some other way
// __attribute__((weak)) const float trigger_height_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM] = { [0 ... AM_PROFILE_NUM-1] = {[0 ... TOTAL_SWITCH_NUM-1] = 0} };
// __attribute__((weak)) const float release_height_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM] = { [0 ... AM_PROFILE_NUM-1] = {[0 ... TOTAL_SWITCH_NUM-1] = 0} };
//TODO: Alias and release height still needs testing
__attribute__((weak)) const float trigger_height_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
__attribute__((weak, alias("trigger_height_config"))) extern const float release_height_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
#endif
#endif // if defined USE_TRIGGER_HEIGHT

#if defined USE_RT_DISTANCE
#if defined RT_PRESS_DISTANCE
__attribute__((weak)) CONFIG_MUTABLE float rt_press_distance[AM_PROFILE_NUM][SWITCH_NUM] = RT_PRESS_DISTANCE;
__attribute__((weak)) CONFIG_MUTABLE float rt_release_distance[AM_PROFILE_NUM][SWITCH_NUM] = RT_RELEASE_DISTANCE;
#else
__attribute__((weak)) float rt_press_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
__attribute__((weak)) float rt_release_distance[AM_PROFILE_NUM][SMAX(SWITCH_NUM)];
// __attribute__((weak)) const float rt_release_distance_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
__attribute__((weak)) const float rt_press_distance_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
__attribute__((weak, alias("rt_press_distance_config"))) extern const float rt_release_distance_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM];
#endif
#endif
#endif // if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN else

// During initialization, the adc pins are translated to the adc mux combination that the adc_read function uses
adc_mux adc_pin_mux[ADC_PIN_NUM];

//TODO: Assign correct side at init
// This is currently unused in favor of PRIORITY_INDICES
#ifdef PRIORITY_MUXES
uint8_t scan_amt = 0;
uint8_t priority_muxes[PRIORITY_MUX_NUM][3] = PRIORITY_MUXES;
uint8_t matrix_scan_priority(matrix_row_t current_matrix[]);
#endif

#if defined PRIORITY_INDICES || defined SLAVE_LOW_PRIORITY
uint8_t scan_amt = 0;
#ifdef PRIORITY_INDICES
uint8_t priority_indices[SMAX(SWITCH_NUM)] = PRIORITY_INDICES;
SPLIT_MUTABLE uint8_t priority_index_num = PRIORITY_INDEX_NUM;
#endif
#endif


//MARK: Init
__attribute__((weak)) void analog_matrix_init(void) {
    // Determine keyboard half, and assign heights if keymap config is used
    #if (KEYBOARD_SIDE == UNKNOWN && defined SPLIT_KEYBOARD) || defined KEYMAP_CONFIG
    assign_config(is_keyboard_left());
    #endif

    for(uint8_t i = 0; i < adc_pin_num; i++) {
        palSetLineMode(adc_pins[i], PAL_MODE_INPUT_ANALOG);
        // Convert adc pins to adc mux combination
        adc_pin_mux[i] = pinToMux(adc_pins[i]);
    }

    #ifdef MUX_PINS
    for(uint8_t i = 0; i < mux_pin_num; i++) {
        gpio_set_pin_output_push_pull(mux_pins[i]);
        gpio_write_pin_low(mux_pins[i]);
    }
    #endif

    // POWER_BEFORE_SCAN stuff hasn't really been tested yet
    #if defined POWER_BEFORE_SCAN
    for(uint8_t i = 0; i < power_pin_num; i++) {
        gpio_set_pin_output_push_pull(power_pins[i]);
        gpio_write_pin_low(power_pins[i]);
    }

    #elif defined CUSTOM_POWER_BEFORE_SCAN
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

    //TODO: Add mode to check init keys based on deviation larger than the avg diff between them + some value

    // Get the min/max values of each switch
    if(!get_calibration_data()) {
    // If loading the calibration data fails, start calibration
        calibrate_switches(false);
    }

    //TODO: Make sure only having keys for one half defined doesn't cause problems
    //      If only keys for one half are defined, they should be used for both halves
    // Check keys like the calibration or bootmagic key before scanning begins
    #if AM_INIT_KEY_NUM > 0
    scan_init_keys();
    #endif

    // Translate the trigger height etc into the equivalent ADC value
    for(uint8_t index = 0; index < switch_num; index++) {
        translate_mm_to_value(index);
    }

    #if defined SPLIT_LAYER_SYNC
    // #if defined SPLIT_LAYER_SYNC || defined PRIORITY_INDICES
    change_layer_settings(highest_layer);
    #endif

    #if defined SPLIT_KEYBOARD && defined AM_NO_EEPROM
    if (is_keyboard_master()) {
        if (!is_keyboard_left()) switch_num_slave = SWITCH_NUM_L;
    } else if (is_keyboard_left()) switch_num_slave = SWITCH_NUM_L;
    #endif

    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}


//MARK: Init keys
#if AM_INIT_KEY_NUM > 0
void scan_init_keys(void) {
    uint16_t adc_value;
    // Dummy read, as first read is always way off
    adc_value = adc_read(adc_pin_mux[init_keys[0][0]]);
    // Long-ish delay needed for correct init key reading after being plugged in
    wait_ms(AM_STARTUP_DELAY);

    //TODO: AM_INIT_KEY_NUM should be replaced by a var here, or just assign an empty
    for(uint8_t idx = 0; idx < SMAX(AM_INIT_KEY_NUM); idx++) {
        #ifdef POWER_BEFORE_SCAN
        gpio_write_pin_high(power_pins[init_keys[idx][1]]);
        #elif defined CUSTOM_POWER_BEFORE_SCAN
        set_sensor_power_high_kb(init_keys[idx][1]);
        #endif // CUSTOM_POWER_BEFORE_SCAN

        set_mux_channel(init_keys[idx][1]);
        #ifdef MUX_SELECT_DELAY
        delay_ns(MUX_SELECT_CYCLES);
        #endif // MUX_SELECT_DELAY

        const uint8_t matrix_index = mux_to_num[init_keys[idx][1]][init_keys[idx][0]];
        wait_ms(AM_STARTUP_DELAY / 5);
        adc_value = adc_read(adc_pin_mux[init_keys[idx][0]]);

        #ifdef POWER_BEFORE_SCAN
        gpio_write_pin_low(power_pins[init_keys[idx][1]]);
        #elif defined CUSTOM_POWER_BEFORE_SCAN
        set_sensor_power_low_kb(init_keys[idx][1]);
        #endif // CUSTOM_POWER_BEFORE_SCAN

        // If the key is activated, call the respective function
        // These functions are set in the INIT_FUNCTIONS dict at the top of analog_matrix.py
        #ifndef INVERT_ADC
        if(adc_value < key_config[matrix_index].bottom_value + 4 * ADC_BOTTOM_DEADZONE) {
            LED_ON;
            init_functions[idx](true);
        }
        #else
        if(adc_value > key_config[matrix_index].bottom_value - 4 * ADC_BOTTOM_DEADZONE) {
            LED_ON;
            (*init_functions[idx])(true);
        }
        #endif
    }
}
#else // AM_INIT_KEY NUM > 0
#define scan_init_keys()
#endif // AM_INIT_KEY NUM > 0 else


//MARK: Scan
__attribute__((weak)) uint8_t analog_matrix_scan(void) {
    bool matrix_has_changed = false;
    uint16_t adc_value;
    uint8_t index;

    #ifdef PRIORITY_MUXES
    if(profiles[active_profile].priority_profile && scan_amt < PRIORITY_LEVEL) {
        scan_amt += 1;
        return matrix_scan_priority(current_matrix);
    } else { scan_amt = 0; }
    #endif

    for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
        #if defined POWER_BEFORE_SCAN
        set_sensor_power(mux_channel);
        #elif defined CUSTOM_POWER_BEFORE_SCAN
        set_sensor_power_high_kb(mux_channel);
        #endif

        set_mux_channel(mux_channel);
        #ifdef MUX_SELECT_DELAY
        delay_ns(MUX_SELECT_CYCLES);
        #endif

        for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
            // Translate matrix mux and adc channels to matrix position
            index = mux_to_num[mux_channel][adc_channel];
            // Check if a switch is at the position (matrix index < 255), and scan if so
            if(index == 255) {
                #ifdef DEBUG_SCAN_VALUES
                else {
                    dprintf("%u/%2u:    , ",adc_channel, mux_channel);
                }
                #endif
                continue;
            }

            #ifdef PRIORITY_INDICES
            if(profiles[active_profile].priority_profile && scan_amt < PRIORITY_LEVEL) {
                if(!priority_indices[index]) { continue; }
            }
            #endif

            adc_value = adc_read(adc_pin_mux[adc_channel]);

            // Simple IIR filter with 1/4 alpha by default
            ADC_FILTER(adc_value, index);

            #ifdef DEBUG_MUX_POSITION
            if(adc_channel == debug_mux[0] && mux_channel == debug_mux[1]) {
                dprintf("%u\n", adc_value);
            }
            #elif defined DEBUG_SCAN_VALUES
            dprintf("%u/%2u: %3u, ", adc_channel, mux_channel, adc_value);
            #endif
            #ifdef ADC_SCAN_DELAY
            delay_ns(ADC_SCAN_CYCLES);
            #endif

            // Check if the value has changed enough to warrant an evaluation
            //TODO: This could perhaps cause issues around the borders, with missing updates, especially on higher smoothing levels
            if ((adc_value < (key_config[index].scan_value + ADC_SMOOTHING)) && (adc_value > (key_config[index].scan_value - ADC_SMOOTHING))) continue;
            // Check if key is pressed/released, returns true if the switch state has changed
            if(evaluate_value(index, adc_value)) {
                matrix_has_changed = true;
                #ifndef DEBUG_SCAN_NO_INPUT
                matrix[key_config[index].row] ^= 1 << key_config[index].col;
                #endif
            }

            // If dynamic calibration is enabled, check if the boundaries need updating
            #ifdef DYNAMIC_CALIBRATION
            if(update_switch_bounds(index, adc_value)) {
                // If the bounds have been updated, translate the heights
                translate_mm_to_value(index);
                recalibrated_switches += 1;
            }
            #endif // ifdef DYNAMIC_CALIBRATION
            key_config[index].scan_value = adc_value;
        }
        #ifdef CUSTOM_POWER_BEFORE_SCAN
        set_sensor_power_low_kb(mux_channel);
        #endif
    }

    #ifdef DEBUG_SCAN_VALUES
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
    #if !defined SLAVE_LOW_PRIORITY
    matrix_has_changed |= matrix_post_scan();
    #else // Only synchronise the matrices during the full scans if the slave is low priority (doesn't contain priority keys)
    if(profiles[active_profile].priority_profile) {
        if(scan_amt == 0) {
            matrix_has_changed |= matrix_post_scan();
        }
    } else {
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
// Translate the heights of all keys into the corresponding ADC values
void translate_mm_to_value(uint8_t index) {
    #ifdef INVERT_ADC
    const uint16_t top_value = key_config[index].top_value - ADC_TOP_DEADZONE;
    const uint16_t bottom_value = key_config[index].bottom_value + ADC_BOTTOM_DEADZONE;
    const uint16_t travel_unit = floor((float)(bottom_value - top_value) / (float)TRAVEL_DISTANCE);
    #else
    // Take out the deadzones here, since we want to calculate the travel unit for the entire range
    const uint16_t top_value = key_config[index].top_value + ADC_TOP_DEADZONE;
    const uint16_t bottom_value = key_config[index].bottom_value - ADC_BOTTOM_DEADZONE;
    const uint16_t travel_unit = floor((float)(top_value - bottom_value) / (float)TRAVEL_DISTANCE);
    #endif

    for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
        #ifndef INVERT_ADC
        #ifdef USE_TRIGGER_HEIGHT
        #ifndef DISTANCE_FROM_BOTTOM
        //TODO: Instead of having a separate section, I can just subtract the height from the TRAVEL_DISTANCE to get the from_bottom value
        //TODO: Maybe instead of adjusting the converted adc value, instead adjust the height setting? like 1.0mm -> 0.7mm, 3.0mm -> 3.5mm
        //TODO: Test this, add INVERT_ADC values
        key_config[index].trigger_value[profile] = adjust(top_value - (travel_unit * trigger_height[profile][index])) - ADC_SMOOTHING;
        key_config[index].release_value[profile] = adjust(top_value - (travel_unit * release_height[profile][index])) + ADC_SMOOTHING;

        #else // ifndef DISTANCE_FROM_BOTTOM
        key_config[index].trigger_value[profile] = adjust(travel_unit * trigger_height[profile][index] + bottom_value) - ADC_SMOOTHING;
        key_config[index].release_value[profile] = adjust(travel_unit * release_height[profile][index] + bottom_value) + ADC_SMOOTHING;
        #endif // ifndef DISTANCE_FROM_BOTTOM else
        #endif

        #else // ifndef INVERT_ADC
        //Inverted ADC -> Lower switch means higher value

        #if defined USE_TRIGGER_HEIGHT
        #ifndef DISTANCE_FROM_BOTTOM
        //TODO: Is this correct
        key_config[index].trigger_value[profile] = adjust(top_value + (travel_unit * trigger_height[profile][index])) + ADC_SMOOTHING;
        key_config[index].release_value[profile] = adjust(top_value + (travel_unit * release_height[profile][index])) - ADC_SMOOTHING;

        #else // ifndef DISTANCE_FROM_BOTTOM
        key_config[index].trigger_value[profile] = adjust(bottom_value - (travel_unit * trigger_height[profile][index])) + ADC_SMOOTHING;
        key_config[index].release_value[profile] = adjust(bottom_value - (travel_unit * release_height[profile][index])) - ADC_SMOOTHING;
        #endif // ifndef DISTANCE_FROM_BOTTOM else
        #endif // if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
        #endif // ifndef INVERT_ADC else

        #if defined USE_RT_DISTANCE
        //TODO: The threshold starts at 0 because there's only one, instead of one per profile, so I need to set it every profile switch(?)
        key_config[index].rt_press_value[profile] = travel_unit * rt_press_distance[profile][index];
        key_config[index].rt_release_value[profile] = travel_unit * rt_release_distance[profile][index] + ADC_SMOOTHING;
        #endif

        // Assign the switch mode
        key_config[index].mode[profile] = key_modes[profile][index];
    }
}


//MARK: Get calibration
bool get_calibration_data(void) {
    #ifdef FORCE_CALIBRATE
    return false;
    #endif

#   ifdef AM_NO_EEPROM
    #if !defined AM_TOP_VALUES || !defined AM_BOTTOM_VALUES
    return false;
    #endif

    #if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
    uint16_t top_values[SMAX(SWITCH_NUM)] = AM_TOP_VALUES;
    uint16_t bottom_values[SMAX(SWITCH_NUM)] = AM_BOTTOM_VALUES;

    if(!is_keyboard_left()) {
        const uint16_t top_values_r[SMAX(SWITCH_NUM)] = AM_TOP_VALUES_R;
        const uint16_t bottom_values_r[SMAX(SWITCH_NUM)] = AM_BOTTOM_VALUES_R;
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

    uint16_t adjustment = 0;
    for(uint8_t key = 0; key < switch_num; key++) {
    #ifndef INVERT_ADC
    #ifdef DYNAMIC_CALIBRATION
        adjustment = AM_DC_FACTOR * (top_values[key] - bottom_values[key]);
    #endif
        key_config[key].top_value = top_values[key] - ADC_TOP_DEADZONE - adjustment;
        key_config[key].bottom_value = bottom_values[key] + ADC_BOTTOM_DEADZONE + adjustment;
    #else // ifndef INVERT_ADC
    #ifdef DYNAMIC_CALIBRATION
        adjustment = AM_DC_FACTOR * (bottom_values[key] - top_values[key]);
    #endif
        key_config[key].top_value = top_values[key] + ADC_TOP_DEADZONE + adjustment;
        key_config[key].bottom_value = bottom_values[key] - ADC_BOTTOM_DEADZONE - adjustment;
        #endif // ifndef INVERT_ADC else
        // Prime the filter to avoid large swings at the start
        key_config[key].scan_value = key_config[key].top_value;
    }

    return true;

#   else //ifdef AM_NO_EEPROM

    // Read the calibration data from EEPROM
    eeconfig_read_keyboard((analog_switch_t*)&calibration_data);

    for(uint8_t key = 0; key < switch_num; key++) {
        uint16_t adjustment = 0;
        // Check if the switch data makes sense, start calibration if not
        // A fake mixed matrix will have top/bottom values of 4095 and 0
        if(calibration_data[key].top_value < 5 && calibration_data[key].bottom_value < 5)  return false;
        if(calibration_data[key].top_value > 4000 && calibration_data[key].bottom_value > 4000)  return false;
        // No switch can have a valid value above 4095 due to the 12 bit ADC resolution
        if(calibration_data[key].top_value > 4096 || calibration_data[key].bottom_value > 4096) return false;

        #ifndef INVERT_ADC
        if(calibration_data[key].top_value <= calibration_data[key].bottom_value) return false;
        if(calibration_data[key].top_value - calibration_data[key].bottom_value < 7 * ADC_TOP_DEADZONE) return false;
        #ifdef DYNAMIC_CALIBRATION
        adjustment = AM_DC_FACTOR * (calibration_data[key].top_value - calibration_data[key].bottom_value);
        #endif
        // As we already scanned each key once before this, we can use that value to skip the recalculation
        uint16_t top_val = calibration_data[key].top_value - ADC_TOP_DEADZONE - adjustment;
        if(key_config[key].top_value < top_val) {
            key_config[key].top_value = top_val;
            key_config[key].scan_value = top_val;
        }
        key_config[key].bottom_value = calibration_data[key].bottom_value + ADC_BOTTOM_DEADZONE + adjustment;

        #else
        if(calibration_data[key].top_value >= calibration_data[key].bottom_value) return false;
        if(calibration_data[key].bottom_value - calibration_data[key].top_value < 7 * ADC_TOP_DEADZONE) return false;
        #ifdef DYNAMIC_CALIBRATION
        adjustment = AM_DC_FACTOR * (calibration_data[key].bottom_value - calibration_data[key].top_value);
        #endif
        uint16_t top_val = calibration_data[key].top_value + ADC_TOP_DEADZONE + adjustment;
        if(key_config[key].top_value > top_val) {
            key_config[key].top_value = top_val;
            key_config[key].scan_value = top_val;
        }
        key_config[key].bottom_value = calibration_data[key].bottom_value - ADC_BOTTOM_DEADZONE - adjustment;
        #endif

        key_config[key].scan_value = key_config[key].top_value;

    }
    return true;
    #endif // ifdef AM_NO_EEPROM else
}


//MARK: Calibrate
void calibrate_switches(bool init) {
    uint16_t adc_value;
    uint8_t matrix_index;
    // Save config values after x scans without a change
    uint32_t scans_without_change = 0;
    bool first_scan = true;

    #ifdef AM_NO_EEPROM
    char side[7] = "";
    #endif

    #ifdef SPLIT_KEYBOARD
    if(is_keyboard_master()) {
        #ifdef AM_NO_EEPROM
        if(!is_keyboard_left()) {
            char right[] = "_right";
            memcpy(&side, &right, sizeof(right));
        }
        #endif

        calibration_started = true;
        // Force a synchronisation so that the slave starts calibrating as well
        uint8_t tries = 0;
        while (tries < 50) {
            // Start the transaction manually
            if (!am_data_manual_transaction()) {
                wait_ms(10);
                tries++;
            } else { break; }
        }
    }
    #endif

    while(true) {
        LED_ON;
        for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
            #if defined DEBUG_CALIBRATION || defined DEBUG_SCAN_VALUES
            dprintf("Mux: %i\n", mux_channel);
            #endif
            #ifdef POWER_BEFORE_SCAN
            set_sensor_power(mux_channel);
            #ifdef POWER_SELECT_DELAY
            delay_ns(POWER_SELECT_CYCLES);
            #endif
            #elif defined CUSTOM_POWER_BEFORE_SCAN
            set_sensor_power_high_kb(mux_channel);
            #endif
            set_mux_channel(mux_channel);
            #ifdef MUX_SELECT_DELAY
            delay_ns(MUX_SELECT_CYCLES);
            #endif

            for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
                // Translate matrix mux and adc channels to matrix position
                matrix_index = mux_to_num[mux_channel][adc_channel];
                // Check if a switch is at the position (matrix index < 255), and scan if so
                if(matrix_index == 255) continue;

                adc_value = adc_read(adc_pin_mux[adc_channel]);

                if(first_scan) {
                    #ifdef INVERT_ADC
                    key_config[matrix_index].top_value = adc_value;
                    key_config[matrix_index].bottom_value = adc_value + ADC_BOTTOM_DEADZONE;
                    #else
                    key_config[matrix_index].top_value = adc_value;
                    key_config[matrix_index].bottom_value = adc_value - ADC_BOTTOM_DEADZONE;
                    #endif
                    key_config[matrix_index].scan_value = adc_value;

                    continue;
                }

                //TODO: I need to make sure that scan_value is primed, else high filters will cause issues
                ADC_FILTER(adc_value, matrix_index);
                key_config[matrix_index].scan_value = adc_value;

                #if defined DEBUG_CALIBRATION || defined DEBUG_SCAN_VALUES
                dprintf("%u/%2u: %i, ", adc_channel, mux_channel, adc_value);
                #endif

                #ifdef INVERT_ADC
                if(adc_value < key_config[matrix_index].top_value){
                    key_config[matrix_index].top_value = adc_value;
                    scans_without_change = 0;
                    #ifdef DEBUG_CALIBRATION
                    dprintf("key %i: new top value %i\n", matrix_index, adc_value);
                    #endif

                } else if (adc_value > key_config[matrix_index].bottom_value) {
                    key_config[matrix_index].bottom_value = adc_value;
                    scans_without_change = 0;
                    #ifdef DEBUG_CALIBRATION
                    dprintf("key %i: new bottom value %i\n", matrix_index, adc_value);
                    #endif

                } else if(key_config[matrix_index].bottom_value - key_config[matrix_index].top_value < 7 * ADC_TOP_DEADZONE) {
                    scans_without_change = 0;
                }

                #else // ifdef INVERT_ADC
                if(adc_value > key_config[matrix_index].top_value){
                    key_config[matrix_index].top_value = adc_value;
                    scans_without_change = 0;
                    #ifdef DEBUG_CALIBRATION
                    dprintf("Key %i: new top value %i\n", matrix_index, adc_value);
                    #endif

                } else if (adc_value < key_config[matrix_index].bottom_value) {
                    key_config[matrix_index].bottom_value = adc_value;
                    scans_without_change = 0;
                    #ifdef DEBUG_CALIBRATION
                    dprintf("Key %i: new bottom value %i\n", matrix_index, adc_value);
                    #endif

                // Check if new valid calibration values have been saved for this key
                } else if(key_config[matrix_index].top_value - key_config[matrix_index].bottom_value < (7 * ADC_TOP_DEADZONE)) {
                    scans_without_change = 0;
                }
                #endif // ifdef INVERT_ADC else
                //TODO: Check how much the calibrated values deviate from the scan values, would matching the time help?
                // Dummy evaluation, so that a calibration cycle takes about as long as a scan cycle
                evaluate_value(matrix_index, adc_value);
            }

            #if defined DEBUG_CALIBRATION || defined DEBUG_SCAN_VALUES
            dprint("\n");
            #endif
            #if defined CUSTOM_POWER_BEFORE_SCAN
            set_sensor_power_low_kb(mux_channel);
            #endif
        }

        first_scan = false;
        scans_without_change += 1;
        //TODO: Change this to a more precise method
        if(scans_without_change >= SCANS_WITHOUT_CHANGE){
            // Calibration is done
            LED_OFF;
            #ifdef SPLIT_KEYBOARD
            calibration_started = false;
            #endif
            // Clear matrix and layer state to avoid stuck keys
            void reset_matrix_keys(void);
            reset_matrix_keys();
            layer_state_t reset_layer_state(void);
            layer_state = reset_layer_state();

            #ifndef AM_NO_EEPROM
            for(uint8_t key = 0; key < switch_num; key++) {
                // Save the calibration values without a deadzone applied
                calibration_data[key].top_value = key_config[key].top_value;
                calibration_data[key].bottom_value = key_config[key].bottom_value;
            }
            eeconfig_update_keyboard((analog_switch_t*)&calibration_data);

            #else // ifndef AM_NO_EEPROM
            #ifdef SPLIT_KEYBOARD
            if(!init) {
                void sync_calibration_values(bool init);
                sync_calibration_values(init);
            }
            #endif

            // Print the calibration values of each switch so that they can be adjusted in the config
            printf("\"top_values%s\":    [ %u", side, key_config[0].top_value);
            key_config[0].pressed = false;
            for(uint8_t index = 1; index < switch_num; index++){
                printf(", %u", key_config[index].top_value);
                key_config[index].pressed = false;
            }

            printf(" ],\n\"bottom_values%s\": [ %u", side, key_config[0].bottom_value);
            for(uint8_t index = 1; index < switch_num; index++){
                printf(", %u", key_config[index].bottom_value);
            }
            print(" ],\nPaste these lines into the hall_effect.config object in keyboard.json.\n\n");

            #endif // ifndef AM_NO_EEPROM else

            // Apply deadzones
            for(uint8_t key = 0; key < switch_num; key++) {
                #ifndef INVERT_ADC
                key_config[key].top_value -= ADC_TOP_DEADZONE;
                key_config[key].bottom_value += ADC_BOTTOM_DEADZONE;
                #else
                key_config[key].top_value += ADC_TOP_DEADZONE;
                key_config[key].bottom_value -= ADC_BOTTOM_DEADZONE;
                #endif
            }

            break;
        }
    }
}


//This is run from AM_PRNT for testing purposes
void _sync_cal(void) {
    #if defined SPLIT_KEYBOARD && defined AM_NO_EEPROM
    sync_calibration_values(false);
    #endif
}


//MARK: Evaluate
bool evaluate_value(uint8_t index, uint16_t value) {
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

#ifdef INVERT_ADC
    switch(key_config[index].mode[active_profile]) {
        #if defined USE_NONE
        case none:
        if(value > key_config[index].trigger_value[active_profile] || value > key_config[index].bottom_value) {
            key_config[index].pressed = true;
        } else if(value < key_config[index].release_value[active_profile] || value < key_config[index].top_value) {
            key_config[index].pressed = false;
        } else {
            return false;
        }
        break;
        #endif //defined RAPID_TRIGGER

        #if defined USE_RAPID_TRIGGER
        case rapid_trigger:
        // Rapid trigger is only active when the switch is lower than the trigger and release height
        if(value > key_config[index].trigger_value[active_profile]) {
            // Set the new lowest value if needed
            if(value > key_config[index].rt_threshold + ADC_SMOOTHING || value > key_config[index].bottom_value) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value + key_config[index].rt_threshold) < key_config[index].rt_release_value[active_profile] || value < key_config[index].top_value) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value + key_config[index].rt_press_value[active_profile];
            }
        } else if(value < key_config[index].release_value[active_profile] || value < key_config[index].top_value) {
            // If the switch is not pressed past the threshold, reset it
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
        }
        break;
        #endif

        #if defined USE_CONTINUOUS_RAPID_TRIGGER
        case continuous_rapid_trigger:
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
        break;
        #endif

        #if defined USE_CONSTANT_RAPID_TRIGGER
        case constant_rapid_trigger:
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
        break;
        #endif

        default:
        return false;
    }

#else // ifdef INVERT_ADC
    switch(key_config[index].mode[active_profile]) {
        #if defined USE_NONE
        case none:
        if(value < key_config[index].trigger_value[active_profile] || value < key_config[index].bottom_value) {
            key_config[index].pressed = true;
        } else if(value > key_config[index].release_value[active_profile] || value > key_config[index].top_value) {
            key_config[index].pressed = false;
        } else {
            return false;
        }
        break;
        #endif //defined USE_NONE

        #if defined USE_RAPID_TRIGGER
        case rapid_trigger:
        // Rapid trigger is only active when the switch is lower than the trigger and release height
        if(value < key_config[index].trigger_value[active_profile]) {
            // Set the new lowest value if needed
            if(value < key_config[index].rt_threshold - ADC_SMOOTHING || value < key_config[index].bottom_value) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value - key_config[index].rt_threshold) > key_config[index].rt_release_value[active_profile]) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value - key_config[index].rt_press_value[active_profile];
            }
        //TODO: Check if it is faster to check if the key state is released, and only set values if they're not set already
        } else if(value > key_config[index].release_value[active_profile] || value > key_config[index].top_value) {
            // If the switch is not pressed past the threshold, reset it
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
        }
        break;
        #endif // defined USE_RAPID_TRIGGER

        #if defined USE_CONTINUOUS_RAPID_TRIGGER
        case continuous_rapid_trigger:
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
        break;
        #endif // defined USE_CONTINUOUS_RAPID_TRIGGER

        #if defined USE_CONSTANT_RAPID_TRIGGER
        case constant_rapid_trigger:
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
        break;
        #endif // defined USE_CONSTANT_RAPID_TRIGGER

        default:
        return false;
    }
#endif // ifdef INVERT_ADC else

    return !(prev_pressed == key_config[index].pressed);
}


//MARK: Update bounds
#ifdef DYNAMIC_CALIBRATION
#ifdef DEBUG_CALIBRATION
#define OUTPUT_VAL(value, side, index) dprintf("New %s value for key %u: %u\n", side, index, value)
#else
#define OUTPUT_VAL(value, side, index)
#endif
bool update_switch_bounds(uint8_t index, uint16_t value) {
    #ifndef INVERT_ADC
    if(value >= (key_config[index].top_value + AM_DC_DELTA + ADC_TOP_DEADZONE)){
        key_config[index].top_value = value - ADC_TOP_DEADZONE;
        calibration_data[index].top_value = value;
        OUTPUT_VAL(value, "top", index);
        return true;
    } else if (value <= (key_config[index].bottom_value - AM_DC_DELTA - ADC_BOTTOM_DEADZONE)) {
        key_config[index].bottom_value = value + ADC_BOTTOM_DEADZONE;
        calibration_data[index].bottom_value = value;
        OUTPUT_VAL(value, "bottom", index);
        return true;
    }

    #else // ifndef INVERT_ADC
    if(value < key_config[index].top_value - AM_DC_DELTA - ADC_TOP_DEADZONE){
        key_config[index].top_value = value + ADC_TOP_DEADZONE;
        OUTPUT_VAL(value, "top", index);
        return true;
    } else if (value > key_config[index].bottom_value + AM_DC_DELTA + ADC_BOTTOM_DEADZONE) {
        key_config[index].bottom_value = value - ADC_BOTTOM_DEADZONE;
        OUTPUT_VAL(value, "top", index);
        return true;
    }
    #endif // ifndef INVERT_ADC else

    return false;
}
#endif


//MARK: Switch data
// Populates the key matrix with the static config params, heights are populated separately
void get_switch_data(void) {
    // Dummy reads because the first reads are wrong
    adc_read(adc_pin_mux[0]);
    adc_read(adc_pin_mux[0]);

    // for(uint8_t key = 0; key < switch_num; key++) {
    //     uint8_t row = num_to_matrix[key][0];
    //     uint8_t col = num_to_matrix[key][1];
    //     key_config[key].row = row;
    //     key_config[key].col = col;

    //     for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++){
    //         key_config[key].mode[profile] = key_modes[profile][key];
    //     }

    //     #if defined JOYSTICK_ENABLE
    //     key_config[key].axis_index = -1;
    //     #endif
    // }
    for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
        //TODO: Add delays
        set_mux_channel(mux_channel);
        for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
            const uint8_t index = mux_to_num[mux_channel][adc_channel];
            if(index == 255) continue;

            uint16_t adc_value = adc_read(adc_pin_mux[adc_channel]);
            key_config[index].top_value = adc_value;
            // Assign this value here to prime the filter
            key_config[index].scan_value = adc_value;

            key_config[index].row = num_to_matrix[index][0];
            key_config[index].col = num_to_matrix[index][1];

            for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++){
                key_config[index].mode[profile] = key_modes[profile][index];
            }

            #if defined JOYSTICK_ENABLE
            key_config[key].axis_index = -1;
            #endif
        }
    }
}


//MARK: Bootmagic
// Helper functions because the current init key implementation requires taking a bool for the calibration function
void _bootmagic(bool init) {
    eeconfig_disable();
    bootloader_jump();
}

void _bootloader_jump(bool init) {
    bootloader_jump();
}


//TODO: Remove this and the call in keyboard.c at the start
//MARK: Force bootloader
//TODO: Do actual init key checking here?
// void _force_bootloader(void) {
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
    //     bootloader_jump();
    // }
// }


//MARK: Power pins
#ifdef POWER_PINS
void set_sensor_power(uint8_t index) {
    #ifdef POWER_PINS_CONTINUOUS
    #ifdef USE_BSRR
    #if defined MCU_AT32
    // Set action takes priority
    CONTINUOUS_POWER_PORT->SCR.W = (((1 << POWER_PIN_NUM) - 1) << POWER_PIN_OFFSET << 16) | (1 << (index + POWER_PIN_OFFSET));
    #elif defined MCU_RP
    //TODO: Test RP2040 optimizations
    *((volatile uint64_t *)GPIO_OUT_SET) = (((uint64_t)(1 << POWER_PIN_NUM) - 1) << POWER_PIN_OFFSET << 16) | (1 << (index + POWER_PIN_OFFSET));
    #else // ifdef mcutype
    CONTINUOUS_POWER_PORT->BSRR.W = (((1 << POWER_PIN_NUM) - 1) << POWER_PIN_OFFSET << 16) | (1 << (index + POWER_PIN_OFFSET));
    #endif // ifdef mcutype else
    #else // ifdef USE_BSRR
    //TODO: Does this actually work? Maybe I can use XOR to set and reset in one call?
    CONTINUOUS_POWER_PORT->ODR = (CONTINUOUS_POWER_PORT->ODR & ~(((1 << POWER_PIN_NUM) - 1) << POWER_PIN_OFFSET)) | (1 << (index + POWER_PIN_OFFSET));
    #endif // ifdef USE_BSRR
    #else // ifdef POWER_PINS_CONTINUOUS

    if(index == 0) {
        gpio_write_pin_low(power_pins[POWER_PIN_NUM - 1]);
        gpio_write_pin_high(power_pins[0]);
    } else {
        gpio_write_pin_low(power_pins[index-1]);
        gpio_write_pin_high(power_pins[index]);
    }
    #endif // ifdef POWER_PINS_CONTINUOUS else

    #ifdef POWER_SELECT_DELAY
    delay_ns(POWER_SELECT_CYCLES);
    #endif
}
#endif // ifdef POWER_PINS


//MARK: Delay
//TODO: Scale this so that it's roughly correct
void delay_ns(uint16_t delay) {
    // delay = (delay * 1000000000 / 180000000000);
    for(; delay > 0; delay--){
        __asm("");
    }
}


//MARK: Profiles
// If true, layer_state_set doesn't update the profile together with the layer.
bool manual_profile_lock = false;

uint8_t get_active_profile(void) { return active_profile; }

void toggle_profile_lock(void) { manual_profile_lock = !manual_profile_lock; }
void set_profile_lock(bool value) { manual_profile_lock = value; }

#if AM_PROFILE_NUM > 1
void profile_state_changed(uint8_t profile);
#ifndef SPLIT_KEYBOARD
void set_active_profile(uint8_t profile) { active_profile = profile; }
#else // ifndef SPLIT_KEYBOARD
void set_active_profile(uint8_t profile) {
    if(is_keyboard_master()) {
        active_profile = profile;

        // Start the transaction manually
        am_data_manual_transaction();

        //TODO: Call profile setting change function here, and call manual transaction from there
        profile_state_changed(profile);
    }
}
#endif // ifndef SPLIT_KEYBOARD else
#else // if AM_PROFILE_NUM > 1
#define set_active_profile(profile)
#endif // if AM_PROFILE_NUM > 1

#if AM_PROFILE_NUM > 1
// Makes all changes that need to happen on a profile change
void profile_state_changed(uint8_t profile) {
    //TODO: If am_data_manual_transaction is split up into many separate ones for each piece of data, send the profile change here
    // am_profile_manual_transaction();

    //TODO: Make sure no conflicts are caused with features remapping key modes (probably doesn't happen anyway)

}

#else
#define profile_state_change(profile)
#endif


void reset_matrix_keys(void) {
    for(uint8_t index = 0; index < switch_num; index++) {
        key_config[index].pressed = false;
    }
    memset(&matrix, 0, sizeof(matrix));
}


layer_state_t reset_layer_state(void) {
    return default_layer_state;
}


//TODO: Make an equivalent for profile changes
//MARK: Layer state
layer_state_t layer_state_set_am(layer_state_t state) {
    highest_layer = get_highest_layer(state);

    #if defined JOYSTICK_ENABLE && !defined USE_JOYSTICK
    if(joystick_layer) { reset_joystick_keys(); }
    #endif
    #if defined MIDI_ENABLE
    if(midi_layer) { reset_midi_keys(); }
    #endif
    #if defined SPLIT_LAYER_SYNC
    am_data_manual_transaction();
    #endif

    change_layer_settings(highest_layer);

    #ifdef DYNAMIC_CALIBRATION
    // Not the cleanest way to allow disabling the update check
    #if !defined AM_NO_EEPROM && RECALIBRATED_SWITCHES <= (SMAX(SWITCH_NUM) + 1)
    if(recalibrated_switches >= RECALIBRATED_SWITCHES) {
        eeconfig_update_keyboard((analog_switch_t*)&calibration_data);
        recalibrated_switches = 0;
    }
    #endif
    #endif

    #if PROFILE_SWITCH_MODE != MANUAL_PROFILE
    if (!manual_profile_lock) {
        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            // If the highest active layer is in the layers list of that profile, activate it
            if(profiles[profile].layers & (1 << highest_layer)) {
                set_active_profile(profile);

                // Only the lowest profile should apply
                return state;
            }
        }
        // If profile switch mode is default, switch to the default profile if layer is not set for any profile
        #if PROFILE_SWITCH_MODE == DEFAULT_PROFILE
        set_active_profile(AM_DEFAULT_PROFILE);
        #endif
    }
    #endif // if PROFILE_SWITCH_MODE != MANUAL_PROFILE
    return state;
}


//MARK: Adjust
//TODO: A normal function doesn't work, since every key has different top/bottom values
//      Need to do a function that takes them into account
#ifdef ADJUSTMENT_FUNCTION
__attribute__((weak)) uint16_t adjust(uint16_t value) {
    #define EULER 2.7182817
    const float exp = (-value + 1265) / 170.0;
    return (-powf(EULER, exp) + 650);
}
#else
#   define adjust(value) value
#endif


//MARK: Priority mux
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

        #ifdef CUSTOM_POWER_BEFORE_SCAN
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
        #ifdef DEBUG_MUX_POSITION
        if(adc_channel == debug_mux[0] && mux_channel == debug_mux[1]) {
            dprintf("%u\n", adc_value);
        }
        #endif
        #ifdef ADC_SCAN_DELAY
        delay_ns(ADC_SCAN_CYCLES);
        #endif

        if(evaluate_value(matrix_index, adc_value)) {
            matrix_has_changed = true;
            #ifndef DEBUG_SCAN_NO_INPUT
            current_matrix[key_config[matrix_index].row] ^= 1 << key_config[matrix_index].col;
            #endif
        }

        #ifdef CUSTOM_POWER_BEFORE_SCAN
        set_sensor_power_low_kb(mux_channel);
        #endif
    }
    matrix_scan_kb();

    return matrix_has_changed;
}
#endif


//MARK: Suspend
__attribute__((weak)) void suspend_power_down_kb(void) {
    #ifdef MATRIX_POWER_PIN
    #ifndef INVERT_MATRIX_POWER
    gpio_write_pin_low(MATRIX_POWER_PIN);
    #else
    gpio_write_pin_high(MATRIX_POWER_PIN);
    #endif
    #endif

    suspend_power_down_user();
}

__attribute__((weak)) void suspend_wakeup_init_kb(void) {
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


// #ifndef CONST_A1
// #    define CONST_A1 426.88962
// #endif
// #ifndef CONST_B1
// #    define CONST_B1 -0.48358
// #endif
// #ifndef CONST_C1
// #    define CONST_C1 2.04637e-4
// #endif
// #ifndef CONST_D1
// #    define CONST_D1 -2.99368e-8
// #endif

//MARK: Keychron adjust
// Removed a minus since keychron uses inverted ADC values
// #define TRAVEL_POLYNOMIAL(x) (CONST_A1 + CONST_B1 * x + CONST_C1 * x * x + CONST_D1 * x * x * x)

// uint8_t convert_to_travel(uint8_t index, uint16_t value) {
//     uint16_t travel;
//     // const uint16_t bottom = key_config[index].bottom_value;
//     const uint16_t x = value - key_config[index].bottom_value;

//     // Keychron uses inverse adc values
//     // if (x > REF_ZERO_TRAVEL) return 0;

//     //TODO: Save the travel_units to the key config
//     travel = (uint16_t)((TRAVEL_POLYNOMIAL(x) - TRAVEL_POLYNOMIAL(REF_ZERO_TRAVEL)) * scale_factor[row][col] * TRAVEL_SCALE + 0.5);
//     if (travel > (FULL_TRAVEL_UNIT + 1) * TRAVEL_SCALE - 1) travel = (FULL_TRAVEL_UNIT + 1) * TRAVEL_SCALE - 1;

//     // Limit travel to 256 counts?
//     return travel & 0xFF;
// }
