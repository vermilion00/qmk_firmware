//TODO: Go through these and check which ones are needed

#include "analog_matrix.h"
#include "config.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "_wait.h"
#include "action_layer.h"
#include "bootloader.h"
#include "eeconfig.h"
#include "keymap_introspection.h"
#include "debug.h"
#include "print.h"
#ifdef SPLIT_KEYBOARD
#   include "transactions.h"
#endif
#if ANALOG_DEBOUNCE > 0
#   include "debounce.h"
#endif
#ifdef MIXED_MATRIX_ENABLE
#   include "mixed_matrix.h"
#   include "debounce.h"
#endif
#ifdef JOYSTICK_ENABLE
#   include "analog_joystick.h"
#endif
#ifdef MIDI_ENABLE
#   include "analog_midi.h"
#endif

#ifdef VIA_ENABLE
#   include "nvm_dynamic_keymap.h"
// If VIA is enabled, an external definition is used instead
extern am_keyboard_t am_keyboard_data;
#else
SPLIT_MUTABLE am_keyboard_t am_keyboard_data = {
    #ifdef USE_TRIGGER_HEIGHT
    .trigger_height = TRIGGER_HEIGHT,
    .release_height = RELEASE_HEIGHT,
    #endif
    #ifdef USE_RT_DISTANCE
    .rt_press_distance = RT_PRESS_DISTANCE,
    .rt_release_distance = RT_RELEASE_DISTANCE,
    #endif
    .key_mode = KEY_MODES,
    .profile_config = AM_DEFAULT_PROFILE << 4 | PROFILE_SWITCH_MODE,
    .top_deadzone = USER_TOP_DEADZONE,
    .bottom_deadzone = USER_BOTTOM_DEADZONE,
    .smoothing = ADC_SMOOTHING,
    .top_mult = TOP_DEADZONE_MULT * 100,
    #ifdef DYNAMIC_CALIBRATION
    .dc_switch_num = RECALIBRATED_SWITCHES,
    .dc_factor = AM_DC_FACTOR * 100,
    .dc_delta = AM_DC_DELTA,
    #endif
    #ifdef SPLIT_KEYBOARD
    .right_mult = RIGHT_MULTIPLIER * 100,
    .slave_mult = SLAVE_MULTIPLIER * 100,
    #endif
    #ifdef PRIORITY_PROFILES
    .priority_profiles = PRIORITY_PROFILES,
    #endif
    .profile_layers = PROFILE_LAYERS,
    .profile_num = 0b10000000 | AM_PROFILE_NUM,
};
#endif

#if defined SPLIT_KEYBOARD || defined VIA_ENABLE
uint16_t top_deadzone = ADC_TOP_DEADZONE;
uint16_t bottom_deadzone = ADC_BOTTOM_DEADZONE;
//TODO: Once smoothing calibration is a thing, also do a USER_SMOOTHING thing
uint16_t smoothing = ADC_SMOOTHING;
#if FILTER_STRENGTH != SLAVE_FILTER_STRENGTH
adc_filter_t adc_filter = adc_filter_function;
#else
const adc_filter_t adc_filter = adc_filter_function;
#endif
#else // ifdef SPLIT_KEYBOARD || defined VIA_ENABLE
const uint16_t top_deadzone = ADC_TOP_DEADZONE + USER_TOP_DEADZONE;
const uint16_t bottom_deadzone = ADC_BOTTOM_DEADZONE + USER_BOTTOM_DEADZONE;
const uint16_t smoothing = ADC_SMOOTHING;
const adc_filter_t adc_filter = adc_filter_function;
#endif // if defined SPLIT_KEYBOARD || defined VIA_ENABLE else

#ifndef AM_NO_EEPROM
uint16_t calibration_data[SMAX(SWITCH_NUM)];
#endif
uint8_t top_deadzones[SMAX(SWITCH_NUM)] = {[0 ... SMAX(SWITCH_NUM) - 1] = ADC_TOP_DEADZONE};

extern matrix_row_t matrix[MATRIX_ROWS];
#if ANALOG_DEBOUNCE > 0
extern matrix_row_t raw_matrix[MATRIX_ROWS];
#define SCAN_MATRIX raw_matrix
#else
#define SCAN_MATRIX matrix
#endif
#ifdef SPLIT_KEYBOARD
// Row offsets for each hand
extern uint8_t thisHand, thatHand;
#endif
void reset_matrix_keys(void);
#ifdef DYNAMIC_CALIBRATION
// Keeps track of how many switches need updating, and saves new data once it exceeds RECALIBRATED_SWITCHES, to avoid writing to storage too often
__attribute__((unused)) uint8_t recalibrated_switches = 0;
#endif

#if SMAX(AM_INIT_KEY_NUM) > 0
// Initialize the keys to be checked at initialization
void _bootloader_jump(bool init);
void _bootmagic(bool init);
SPLIT_MUTABLE uint8_t init_keys[SMAX(AM_INIT_KEY_NUM)][2] = AM_INIT_KEYS;
SPLIT_MUTABLE init_func_t init_functions[SMAX(AM_INIT_KEY_NUM)] = AM_INIT_FUNCTIONS;
#endif

#if defined DEBUG_MUX_POSITION
uint8_t debug_mux[2] = DEBUG_MUX_POSITION;
#elif defined DEBUG_MUX_POSITION_R
uint8_t debug_mux[2] = DEBUG_MUX_POSITION_R;
#endif

#ifdef USE_PRIORITY_MODE
bool priority_mode = false;
#else
#   define priority_mode false
#endif

//TODO: Remove PROFILE_MUTABLE?
PROFILE_MUTABLE uint8_t active_profile = AM_DEFAULT_PROFILE;
uint8_t am_highest_layer = 0;

SPLIT_MUTABLE uint8_t switch_num = SWITCH_NUM;
SPLIT_VIA_MUT uint8_t switch_low = 0;
SPLIT_MUTABLE uint8_t adc_pin_num = ADC_PIN_NUM;
SPLIT_MUTABLE uint8_t mux_channel_num = MUX_CHANNELS;
#ifdef MUX_PINS
SPLIT_MUTABLE uint8_t mux_pin_num = MUX_PIN_NUM;
#ifdef MUX_PINS_CONTINUOUS
SPLIT_MUTABLE uint8_t mux_offset = MUX_PIN_OFFSET;
#ifndef MCU_RP
gpio_port_t* mux_port = CONTINUOUS_MUX_PORT;
#endif
#endif
#endif

#if defined SPLIT_KEYBOARD && defined AM_NO_EEPROM
uint8_t switch_num_slave = SWITCH_NUM_R;
#endif

analog_key_t key_config[SMAX(SWITCH_NUM)];
uint8_t mux_to_num[SMAX(MUX_CHANNELS)][ADC_PIN_NUM] = MUX_TO_NUM;
uint8_t num_to_matrix[SMAX(SWITCH_NUM)][2] = NUM_TO_MATRIX;

SPLIT_MUTABLE pin_t adc_pins[SMAX(ADC_PIN_NUM)] = ADC_PINS;
#if defined MUX_PINS
SPLIT_MUTABLE pin_t mux_pins[SMAX(MUX_PIN_NUM)] = MUX_PINS;
#endif
#if defined POWER_PINS
SPLIT_MUTABLE pin_t power_pins[SMAX(POWER_PIN_NUM)] = POWER_PINS;
SPLIT_MUTABLE uint8_t power_pin_num = POWER_PIN_NUM;
// Set the sensor power pins and wait, if defined
void set_sensor_power(uint8_t index);
#endif

// During initialization, the adc pins are translated to the adc mux combination that the adc_read function uses
adc_mux adc_pin_mux[ADC_PIN_NUM];

//TODO: Assign correct side at init
// This is currently unused in favor of PRIORITY_INDICES
// #ifdef PRIORITY_MUXES
// uint8_t scan_amt = 0;
// uint8_t priority_muxes[PRIORITY_MUX_NUM][3] = PRIORITY_MUXES;
// uint8_t matrix_scan_priority(matrix_row_t current_matrix[]);
// #endif

#if defined PRIORITY_INDICES || defined SLAVE_LOW_PRIORITY
uint8_t scan_amt = 0;
#ifdef PRIORITY_INDICES
SPLIT_VIA_MUT uint8_t priority_indices[SMAX(SWITCH_NUM)] = PRIORITY_INDICES;
// SPLIT_VIA_MUT uint8_t priority_index_num = PRIORITY_INDEX_NUM;
#endif
#endif


//MARK: Init
__attribute__((weak)) void analog_matrix_init(void) {
    #ifdef CLEAR_CALIBRATION
    clear_calibration();
    #endif

    #ifdef MIXED_MATRIX_ENABLE
    mixed_matrix_init();
    #endif

    #if (defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN)
    // Assign the right half configuration if necessary
    assign_side();
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

    get_key_config();

    //TODO: Add mode to check init keys based on deviation larger than 2* the avg diff between them (or smth like that), because right now init keys don't work without saved bottom values
    //      Also being unable to save cali values properly, due to a small difference between top & bottom values or a misconfigured storage, means that the keyboard keeps looping endlessly

    // Get the min/max values of each switch
    if(!get_calibration_data()) {
        // If loading the calibration data fails, start calibration
        calibrate_switches(true);
    }

    // Translate the trigger height etc into the equivalent ADC value
    for(uint8_t index = 0; index < switch_num; index++) {
        translate_mm_to_value(index, true);
    }

    profile_state_changed(active_profile);

    #if defined SPLIT_LAYER_SYNC
    change_layer_settings(am_highest_layer);
    #endif

    // This is only needed to print the calibration data
    #if defined SPLIT_KEYBOARD && defined AM_NO_EEPROM
    if (is_keyboard_master()) {
        if (!is_keyboard_left()) switch_num_slave = SWITCH_NUM_L;
    } else if (is_keyboard_left()) switch_num_slave = SWITCH_NUM_L;
    #endif

    matrix_init_kb();
}


//MARK: Init keys
#if SMAX(AM_INIT_KEY_NUM) > 0
bool scan_init_keys(void) {
    for(uint8_t idx = 0; idx < sizeof(init_keys)/2; idx++) {
        set_sensor_power_high(init_keys[idx][1]);
        set_mux_channel(init_keys[idx][1]);

        const uint8_t matrix_index = mux_to_num[init_keys[idx][1]][init_keys[idx][0]];
        wait_ms(AM_STARTUP_DELAY / 5);
        const uint16_t adc_value = adc_read(adc_pin_mux[init_keys[idx][0]]);

        set_sensor_power_low(init_keys[idx][1]);

        // If the key is activated, call the respective function
        // These functions are set in the INIT_FUNCTIONS dict at the top of analog_matrix.py
        #ifndef INVERT_ADC
        if(adc_value < key_config[matrix_index].bottom_value + INIT_THRESHOLD)
        #else
        if(adc_value > key_config[matrix_index].bottom_value - INIT_THRESHOLD)
        #endif
        {
            init_functions[idx](true);
            return true;
        }
    }
    // No init keys are held
    return false;
}
#else // AM_INIT_KEY NUM > 0
#define scan_init_keys() false
#endif // AM_INIT_KEY NUM > 0 else


//MARK: Scan
// Guarded to allow overwriting the full scan function
#ifndef CUSTOM_MATRIX_FULL
uint8_t matrix_scan(void) {
    bool matrix_has_changed = false;

    #ifdef MIXED_MATRIX_ENABLE
    matrix_has_changed |= mixed_matrix_scan();
    #endif

    // For custom matrix lite implementations, we skip the analog matrix scanning, but keep debouncing and split synchronisation
    #ifdef CUSTOM_MATRIX_LITE
    matrix_has_changed |= matrix_scan_custom(SCAN_MATRIX);
    #else

    // Unused in favor of priority_indices
    // #ifdef PRIORITY_MUXES
    // if(profiles[active_profile].priority_profile && scan_amt < PRIORITY_LEVEL) {
    //     scan_amt += 1;
    //     return matrix_scan_priority(current_matrix);
    // } else { scan_amt = 0; }
    // #endif

    for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
        set_sensor_power_high(mux_channel);
        set_mux_channel(mux_channel);

        for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
            // Translate matrix mux and adc channels to matrix position
            const uint8_t index = mux_to_num[mux_channel][adc_channel];
            // Check if a switch is at the position (matrix index < 255), and scan if so
            if(index == 255) {
                #ifdef DEBUG_SCAN_VALUES
                dprintf("%u/%2u:    , ",adc_channel, mux_channel);
                #endif
                continue;
            }

            #ifdef PRIORITY_INDICES
            if(priority_mode && scan_amt < PRIORITY_LEVEL) {
                if(!priority_indices[index]) { continue; }
            }
            #endif


            // Simple IIR filter with 1/4 alpha by default
            const uint16_t adc_value = adc_filter(adc_read(adc_pin_mux[adc_channel]), index);
            wait_cycles(ADC_SCAN_CYCLES);

            #ifdef DEBUG_MUX_POSITION
            if(adc_channel == debug_mux[0] && mux_channel == debug_mux[1]) {
                dprintf("%u\n", adc_value);
            }
            #elif defined DEBUG_SCAN_VALUES
            dprintf("%u/%2u: %3u, ", adc_channel, mux_channel, adc_value);
            #endif

            // Check if the value has changed enough to warrant an evaluation
            // On special layers (joystick, midi etc) this is skipped, as they need to be more precise
            if(!special_layer && (adc_value < (key_config[index].scan_value + smoothing)) && (adc_value > (key_config[index].scan_value - smoothing))) continue;
            key_config[index].scan_value = adc_value;

            // Check if key is pressed/released, returns true if the switch state has changed
            if(evaluate_value(index, adc_value)) {
                matrix_has_changed = true;
                #ifndef DEBUG_SCAN_NO_INPUT
                SCAN_MATRIX[key_config[index].row] ^= 1 << key_config[index].col;
                #else
                dprintf("[%u/%u]: %u\n", SCAN_MATRIX[key_config[index].row], SCAN_MATRIX[key_config[index].col], key_config[index].pressed);
                #endif
            }

            // If dynamic calibration is enabled, check if the boundaries need updating
            #ifdef DYNAMIC_CALIBRATION
            if(!priority_mode && update_switch_bounds(index, adc_value)) {
                // If the bounds have been updated, translate the heights
                translate_mm_to_value(index, false);
                recalibrated_switches += 1;
            }
            #endif // ifdef DYNAMIC_CALIBRATION
        }
        set_sensor_power_low(mux_channel);
    }

    #ifdef DEBUG_SCAN_VALUES
    dprint("\n");
    #endif

    #ifdef PRIORITY_INDICES
    if(scan_amt < PRIORITY_LEVEL) {
        scan_amt += 1;
    } else { scan_amt = 0; }
    #endif

    #endif // ifdef CUSTOM_MATRIX_LITE

    #if ANALOG_DEBOUNCE > 0
    #ifdef SPLIT_KEYBOARD
    matrix_has_changed = debounce(raw_matrix + thisHand, matrix + thisHand, MATRIX_ROWS_PER_HAND, matrix_has_changed) | matrix_post_scan();
    #else
    matrix_has_changed = debounce(raw_matrix, matrix, MATRIX_ROWS_PER_HAND, changed);
    matrix_scan_kb();
    #endif

    #else // if ANALOG_DEBOUNCE > 0
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
    #endif // if ANALOG_DEBOUNCE > 0

    return matrix_has_changed;
}
// Weakly defined to allow overwriting with custom implementation
#else // ifndef CUSTOM_MATRIX_FULL
__attribute__((weak)) uint8_t matrix_scan(void) { return false; }
#endif


//MARK: Translate
// Translate the heights of all keys into the corresponding ADC values
void translate_mm_to_value(uint8_t index, bool init) {
    #ifdef INVERT_ADC
    const uint16_t top_value = init ? (key_config[index].top_value - top_deadzone) : (key_config[index].top_value - top_deadzones[index]);
    const uint16_t bottom_value = key_config[index].bottom_value + bottom_deadzone;
    const float travel_unit = floor((float)(bottom_value - top_value) / (TRAVEL_DISTANCE * HEIGHT_MULT));
    #else
    // Take out the deadzones here, since we want to calculate the travel unit for the entire range
    const uint16_t top_value = init ? (key_config[index].top_value + top_deadzone) : (key_config[index].top_value + top_deadzones[index]);
    const uint16_t bottom_value = key_config[index].bottom_value - bottom_deadzone;
    const float travel_unit = floor((float)(top_value - bottom_value) / (TRAVEL_DISTANCE * HEIGHT_MULT));
    #endif

    for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
        //TODO: Make sure these offsets are correct
        #ifdef USE_TRIGGER_HEIGHT
        const height_t* trigger_height = am_keyboard_data.trigger_height[profile] + switch_low;
        const height_t* release_height = am_keyboard_data.release_height[profile] + switch_low;
        #ifndef INVERT_ADC
        #ifndef DISTANCE_FROM_BOTTOM
        //TODO: Maybe instead of adjusting the converted adc value, instead adjust the height setting? like 1.0mm -> 0.7mm, 3.0mm -> 3.5mm
        //      Would need to find something for RT distances though, which probably would render it redundant
        //TODO: Test this, add INVERT_ADC values
        key_config[index].trigger_value[profile] = adjust(top_value - (travel_unit * trigger_height[index])) - smoothing;
        key_config[index].release_value[profile] = adjust(top_value - (travel_unit * release_height[index])) + smoothing;

        #else // ifndef DISTANCE_FROM_BOTTOM
        key_config[index].trigger_value[profile] = adjust(travel_unit * trigger_height[index] + bottom_value) - smoothing;
        key_config[index].release_value[profile] = adjust(travel_unit * release_height[index] + bottom_value) + smoothing;
        #endif // ifndef DISTANCE_FROM_BOTTOM else

        #else // ifndef INVERT_ADC
        //Inverted ADC -> Lower switch means higher value
        #ifndef DISTANCE_FROM_BOTTOM
        //TODO: Is this correct
        key_config[index].trigger_value[profile] = adjust(top_value + (travel_unit * trigger_height[index])) + smoothing;
        key_config[index].release_value[profile] = adjust(top_value + (travel_unit * release_height[index])) - smoothing;

        #else // ifndef DISTANCE_FROM_BOTTOM
        key_config[index].trigger_value[profile] = adjust(bottom_value - (travel_unit * trigger_height[index])) + smoothing;
        key_config[index].release_value[profile] = adjust(bottom_value - (travel_unit * release_height[index])) - smoothing;
        #endif // ifndef DISTANCE_FROM_BOTTOM else
        #endif // ifndef INVERT_ADC else
        #endif // ifdef USE_TRIGGER_HEIGHT

        #if defined USE_RT_DISTANCE
        const uint16_t press_value = travel_unit * am_keyboard_data.rt_press_distance[profile][index + switch_low];
        key_config[index].rt_press_value[profile] = (press_value > smoothing) ? press_value : smoothing;
        const uint16_t release_value = travel_unit * am_keyboard_data.rt_release_distance[profile][index + switch_low];
        key_config[index].rt_release_value[profile] = (release_value > smoothing) ? release_value : smoothing;

        // If RT is enabled for this key and profile, set the threshold (only on the lowest profile with RT enabled)
        //TODO: Need to do this anytime the profile changes
        if(key_config[index].rt_threshold == 0 && key_config[index].mode[profile] > 0) key_config[index].rt_threshold = key_config[index].rt_press_value[profile];
        #endif
    }
}


//MARK: Get calibration
bool get_calibration_data(void) {
    #ifdef FORCE_CALIBRATE
    return false;
    #endif

    #if defined AM_NO_EEPROM && !defined AM_BOTTOM_VALUES
    return false;
    #endif

    // Get the calibrated values from whatever source
    uint16_t* bottom_value = NULL;
    uint8_t* deadzone_value = &top_deadzones[0];
    #ifdef AM_BOTTOM_VALUES
    SPLIT_MUTABLE uint16_t bottom_values[SMAX(SWITCH_NUM)] = AM_BOTTOM_VALUES;
    bottom_value = &bottom_values[0];
    #if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
    if(!is_keyboard_left()) {
        const uint16_t bottom_values_r[SMAX(SWITCH_NUM)] = AM_BOTTOM_VALUES_R;
        memcpy(&bottom_values, &bottom_values_r, sizeof(bottom_values_r));
    }
    #endif // if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
    #endif
    #ifdef AM_TOP_DEADZONES
    SPLIT_MUTABLE uint8_t deadzones[SMAX(SWITCH_NUM)] = AM_TOP_DEADZONES;
    deadzone_value = &deadzones[0];
    #if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
    if(!is_keyboard_left()) {
        const uint8_t deadzones_r[SMAX(SWITCH_NUM)] = AM_TOP_DEADZONES_R;
        memcpy(&deadzones, &deadzones_r, sizeof(deadzones));
    }
    memcpy(&top_deadzones, &deadzones, sizeof(deadzones));
    #endif // if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
    #endif // ifdef AM_BOTTOM_VALUES
    #ifndef AM_NO_EEPROM
    // Read calibrated values from eeprom
    eeconfig_read_keyboard((uint16_t*)&calibration_data);
    eeconfig_read_deadzone((uint8_t*)&top_deadzones);
    bottom_value = &calibration_data[0];
    deadzone_value = &top_deadzones[0];
    #ifdef AM_BOTTOM_VALUES
    const uint16_t* bottom_backup = &bottom_values[0];
    #endif
    #endif

    for(uint8_t key = 0; key < switch_num; key++) {
        bool valid = true;
        int16_t bottom_adjustment = 0;

        #ifndef INVERT_ADC
        // Check if the top deadzone is valid
        if(deadzone_value[key] < 250 && deadzone_value[key] > 1) {
            // The overall top deadzone needs to be removed as well, as it's already applied at this stage
            key_config[key].top_value = key_config[key].top_value + top_deadzone - deadzone_value[key] - am_keyboard_data.top_deadzone;
        }

        // Check if the switch data makes sense, start calibration or use fallback values if not
        if(key_config[key].top_value <= bottom_value[key]) valid = false;
        if(key_config[key].top_value - bottom_value[key] < INIT_THRESHOLD) valid = false;
        #ifdef DYNAMIC_CALIBRATION
        bottom_adjustment = AM_DC_FACTOR * (key_config[key].top_value - bottom_value[key]) + bottom_deadzone;
        #endif

        #else
        if(deadzone_value[key] < 250 && deadzone_value[key] > 1) {
            key_config[key].top_value = key_config[key].top_value - top_deadzone + deadzone_value[key] + am_keyboard_data.top_deadzone;
        }
        if(key_config[key].top_value >= bottom_value[key]) valid = false;
        if(bottom_value[key] - key_config[key].top_value < INIT_THRESHOLD) valid = false;
        #ifdef DYNAMIC_CALIBRATION
        adjustment = -(AM_DC_FACTOR * (bottom_value[key] - key_config[key].top_value) + bottom_deadzone);
        #endif
        #endif

        if(bottom_value[key] < 100 || bottom_value[key] > 4000)  valid = false;

        key_config[key].bottom_value = bottom_value[key] + bottom_adjustment;

        if(!valid) {
            //TODO: What happens if the config is actually invalid? Will that erroneously trigger init keys, or not?
            // An invalid reading can stem from a pressed down key, check for that here
            #if AM_INIT_KEY_NUM > 0
            valid = scan_init_keys();
            #endif
            if(!valid) {
                // If no init key is pressed, check if we have backup data, otherwise start calibration immediately
                #if !defined AM_NO_EEPROM && defined AM_BOTTOM_VALUES
                key_config[key].bottom_value = bottom_backup[key];
                // Otherwise, check if a default offset is defined, and apply that
                #elif defined ADC_DEFAULT_OFFSET
                #ifndef INVERT_ADC
                key_config[key].bottom_value = key_config[key].top_value - ADC_DEFAULT_OFFSET - bottom_deadzone;
                #else
                key_config[key].bottom_value = key_config[key].top_value + ADC_DEFAULT_OFFSET + bottom_deadzone;
                #endif
                // If neither fallback is available, start calibration without checking other keys
                #else
                return false;
                #endif
            }
        }

        key_config[key].scan_value = key_config[key].top_value;
    }
    return true;
}


//MARK: Calibrate
void calibrate_switches(bool init) {
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

        // Force a synchronisation so that the slave starts calibrating as well
        uint8_t tries = 0;
        while (tries < 50) {
            // Start the transaction manually
            if (!am_data_manual_transaction(calibration_started)) {
                wait_ms(10);
                tries++;
            } else { break; }
        }
        wait_ms(1000);
    }
    #endif

    LED_ON;
    while(true) {
        for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
            #if defined DEBUG_CALIBRATION || defined DEBUG_SCAN_VALUES
            dprintf("Mux: %i\n", mux_channel);
            #endif
            set_sensor_power_high(mux_channel);
            set_mux_channel(mux_channel);

            for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
                // Translate matrix mux and adc channels to matrix position
                uint8_t matrix_index = mux_to_num[mux_channel][adc_channel];
                // Check if a switch is at the position (matrix index < 255), and scan if so
                if(matrix_index == 255) continue;

                uint16_t adc_value = adc_read(adc_pin_mux[adc_channel]);

                if(first_scan) {
                    key_config[matrix_index].bottom_value = adc_value;
                    key_config[matrix_index].scan_value = adc_value;
                    continue;
                }

                adc_value = adc_filter(adc_value, matrix_index);
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

                } else if(key_config[matrix_index].bottom_value - key_config[matrix_index].top_value < CAL_THRESHOLD) {
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
                } else if(key_config[matrix_index].top_value - key_config[matrix_index].bottom_value < CAL_THRESHOLD) {
                    scans_without_change = 0;
                }
                #endif // ifdef INVERT_ADC else
            }

            #if defined DEBUG_CALIBRATION || defined DEBUG_SCAN_VALUES
            dprint("\n");
            #endif
            set_sensor_power_low(mux_channel);
        }

        first_scan = false;
        scans_without_change += 1;
        if(scans_without_change > SCANS_WITHOUT_CHANGE){
            // Calibration is done
            LED_OFF;
            // Clear matrix and layer state to avoid stuck keys
            reset_matrix_keys();
            layer_state = default_layer_state;
            am_highest_layer = default_layer_state;

            #if defined AM_NO_EEPROM || defined DEBUG_CALIBRATION
            #ifdef SPLIT_KEYBOARD
            if(!init) {
                void sync_calibration_values(bool init);
                sync_calibration_values(init);
            }
            #endif

            // Print the bottom value of each switch so that it can be adjusted in the config
            printf("bottom_values%s\": [ %u", side, key_config[0].bottom_value);
            for(uint8_t index = 1; index < switch_num; index++){
                printf(", %u", key_config[index].bottom_value);
            }
            print(" ],\n");

            #else // ifdef AM_NO_EEPROM
            for(uint8_t key = 0; key < switch_num; key++) {
                // Save the calibration values without a deadzone applied
                calibration_data[key] = key_config[key].bottom_value;
            }
            eeconfig_update_keyboard((uint16_t*)&calibration_data);

            #endif // ifdef AM_NO_EEPROM else

            // Apply deadzones
            for(uint8_t key = 0; key < switch_num; key++) {
                #ifndef INVERT_ADC
                key_config[key].top_value -= (top_deadzones[key] + am_keyboard_data.top_deadzone);
                key_config[key].bottom_value += bottom_deadzone;
                #else
                key_config[key].top_value += (top_deadzones[key] + am_keyboard_data.top_deadzone);
                key_config[key].bottom_value -= bottom_deadzone;
                #endif
                key_config[key].scan_value = key_config[key].top_value;

                // Translate the heights again using the new values
                translate_mm_to_value(key, false);
            }

            break;
        }
    }
}


//This is run from AM_PRNT
#if defined SPLIT_KEYBOARD && defined AM_NO_EEPROM
void _sync_cal(void) {
    sync_calibration_values(false);
}
#endif

//MARK: Calibrate top
void calibrate_top_value(void) {
    //TODO: Wait in a scan loop until all keys are in a certain distance of the established top_value, then wait a small amount of time before starting
    //      Could also use that to prime the filter

    #ifdef AM_NO_EEPROM
    char side[7] = "";
    #endif

    #ifdef SPLIT_KEYBOARD
    if(is_keyboard_master()) {
        #ifdef AM_NO_EEPROM
        if(!is_keyboard_left()) {
            const char right[] = "_right";
            memcpy(&side, &right, sizeof(right));
        }
        #endif

        // Force a synchronisation so that the slave starts calibrating as well
        uint8_t tries = 0;
        while (tries < 50) {
            // Start the transaction manually
            if (!am_data_manual_transaction(top_calibration_started)) {
                wait_ms(10);
                tries++;
            } else { break; }
        }
    }
    #endif

    LED_ON;
    // Wait to make sure that all keys are released
    wait_ms(1500);
    // Dummy read just in case
    adc_read(adc_pin_mux[0]);
    for(uint8_t repeat = 0; repeat < 2; repeat++) {
        for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
            set_sensor_power_high(mux_channel);
            set_mux_channel(mux_channel);

            for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
                const uint8_t index = mux_to_num[mux_channel][adc_channel];
                if(index == 255) continue;

                // The first scan needs to prime the scan value for the filter, else the pressed keys at start will read wrong values
                if(!repeat) {
                    key_config[index].scan_value = adc_read(adc_pin_mux[adc_channel]);
                    wait_cycles(ADC_SCAN_DELAY);
                    continue;
                }
                uint32_t adc_values = 0;

                #ifndef INVERT_ADC
                uint16_t lowest_value = 4095;
                // Take the average of 250 scans
                for(uint8_t scan = 0; scan < 250; scan++) {
                    //TODO: Test if it makes sense to filter this value
                    // const uint16_t adc_value = adc_read(adc_pin_mux[adc_channel]), index;
                    const uint16_t adc_value = adc_filter(adc_read(adc_pin_mux[adc_channel]), index);
                    wait_cycles(ADC_SCAN_DELAY);
                    key_config[index].scan_value = adc_value;
                    adc_values += adc_value;
                    if(adc_value < lowest_value) lowest_value = adc_value;
                }
                adc_values /= 250;
                // Set the deadzone to be 30% higher, rounded up
                const uint16_t cal_top_deadzone = (uint16_t)(floor((adc_values - lowest_value) * (am_keyboard_data.top_mult/100.0)) + 1);
                key_config[index].top_value = adc_values - (cal_top_deadzone < 255 ? cal_top_deadzone : 255) - am_keyboard_data.top_deadzone;

                #else
                uint16_t lowest_value = 0;
                for (uint8_t scan = 0; scan < 250; scan++) {
                    const uint16_t adc_value = adc_filter(adc_read(adc_pin_mux[adc_channel]), index);
                    wait_cycles(ADC_SCAN_DELAY);
                    key_config[index].scan_value = adc_value;
                    adc_values += adc_value;
                    if(adc_value > lowest_value) lowest_value = adc_value;
                }
                adc_values /= 250;
                const uint8_t cal_top_deadzone = (uint16_t)(floor((lowest_value - adc_values) * (am_keyboard_data.top_mult/100.0)) + 1);
                key_config[index].top_value = adc_values + (cal_top_deadzone < 255 ? cal_top_deadzone : 255) + am_keyboard_data.top_deadzone;
                #endif
                key_config[index].scan_value = adc_values;
                top_deadzones[index] = (cal_top_deadzone < 255 ? cal_top_deadzone : 255);

                #ifdef DEBUG_CALIBRATION
                dprintf("%u: %lu, %u\n", index, adc_values, cal_top_deadzone);
                #endif
                translate_mm_to_value(index, false);
            }
            set_sensor_power_low(mux_channel);
        }
    }
    LED_OFF;
    dprint("Top deadzone calibration finished\n");
    // Clear matrix and layer state to avoid stuck keys
    reset_matrix_keys();
    layer_state = default_layer_state;
    am_highest_layer = default_layer_state;
    #ifndef AM_NO_EEPROM
    eeconfig_update_deadzone((uint8_t*)&top_deadzones);
    #else
    #if defined SPLIT_KEYBOARD || defined DEBUG_CALIBRATION
    void sync_top_calibration(void);
    sync_top_calibration();
    #endif
    // Print the calibration values of each switch so that they can be adjusted in the config
    printf("\"top_deadzones%s\":    [ %u", side, top_deadzones[0]);
    for(uint8_t index = 1; index < switch_num; index++){
        printf(", %u", top_deadzones[index]);
    }
    print(" ]\n");
    #endif
}


//MARK: Evaluate
bool evaluate_value(uint8_t index, uint16_t value) {
    #if defined JOYSTICK_ENABLE
    if(joystick_layer) {
        if(key_config[index].axis_index < 255) {
            if(!translate_joystick_axis(index, value)) return false;
            #if defined SPLIT_KEYBOARD && !defined NO_SLAVE_AXES
            // On split keyboards, we can't update the axes like this since the slave axes won't be evaluated
            // Instead, we update all axes in the analog_joystick_task()
            joystick_state.dirty = true;
            return false;
            #else
            evaluate_joystick_axis(key_config[index].axis_index);
             return false;
            #endif
        }
    }
    #endif

    const bool prev_pressed = key_config[index].pressed;

#ifdef INVERT_ADC
    switch(key_config[index].mode[active_profile]) {
        #if defined USE_NONE
        case none:
        if(value < key_config[index].top_value || value < key_config[index].release_value[active_profile]) {
            key_config[index].pressed = false;
        } else if(value > key_config[index].trigger_value[active_profile] || value > key_config[index].bottom_value) {
            key_config[index].pressed = true;
        } else {
            return false;
        }
        break;
        #endif //defined RAPID_TRIGGER

        #if defined USE_RAPID_TRIGGER
        case rapid_trigger:
        // If the switch is not pressed past the threshold, reset it
        if(value < key_config[index].top_value || value < key_config[index].release_value[active_profile]) {
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
        // Rapid trigger is only active when the switch is lower than the trigger and release height
        } else if(value > key_config[index].trigger_value[active_profile]) {
            // Set the new lowest value if needed
            if(value > key_config[index].rt_threshold || value > key_config[index].bottom_value) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value + key_config[index].rt_threshold) < key_config[index].rt_release_value[active_profile] || value < key_config[index].top_value) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value + key_config[index].rt_press_value[active_profile];
            }
        }
        break;
        #endif

        #if defined USE_CONTINUOUS_RAPID_TRIGGER
        case continuous_rapid_trigger:
        // Check if the switch has been released completely
        if(value < key_config[index].top_value) {
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
            key_config[index].rt_active = false;
        // Rapid trigger activates below the trigger height, but only stops when fully released
        } else if(key_config[index].rt_active || (value > key_config[index].trigger_value[active_profile])) {
            key_config[index].rt_active = true;
            // Set the new lowest value if needed
            if(value > key_config[index].rt_threshold || value > key_config[index].bottom_value) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value + key_config[index].rt_threshold) < key_config[index].rt_release_value[active_profile]) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value + key_config[index].rt_press_value[active_profile];
            }
        }
        break;
        #endif

        #if defined USE_CONSTANT_RAPID_TRIGGER
        case constant_rapid_trigger:
        // Check if the key has been released past the threshold or completely
        if(value < key_config[index].top_value || (value + key_config[index].rt_threshold) < key_config[index].rt_release_value[active_profile]) {
            key_config[index].pressed = false;
            // Set the new activation threshold
            key_config[index].rt_threshold = value + key_config[index].rt_press_value[active_profile];
        // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
        } else if(value > key_config[index].rt_threshold || value > key_config[index].bottom_value) {
            key_config[index].pressed = true;
            key_config[index].rt_threshold = value;
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
        if(value > key_config[index].top_value || value > key_config[index].release_value[active_profile]) {
            key_config[index].pressed = false;
        } else if(value < key_config[index].trigger_value[active_profile] || value < key_config[index].bottom_value) {
            key_config[index].pressed = true;
        } else {
            return false;
        }
        break;
        #endif //defined USE_NONE

        #if defined USE_RAPID_TRIGGER
        case rapid_trigger:
        // Rapid trigger is only active when the switch is lower than the trigger and release height
        if(value > key_config[index].top_value || value > key_config[index].release_value[active_profile]) {
            key_config[index].pressed = false;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
        } else if(value < key_config[index].trigger_value[active_profile]) {
            // Set the new lowest value if needed
            if(value < key_config[index].rt_threshold || value < key_config[index].bottom_value) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value - key_config[index].rt_threshold) > key_config[index].rt_release_value[active_profile]) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value - key_config[index].rt_press_value[active_profile];
            }
        }
        break;
        #endif // defined USE_RAPID_TRIGGER

        #if defined USE_CONTINUOUS_RAPID_TRIGGER
        case continuous_rapid_trigger:
        if(value > key_config[index].top_value || value > key_config[index].release_value) {
            key_config[index].pressed = true;
            key_config[index].rt_threshold = key_config[index].trigger_value[active_profile];
            key_config[index].rt_active = false;
        // Rapid trigger activates below the trigger height, but only stops when fully released
        } else if(key_config[index].rt_active || (value < key_config[index].trigger_value[active_profile])) {
            key_config[index].rt_active = true;
            // Set the new lowest value if needed
            if(value < key_config[index].rt_threshold || value < key_config[index].bottom_value) {
                key_config[index].pressed = true;
                key_config[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value - key_config[index].rt_threshold) > key_config[index].rt_release_value[active_profile]) {
                key_config[index].pressed = false;
                // Set the new activation threshold
                key_config[index].rt_threshold = value - key_config[index].rt_press_value[active_profile];
            }
        }
        break;
        #endif // defined USE_CONTINUOUS_RAPID_TRIGGER

        #if defined USE_CONSTANT_RAPID_TRIGGER
        case constant_rapid_trigger:
        if(value > key_config[index].top_value) {
            key_config[index].pressed = false;
        // Check if the key has been pressed far enough for rapid trigger to activate it, or pressed down completely
        } else if(value < key_config[index].rt_threshold || value < key_config[index].bottom_value) {
            key_config[index].pressed = true;
            key_config[index].rt_threshold = value;
        // Check if the key has been released far enough
        } else if((value - key_config[index].rt_threshold) > key_config[index].rt_release_value[active_profile]) {
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
#define OUTPUT_SAVED dprintf("Saved ")
#else
#define OUTPUT_VAL(value, side, index)
#define OUTPUT_SAVED
#endif
bool update_switch_bounds(uint8_t index, uint16_t value) {
    #ifndef INVERT_ADC
    // Allow disabling of the top bound check, as it's already updated every start anyway
    #ifndef NO_TOP_UPDATE
    if(value >= (key_config[index].top_value + am_keyboard_data.dc_delta + top_deadzones[index])){
        key_config[index].top_value = value - top_deadzones[index];
        OUTPUT_VAL(value, "top", index);
        return true;
    } else
    #endif
    if (value <= (key_config[index].bottom_value - am_keyboard_data.dc_delta - bottom_deadzone)) {
        key_config[index].bottom_value = value + bottom_deadzone;
        #ifndef AM_NO_EEPROM
        if(calibration_data[index] - am_keyboard_data.dc_delta > value || calibration_data[index] + am_keyboard_data.dc_delta < value) {
            calibration_data[index] = value;
            OUTPUT_SAVED;
        }
        #endif
        OUTPUT_VAL(value, "bottom", index);
        return true;
    }

    #else // ifndef INVERT_ADC
    #ifndef NO_TOP_UPDATE
    if(value < key_config[index].top_value - am_keyboard_data.dc_delta - top_deadzones[index]){
        key_config[index].top_value = value + top_deadzones[index];
        OUTPUT_VAL(value, "top", index);
        return true;
    } else
    #endif
    if (value > key_config[index].bottom_value + am_keyboard_data.dc_delta + bottom_deadzone) {
        key_config[index].bottom_value = value - bottom_deadzone;
        #ifndef AM_NO_EEPROM
        if(calibration_data[index] + am_keyboard_data.dc_delta < value || calibration_data[index] - am_keyboard_data.dc_delta > value) {
            calibration_data[index] = value;
            OUTPUT_SAVED;
        }
        #endif
        OUTPUT_VAL(value, "top", index);
        return true;
    }
    #endif // ifndef INVERT_ADC else

    return false;
}
#endif


//MARK: Switch data
// Populates the key matrix with the static config params, heights are populated separately
void get_key_config(void) {
    // Slave values are less stable than master values, this allows easily setting separate values per half
    #ifdef SPLIT_KEYBOARD
    #ifdef RIGHT_MULTIPLIER
    if(!is_keyboard_left()){
        top_deadzone *= (am_keyboard_data.right_mult / 100.0);
        bottom_deadzone *= (am_keyboard_data.right_mult / 100.0);
        smoothing *= (am_keyboard_data.right_mult / 100.0);
        //TODO: Update this for VIA_FILTER stuff
        #if FILTER_STRENGTH != SLAVE_FILTER_STRENGTH
        adc_filter = adc_slave_filter;
        #endif
    }
    #endif
    #if defined SLAVE_MULTIPLIER
    if(!is_keyboard_master()){
        //TODO: Test how rounding is handled
        top_deadzone *= (am_keyboard_data.slave_mult / 100.0);
        bottom_deadzone *= (am_keyboard_data.slave_mult / 100.0);
        smoothing *= (am_keyboard_data.slave_mult / 100.0);
        #if !defined RIGHT_FILTER_STRENGTH && (FILTER_STRENGTH != SLAVE_FILTER_STRENGTH)
        adc_filter = adc_slave_filter;
        #endif
    }
    #endif
    // After the ADC deadzones have been multiplied, add the user deadzones
    top_deadzone += am_keyboard_data.top_deadzone;
    bottom_deadzone += am_keyboard_data.bottom_deadzone;
    for (uint8_t key = 0; key < switch_num; key++) top_deadzones[key] = top_deadzone < 255 ? top_deadzone : 255;
    #endif

    wait_ms(AM_STARTUP_DELAY);
    // Dummy reads because the first read is wrong
    adc_read(adc_pin_mux[0]);
    adc_read(adc_pin_mux[0]);

    for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
        set_sensor_power_high(mux_channel);
        set_mux_channel(mux_channel);
        for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
            const uint8_t index = mux_to_num[mux_channel][adc_channel];
            if(index == 255) continue;

            wait_cycles(ADC_SCAN_DELAY);

            //TODO: Test if it's worth taking the average value of 5 actual cycles instead of scanning every key 5 times in a row
            //TODO: Maybe take the lowest value instead of the average?
            // Take the average of 5 scans for higher stability
            // uint16_t adc_value = adc_read(adc_pin_mux[adc_channel]);;
            // // Take the lowest value
            // //TODO: Also do this for inverse adc
            // for(uint8_t scan = 0; scan < 4; scan++) {
            //     const uint16_t new_value = adc_read(adc_pin_mux[adc_channel]);
            //     if(new_value < adc_value) adc_value = new_value;
            // }

            // Take the average of 8 scans
            uint16_t adc_value = 0;
            for (uint8_t scan = 0; scan < 8; scan++) {
                adc_value += adc_read(adc_pin_mux[adc_channel]);
            }
            adc_value >>= 3;

            #ifndef INVERT_ADC
            key_config[index].top_value = adc_value - top_deadzone;
            #else
            key_config[index].top_value = adc_value + top_deadzone;
            #endif
            // Assign this value here to prime the filter
            key_config[index].scan_value = adc_value;

            key_config[index].row = num_to_matrix[index][0];
            key_config[index].col = num_to_matrix[index][1];

            for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++){
                // The MSB contains the priority state
                key_config[index].mode[profile] = am_keyboard_data.key_mode[profile][index + switch_low] & 0b01111111;
            }

            #if defined JOYSTICK_ENABLE
            key_config[index].axis_index = -1;
            #endif
        }
        set_sensor_power_low(mux_channel);
    }
}


//MARK: Init helpers
void clear_calibration(void) {
    memset(&calibration_data, 0, sizeof(calibration_data));
    eeconfig_update_keyboard((uint16_t*)&calibration_data);
    memset(&top_deadzones, 0, sizeof(top_deadzones));
    eeconfig_update_deadzone((uint8_t*)&top_deadzones);

    // Also clear the data on the slave
    #ifdef SPLIT_KEYBOARD
    if(is_keyboard_master()) am_data_manual_transaction(clear_calibration_values);
    #endif
}

// Helper functions because the current init key implementation requires taking a bool for the calibration function
void _bootmagic(bool init) {
    clear_calibration();
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
    // wait_cycles(3000);
    // set_mux_channel(FORCE_BOOTLOADER_CHANNEL);
    // #endif
    // // Dummy read, first read is way off
    // adc_read(pinToMux(FORCE_BOOTLOADER_PIN));
    // wait_cycles(32000);
    // uint16_t val = adc_read(pinToMux(A0));
    // // printf("Val: %u", val);
    // // set_mux_channel(0);
    // if(val < 2000) {
    //     LED_ON;
    //     bootloader_jump();
    // }
// }


//MARK: wait_cycles
//TODO: Scale this so that it's roughly correct
#ifdef AM_USE_DELAY
void wait_cycles(uint16_t delay) {
    wait_cpuclock(delay);
    // for(; delay > 0; delay--){
    //     __asm("");
    // }
}

/* Maybe this is an option for scaling
#define NUMBER_NOPS 6
#define CYCLES_PER_SEC (CPU_CLOCK / NUMBER_NOPS * WS2812_BITBANG_NOP_FUDGE)
#define NS_PER_SEC (1000000000L) // Note that this has to be SIGNED since we want to be able to check for negative values of derivatives
#define NS_PER_CYCLE (NS_PER_SEC / CYCLES_PER_SEC)
#define NS_TO_CYCLES(n) ((n) / NS_PER_CYCLE)

#define wait_ns(x)                                  \
    do {                                            \
        for (int i = 0; i < NS_TO_CYCLES(x); i++) { \
            __asm__ volatile("nop\n\t"              \
                             "nop\n\t"              \
                             "nop\n\t"              \
                             "nop\n\t"              \
                             "nop\n\t"              \
                             "nop\n\t");            \
        }                                           \
    } while (0)
     */
#endif


//MARK: Profiles
// If true, layer_state_set doesn't update the profile together with the layer.

#if AM_PROFILE_NUM > 1
uint8_t get_active_profile(void) { return active_profile; }

// Returns true if profile_lock is active
bool get_profile_lock_state(void) {
    return !!(am_keyboard_data.profile_num & 0b10000000);
}

// Saves the current state of profile_lock
void set_profile_lock_state(bool state) {
    if(state) am_keyboard_data.profile_num |= 0b10000000;
    else am_keyboard_data.profile_num &= 0b01111111;
}

// Makes all changes that need to happen on a profile change
void profile_state_changed(uint8_t profile) {
    //TODO: If am_data_manual_transaction is split up into many separate ones for each piece of data, send the profile change here
    // if(is_keyboard_master()) am_data_manual_transaction(normal_transaction);

    #ifdef USE_PRIORITY_MODE
    priority_mode = !!(am_keyboard_data.priority_profiles & 1 << profile);

    // If VIA is enabled, the priority indices are assigned per profile
    #if defined VIA_ENABLE && defined PRIORITY_INDICES
    for(uint8_t key = 0; key < switch_num; key++) priority_indices[key] = !!(am_keyboard_data.key_mode[profile][key + switch_low] & 1 << 7);
    #endif
    #endif
}

#ifndef SPLIT_KEYBOARD
void set_active_profile(uint8_t profile) { active_profile = profile; }
#else // ifndef SPLIT_KEYBOARD
void set_active_profile(uint8_t profile) {
    if(is_keyboard_master()) {
        active_profile = profile;

        // Start the transaction
        am_data_manual_transaction(normal_transaction);
    }
    profile_state_changed(profile);
}
#endif // ifndef SPLIT_KEYBOARD else

#else // if AM_PROFILE_NUM > 1
#   define set_active_profile(profile)
#   define profile_state_changed(profile)
#   define get_active_profile 0
#   define toggle_profile_lock()
#   define set_profile_lock(value)
#endif // if AM_PROFILE_NUM > 1


void reset_matrix_keys(void) {
    for(uint8_t index = 0; index < switch_num; index++) {
        key_config[index].pressed = false;
    }
    memset(&matrix, 0, sizeof(matrix));
}


//MARK: Layer state
layer_state_t layer_state_set_am(layer_state_t state) {
    am_highest_layer = get_highest_layer(state);

    change_layer_settings(am_highest_layer);

    #if PROFILE_SWITCH_MODE != MANUAL_PROFILE
    if((am_keyboard_data.profile_config & 0x0F) == MANUAL_PROFILE) return state;
    if (!get_profile_lock_state()) {
        for(uint8_t profile = 0; profile < AM_PROFILE_NUM; profile++) {
            // If the highest active layer is in the layers list of that profile, activate it
            if(am_keyboard_data.profile_layers[profile] & (1 << am_highest_layer)) {
                set_active_profile(profile);
                // Only the lowest profile should apply
                return state;
            }
        }
        // If profile switch mode is default, switch to the default profile if layer is not set for any profile
        if((am_keyboard_data.profile_config & 0x0F) == DEFAULT_PROFILE) {
            set_active_profile(am_keyboard_data.profile_config >> 4);
            return state;
        }
    }
    #endif // if PROFILE_SWITCH_MODE != MANUAL_PROFILE

    #if defined SPLIT_LAYER_SYNC
    // If the slave hasn't been synced at this point, do so manually
    am_data_manual_transaction(normal_transaction);
    #endif

    return state;
}


//MARK: Filter functions
//TODO: For the VIA_FILTER_STRENGTH stuff, I could either go through a switch statement or set many different filter functions and swap the pointer around based on the selection
#if FILTER_STRENGTH != 0
uint16_t adc_filter_function(uint16_t value, uint8_t index) {
// Guarded to allow overwriting the filter with a different implementation
#ifndef ADC_FILTER
// This filter seems more efficient than value -= (value - key_config[index].scan_value) >> 1
#if   FILTER_STRENGTH == 4 // 1/4 new, 3/4 old
#   define ADC_FILTER(value, index) (value >> 2) + ((key_config[index].scan_value * 3) >> 2)
#elif FILTER_STRENGTH == 3 // 1/2 new, 1/2 old
#   define ADC_FILTER(value, index) (value >> 1) + (key_config[index].scan_value >> 1)
#elif FILTER_STRENGTH == 2 // 3/4 new, 1/4 old
#   define ADC_FILTER(value, index) ((value * 3) >> 2) + (key_config[index].scan_value >> 2)
#elif FILTER_STRENGTH == 1   // 7/8 new, 1/8 old
#   define ADC_FILTER(value, index) ((value * 7) >> 3) + (key_config[index].scan_value >> 3)
#else
#   error "Invalid filter strength. Only values 0 - 4 are allowed."
#endif
// #define ADC_FILTER(value, index) value - (value - key_config[index].scan_value) >> 2
#endif
    return ADC_FILTER(value, index);
}

#if FILTER_STRENGTH != SLAVE_FILTER_STRENGTH || defined ADC_SLAVE_FILTER
uint16_t adc_slave_filter_function(uint16_t value, uint8_t index) {
#ifndef ADC_SLAVE_FILTER
// This filter seems more efficient than value -= (value - key_config[index].scan_value) >> 1
#if   SLAVE_FILTER_STRENGTH == 4 // 1/4 new, 3/4 old
#   define ADC_SLAVE_FILTER(value, index) (value >> 2) + ((key_config[index].scan_value * 3) >> 2)
#elif SLAVE_FILTER_STRENGTH == 3 // 1/2 new, 1/2 old
#   define ADC_SLAVE_FILTER(value, index) (value >> 1) + (key_config[index].scan_value >> 1)
#elif SLAVE_FILTER_STRENGTH == 2 // 3/4 new, 1/4 old
#   define ADC_SLAVE_FILTER(value, index) ((value * 3) >> 2) + (key_config[index].scan_value >> 2)
#elif SLAVE_FILTER_STRENGTH == 1   // 7/8 new, 1/8 old
#   define ADC_SLAVE_FILTER(value, index) ((value * 7) >> 3) + (key_config[index].scan_value >> 3)
#else
#   error "Invalid filter strength. Only values 0 - 4 are allowed."
#endif
#endif
    return ADC_SLAVE_FILTER(value, index);
}
#endif
#else // if FILTER_STRENGTH != 0
#define ADC_FILTER(value) value
#endif


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

        set_sensor_power_high(mux_channel);
        if(mux_channel != last_channel) {
            set_mux_channel(mux_channel);
            last_channel = mux_channel;
        }

        adc_value = adc_read(adc_pin_mux[adc_channel]);
        #ifdef DEBUG_MUX_POSITION
        if(adc_channel == debug_mux[0] && mux_channel == debug_mux[1]) {
            dprintf("%u\n", adc_value);
        }
        #endif
        wait_cycles(ADC_SCAN_CYCLES);

        if(evaluate_value(matrix_index, adc_value)) {
            matrix_has_changed = true;
            #ifndef DEBUG_SCAN_NO_INPUT
            current_matrix[key_config[matrix_index].row] ^= 1 << key_config[matrix_index].col;
            #endif
        }

        set_sensor_power_low(mux_channel);
    }
    matrix_scan_kb();

    return matrix_has_changed;
}
#endif


//MARK: Suspend
//TODO: Instead of using invert matrix power like this, instead use an on_state define and use gpio_write_pin(pin, state) and gpio_write_pin(pin, !state) to turn on/off respectively
#ifdef MATRIX_POWER_PIN
#include "suspend.h"
__attribute__((weak)) void suspend_power_down_kb(void) {
    #ifndef INVERT_MATRIX_POWER
    gpio_write_pin_low(MATRIX_POWER_PIN);
    #else
    gpio_write_pin_high(MATRIX_POWER_PIN);
    #endif

    suspend_power_down_user();
}

__attribute__((weak)) void suspend_wakeup_init_kb(void) {
    #ifndef INVERT_MATRIX_POWER
    gpio_write_pin_high(MATRIX_POWER_PIN);
    #else
    gpio_write_pin_low(MATRIX_POWER_PIN);
    #endif
    wait_ms(5);

    suspend_wakeup_init_user();
}
#endif


//MARK: Power pins
#ifdef POWER_BEFORE_SCAN
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

    wait_cycles(POWER_SELECT_DELAY);
}

/* Weak defines, to allow a custom powering logic */
#elif defined CUSTOM_POWER_BEFORE_SCAN
__attribute__((weak)) void sensor_power_init(void) { sensor_power_init_user(); }

__attribute__((weak)) void set_sensor_power_high(uint8_t mux_channel) { set_sensor_power_high_user(mux_channel); }

__attribute__((weak)) void set_sensor_power_low(uint8_t mux_channel) { set_sensor_power_low_user(mux_channel); }

__attribute__((weak)) void sensor_power_toggle(uint8_t mux_channel, uint8_t adc_channel) { sensor_power_toggle_user(mux_channel, adc_channel); }

__attribute__((weak)) void sensor_power_init_user(void) {}

__attribute__((weak)) void set_sensor_power_high_user(uint8_t mux_channel) {}

__attribute__((weak)) void set_sensor_power_low_user(uint8_t mux_channel) {}

__attribute__((weak)) void sensor_power_toggle_user(uint8_t mux_channel, uint8_t adc_channel) {}
#endif





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
