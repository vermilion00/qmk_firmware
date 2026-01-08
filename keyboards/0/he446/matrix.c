//TODO: Go through these and check which ones are needed

#include "matrix.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
// #include <stdio.h>
// #include <string.h>
// #include <sys/cdefs.h>
// #include "_wait.h"
// #include "action_layer.h"
// #include "analog.h"
#include "bootloader.h"
#include "debug.h"
// #include "gpio.h"
// #include "hal_pal.h"
// #include "hal_pal_lld.h"
#include "he_matrix.h"
#include "info_config.h"
#include "keyboard.h"
// #include "keycodes.h"
#include "multiplexer.h"
// #include "atomic_util.h"
#include "print.h"
// #include "stm32_gpio.h"
#include "suspend.h"
#include "transaction_id_define.h"
#include "transactions.h"
#include "eeconfig.h"


// Get the switch data configured in the json
void get_switch_data(void);
// Translate the user defined trigger height etc into the equivalent ADC values
void translate_mm_to_value(void);
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

#if DYNAMIC_CALIBRATION == TRUE
// Check if the switch boundaries need updating, and update them if necessary.
inline bool update_switch_bounds(uint8_t index, uint16_t value);
#endif

#if HE_INIT_KEY_NUM > 0
// Initialize the keys to be checked at initialization
static void scan_init_keys(void);
SPLIT_MUTABLE uint8_t init_keys[HE_INIT_KEY_NUM][2] = HE_INIT_KEYS;
void (*init_functions[HE_INIT_KEY_NUM])(void) = HE_INIT_FUNCTIONS;
#endif

#if defined DEBUG_SCAN_VALUE || DEBUG_SCAN_VALUE_R
uint8_t debug_mux[2] = DEBUG_SCAN_VALUE;
#endif

#ifdef POWER_PINS
// Set the sensor power pins and delay, if defined
static inline void set_sensor_power(uint8_t index);
#endif

PROFILE_MUTABLE uint8_t current_he_profile = HE_DEFAULT_PROFILE;

SPLIT_MUTABLE uint8_t switch_num = SWITCH_NUM;

/* Start of decl */
#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
Switch he_matrix[MAX(SWITCH_NUM, SWITCH_NUM_R)];
uint8_t mux_to_num[MAX(MUX_CHANNELS, MUX_CHANNELS_R)][ADC_PIN_NUM] = MUX_TO_NUM;
const uint8_t mux_to_num_r[MAX(MUX_CHANNELS, MUX_CHANNELS_R)][ADC_PIN_NUM] = MUX_TO_NUM_R;
uint8_t num_to_matrix[MAX(SWITCH_NUM, SWITCH_NUM_R)][2] = NUM_TO_MATRIX;
const uint8_t num_to_matrix_r[MAX(SWITCH_NUM, SWITCH_NUM_R)][2] = NUM_TO_MATRIX_R;
uint8_t matrix_to_num[MATRIX_ROWS][MATRIX_COLS] = MATRIX_TO_NUM;
const uint8_t matrix_to_num_r[MATRIX_ROWS][MATRIX_COLS] = MATRIX_TO_NUM_R;
uint8_t num_to_mux[MAX(SWITCH_NUM, SWITCH_NUM_R)][2] = NUM_TO_MUX;
const uint8_t num_to_mux_r[MAX(SWITCH_NUM, SWITCH_NUM_R)][2] = NUM_TO_MUX_R;
uint8_t key_modes[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = KEY_MODES;
const uint8_t key_modes_r[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = KEY_MODES_R;

#if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
float trigger_height[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = TRIGGER_HEIGHT;
float release_height[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RELEASE_HEIGHT;
const float trigger_height_r[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = TRIGGER_HEIGHT_R;
const float release_height_r[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RELEASE_HEIGHT_R;
#endif
#if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
float rt_press_distance[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RT_PRESS_DISTANCE;
float rt_release_distance[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RT_RELEASE_DISTANCE;
const float rt_press_distance_r[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RT_PRESS_DISTANCE_R;
const float rt_release_distance_r[HE_PROFILE_NUM][MAX(SWITCH_NUM, SWITCH_NUM_R)] = RT_RELEASE_DISTANCE_R;
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

#if HE_INIT_KEY_NUM > 0
const uint8_t init_keys_r[HE_INIT_KEY_NUM_R][2] = HE_INIT_KEYS_R;
const void (*init_functions_r[HE_INIT_KEY_NUM_R])(void) = HE_INIT_FUNCTIONS_R;
#endif

//TODO: Implement this
#if defined DEBUG_SCAN_VALUE_R
uint8_t debug_mux_r[2] = DEBUG_SCAN_VALUE_R;
#endif
//etc

#else // if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
//TODO: I dont need this, just declare the left side as split mutable and only declare right
//      side if side is unknown
Switch he_matrix[SWITCH_NUM];
#ifdef MUX_PINS
SPLIT_MUTABLE pin_t mux_pins[MUX_PIN_NUM] = MUX_PINS;
#endif
SPLIT_MUTABLE pin_t adc_pins[ADC_PIN_NUM] = ADC_PINS;
#ifdef POWER_PINS
SPLIT_MUTABLE pin_t power_pins[POWER_PIN_NUM] = POWER_PINS;
#endif

SPLIT_MUTABLE uint8_t key_modes[HE_PROFILE_NUM][SWITCH_NUM] = KEY_MODES;

// Used to translate from the ADC pin/Mux combination to the switch number
SPLIT_MUTABLE uint8_t mux_to_num[MUX_CHANNELS][ADC_PIN_NUM] = MUX_TO_NUM;
// Used to translate from the switch number to the QMK layout position
SPLIT_MUTABLE uint8_t num_to_matrix[SWITCH_NUM][2] = NUM_TO_MATRIX;
// Used to translate from the QMK layout position to the switch number
SPLIT_MUTABLE uint8_t matrix_to_num[MATRIX_ROWS][MATRIX_COLS] = MATRIX_TO_NUM;
// Used to translate from the matrix index to the ADC pin/Mux combination
SPLIT_MUTABLE uint8_t num_to_mux[SWITCH_NUM][2] = NUM_TO_MUX;

#if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
SPLIT_MUTABLE float trigger_height[HE_PROFILE_NUM][SWITCH_NUM] = TRIGGER_HEIGHT;
SPLIT_MUTABLE float release_height[HE_PROFILE_NUM][SWITCH_NUM] = RELEASE_HEIGHT;
#endif

#if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
SPLIT_MUTABLE float rt_press_distance[HE_PROFILE_NUM][SWITCH_NUM] = RT_PRESS_DISTANCE;
SPLIT_MUTABLE float rt_release_distance[HE_PROFILE_NUM][SWITCH_NUM] = RT_RELEASE_DISTANCE;
#endif
#endif

// During initialization, the adc pins are translated to the adc mux combination that the adc_read function uses
adc_mux adc_pin_mux[ADC_PIN_NUM];
/* End of decl */

//TODO: Remove this
// #define SPLIT_KEYBOARD
#ifdef SPLIT_KEYBOARD
uint8_t calibration_done = false;

typedef struct _slave_to_master_t {
    // uint64_t calibration_data[DATA_BUFFER_LEN];
    // uint16_t calibration_data[(SWITCH_NUM + SWITCH_NUM_R) * 2];
    uint16_t top_values[MAX(SWITCH_NUM_L, SWITCH_NUM_R)];
    uint16_t bottom_values[MAX(SWITCH_NUM_L, SWITCH_NUM_R)];
} slave_to_master_t;
slave_to_master_t slave_data;

#if KEYBOARD_SIDE == RIGHT
const bool keyboard_left = RIGHT;
#elif KEYBOARD_SIDE == LEFT
const bool keyboard_left = LEFT;
#else // KEYBOARD_SIDE == UNKNOWN
bool keyboard_left;
// Switch* he_matrix = &he_matrix_l[0];
void assign_split_side(bool side);
#endif
#endif // if defined SPLIT_KEYBOARD

//TODO: Assign correct side at init
#ifdef PRIORITY_MUXES
uint8_t scan_amt = 0;
uint8_t priority_muxes[PRIORITY_MUX_NUM][3] = PRIORITY_MUXES;
uint8_t matrix_scan_priority(matrix_row_t current_matrix[]);
#endif

#ifdef PRIORITY_INDICES
uint8_t scan_amt = 0;
uint8_t priority_indices[PRIORITY_INDEX_NUM] = PRIORITY_INDICES;
#endif

//MARK: Init
void matrix_init_custom(void) {
    //TODO: Abstract this (Enables FPU)
    //TODO: This seems to make the scan slightly slower (3150 instead of 3172)
    // SCB->CPACR |= ((3UL << 20U)|(3UL << 22U));  /* set CP10 and CP11 Full Access */
    //TODO: Determine side first
    #ifdef SPLIT_KEYBOARD
    // Determine keyboard half
    #if KEYBOARD_SIDE == UNKNOWN
    // keyboard_left = is_keyboard_left();
    // if((!keyboard_left)) {
        //     *he_matrix = he_matrix_r[0];
        // }

    assign_split_side(is_keyboard_left());
    #endif // if KEYBOARD_SIDE == UNKNOWN
    // Set switch num based on sid
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
    //TODO: Check side if split keyboard
    #elif defined MATRIX_POWER_PIN
    gpio_set_pin_output_push_pull(MATRIX_POWER_PIN);
    #ifndef INVERT_MATRIX_POWER
    gpio_write_pin_high(MATRIX_POWER_PIN);
    #else
    gpio_write_pin_low(MATRIX_POWER_PIN);
    #endif
    #endif

    get_switch_data();

    // TODO: Load key matrix struct with calibration and distance data from the EEPROM
    // Get the min/max values of each switch from EEPROM
    if(!get_calibration_data()) {
    // If loading the calibration data fails, start calibration immediately
        calibrate_switches();
    }

    // Translate the trigger height etc into the equivalent ADC value
    translate_mm_to_value();

    // Check keys like the calibration key or bootmagic key before scanning begins
    #if HE_INIT_KEY_NUM > 0
    scan_init_keys();
    #endif


    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}


//MARK: Init keys
#if HE_INIT_KEY_NUM > 0
static void scan_init_keys(void) {
    uint16_t adc_value;
    // Long delay needed for correct init key reading after being plugged in
    delay_ns(20000);

    for(uint8_t idx = 0; idx < HE_INIT_KEY_NUM; idx++) {
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

        // if(evaluate_value(matrix_index, adc_value)) {
        #ifndef INVERT_ADC
        if(adc_value < he_matrix[matrix_index].bottom_value + 50) {
        #else
        if(adc_value > he_matrix[matrix_index].bottom_value - 50) {
        #endif
            // If the key is activated, call the respective function
            // These functions are set in the INIT_FUNCTIONS dict at the top of he_config_h.py
            (*init_functions[idx])();
        }
    }
}
#endif // HE_INIT_KEY NUM > 0


//TODO: Would it be faster to save the mux info to the switch, loop through the he_matrix indices,
//      and sort the he_matrix by mux_channel, like the priority muxes?
//      Maybe even sort the adc channels to be ascending, then descending,
//      so at least one adc channel is used twice in a row, if that makes a difference
//MARK: Scan
uint8_t matrix_scan_custom(matrix_row_t current_matrix[]) {
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

        // delay_ns(10);
        // wait_us(7);

        for(uint8_t adc_channel = 0; adc_channel < ADC_PIN_NUM; adc_channel++) {
            // Translate matrix mux and adc channels to matrix position
            index = mux_to_num[mux_channel][adc_channel];
            // Check if a switch is at the position (matrix index > 0), and scan if so
            if(index > 0){

                //TODO: This doesn't seem fast enough
                #ifdef PRIORITY_INDICES
                if(scan_amt < PRIORITY_LEVEL) {
                    bool scan = false;
                    for(uint8_t idx = 0; idx < PRIORITY_INDEX_NUM; idx++) {
                        if(index == priority_indices[idx]) {
                            scan = true;
                        }
                    }
                    if(!scan) { continue; }
                } else { scan_amt = 0; }
                #endif

                index -= 1;
                adc_value = adc_read(adc_pin_mux[adc_channel]);

                #ifdef DEBUG_SCAN_VALUE
                if(adc_channel == debug_mux[0] && mux_channel == debug_mux[1]) {
                    dprintf("%u\n", adc_value);
                }
                #elif DEBUG_SCAN_VALUES == TRUE
                dprintf("%u/%2u: %3u, ", adc_channel, mux_channel, adc_value);
                #endif
                #ifdef ADC_SCAN_DELAY
                delay_ns(ADC_SCAN_CYCLES);
                #endif

                // Check if key is pressed/released and set matrix_has_changed, returns true if the switch state has changed
                if(evaluate_value(index, adc_value)) {
                    matrix_has_changed = true;
                    #if DEBUG_SCAN_NO_INPUT != TRUE
                    current_matrix[he_matrix[index].row] ^= 1 << he_matrix[index].col;
                    #endif
                    // If dynamic calibration is enabled, check if the boundaries need updating
                    #if DYNAMIC_CALIBRATION == TRUE
                    if(update_switch_bounds(index, adc_value)) {
                        // If the bounds have been updated, translate the heights and save the new bounds
                        translate_mm_to_value(index);
                        #if NO_EEPROM == FALSE
                        save_calibration_data();
                        #endif
                    }
                    #endif // DYNAMIC_CALIBRATION == TRUE
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
    scan_amt += 1;
    printf("%u\n", scan_amt);
    #endif


    // This *must* be called for correct keyboard behavior
    matrix_scan_kb();

    return matrix_has_changed;
}


//MARK: Translate
// Translate the trigger height etc of all keys into the corresponding ADC values
void translate_mm_to_value(void) {
    uint8_t key_modes[][SWITCH_NUM] = KEY_MODES;

    for(uint8_t index = 0; index < switch_num; index++) {
        uint16_t bottom_value = he_matrix[index].bottom_value;
        uint16_t top_value = he_matrix[index].top_value;
        #if INVERT_ADC == FALSE
        // ADC count per mm of travel
        uint16_t travel_unit = floor((top_value - bottom_value) / TRAVEL_DISTANCE);
        #else
        uint16_t travel_unit = floor((bottom_value - top_value) / TRAVEL_DISTANCE);
        #endif

        for(uint8_t profile = 0; profile < HE_PROFILE_NUM; profile++) {
            #if INVERT_ADC == FALSE
            #if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
            #if DISTANCE_FROM_BOTTOM == FALSE
            he_matrix[index].trigger_value[profile] = top_value - (travel_unit * trigger_height[profile][index]) - ADC_SMOOTHING;
            he_matrix[index].release_value[profile] = top_value - (travel_unit * release_height[profile][index]) + ADC_SMOOTHING;

            #else // if DISTANCE_FROM_BOTTOM == FALSE
            he_matrix[index].trigger_value[profile] = travel_unit * trigger_height[profile][index] + bottom_value - ADC_SMOOTHING;
            he_matrix[index].release_value[profile] = travel_unit * release_height[profile][index] + bottom_value + ADC_SMOOTHING;
            #endif // else DISTANCE_FROM_BOTTOM == FALSE
            #endif

            #else //Inverted ADC -> Lower switch means higher value

            #if defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER
            #if DISTANCE_FROM_BOTTOM == FALSE
            //TODO: Is this correct
            he_matrix[index].trigger_value[profile] = top_value + (travel_unit * trigger_height[profile][index]) + ;
            he_matrix[index].release_value[profile] = top_value + (travel_unit * release_height[profile][index]);

            #else // if DISTANCE_FROM_BOTTOM == FALSE
            he_matrix[index].trigger_value[profile] = bottom_value - (travel_unit * trigger_height[profile][index]);
            he_matrix[index].release_value[profile] = bottom_value - (travel_unit * release_height[profile][index]);
            #endif // else DISTANCE_FROM_BOTTOM == FALSE
            #endif // if RAPID_TRIGGER_TYPE != CONSTANT_RAPID_TRIGGER
            #endif //else INVERT ADC == TRUE

            #if defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER
            he_matrix[index].rt_press_value[profile] = travel_unit * rt_press_distance[profile][index];
            he_matrix[index].rt_release_value[profile] = travel_unit * rt_release_distance[profile][index] + ADC_SMOOTHING;
            #endif

            // Assign the rt_threshold initially
            //TODO: Do this for all variations
            // #if defined USE_CONSTANT_RAPID_TRIGGER
            // he_matrix[index].rt_threshold = top_value - he_matrix[index].rt_press_value[HE_DEFAULT_PROFILE];
            // #endif

            // Assign the switch mode
            he_matrix[index].mode[profile] = key_modes[profile][index];
        }
    }
}


//MARK: Get calibration
bool get_calibration_data(void) {
    #if CALIBRATE == TRUE
    return false;
    #endif

    #if HE_NO_EEPROM == TRUE
    #if !defined HE_TOP_VALUES || !defined HE_BOTTOM_VALUES
    return false;
    #else
    //TODO: Add unknown side assignment stuff
    const uint16_t top_values[] = HE_TOP_VALUES;
    const uint16_t bottom_values[] = HE_BOTTOM_VALUES;
    #endif

    #if DYNAMIC_CALIBRATION == FALSE
    #if INVERT_ADC == FALSE
    for(uint8_t key = 0; key < switch_num; key++) {
        he_matrix[key].top_value = top_values[key] - ADC_TOP_DEADZONE;
        he_matrix[key].bottom_value = bottom_values[key] + ADC_BOTTOM_DEADZONE;

        he_matrix[key].pressed = false;
    }
    #else // INVERT_ADC == FALSE
    for(uint8_t key = 0; key < switch_num; key++) {
        he_matrix[key].top_value = top_values[key] + ADC_TOP_DEADZONE;
        he_matrix[key].bottom_value = bottom_values[key] - ADC_BOTTOM_DEADZONE;

        he_matrix[key].pressed = false;
    }
    #endif // else INVERT_ADC == FALSE
    #else // DYNAMIC_CALIBRATION == FALSE
    #if INVERT_ADC == FALSE
    for(uint8_t key = 0; key < switch_num; key++) {
        // HE_DC_FACTOR should be defined as < 1
        he_matrix[key].top_value = HE_DC_FACTOR * top_values[key] - ADC_TOP_DEADZONE;
        he_matrix[key].bottom_value = (2 - HE_DC_FACTOR) * bottom_values[key] + ADC_BOTTOM_DEADZONE;
    }
    #else // INVERT_ADC == TRUE
    for(uint8_t key = 0; key < switch_num; key++) {
        he_matrix[key].top_value = (2 - HE_DC_FACTOR) * top_values[key] + ADC_TOP_DEADZONE;
        he_matrix[key].bottom_value = HE_DC_FACTOR * bottom_values[key] - ADC_BOTTOM_DEADZONE;
    }
    #endif // INVERT_ADC
    #endif // DYNAMIC_CALIBRATION

    return true;

#   else //if HE_NO_EEPROM == TRUE
    //TODO: Add eeprom support
    // Read the calibration data from EEPROM

    // Check if the data aligns with configured matrix size

    // Start calibration if necessary
    return true;
    #endif //else HE_NO_EEPROM == TRUE
}


//MARK: Calibrate
void calibrate_switches(void) {
    uint16_t adc_value;
    uint8_t matrix_index;
    // Save config values after x scans without a change
    uint32_t scans_without_change = SCANS_WITHOUT_CHANGE;
    bool first_scan = true;
    char side[7] = "";
    // TODO: Remove this
    // gpio_toggle_pin(B2);

    #ifdef SPLIT_KEYBOARD
    if(is_keyboard_master()) {
        //TODO: Check if NULL size works
        transaction_rpc_send(HE_CALIBRATION_M2S_SYNC, 8, &first_scan);
    }
    if(!is_keyboard_left()) {
        char right[] = "_right";
        memcpy(&side, right, sizeof(right));
    }
    #endif

    //TODO: Remove this when debugging is done
    GPIOB->ODR |= 1 << 2;

    while(true) {
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
                        he_matrix[matrix_index].top_value = adc_value;
                        he_matrix[matrix_index].bottom_value = adc_value - 50;

                        #else
                        he_matrix[matrix_index].top_value = adc_value - 50;
                        he_matrix[matrix_index].bottom_value = adc_value;
                        #endif

                        continue;
                    }

                    #if INVERT_ADC == TRUE
                    if(adc_value < he_matrix[matrix_index].top_value - ADC_TOP_DEADZONE){
                        he_matrix[matrix_index].top_value = adc_value + ADC_TOP_DEADZONE;
                        scans_without_change = 0;
                        calibration_done = false;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new top value %i\n", matrix_index, adc_value);
                        #endif

                    } else if (adc_value > he_matrix[matrix_index].bottom_value + ADC_BOTTOM_DEADZONE) {
                        he_matrix[matrix_index].bottom_value = adc_value - ADC_BOTTOM_DEADZONE;
                        scans_without_change = 0;
                        calibration_done = false;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new bottom value %i\n", matrix_index, adc_value);
                        #endif
                    }

                    #else //if INVERT_ADC == TRUE
                    if(adc_value > he_matrix[matrix_index].top_value + ADC_TOP_DEADZONE){
                        he_matrix[matrix_index].top_value = adc_value - ADC_TOP_DEADZONE;
                        scans_without_change = SCANS_WITHOUT_CHANGE;
                        calibration_done = false;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new top value %i\n", matrix_index, adc_value);
                        #endif
                    } else if (adc_value < he_matrix[matrix_index].bottom_value - ADC_BOTTOM_DEADZONE) {
                        he_matrix[matrix_index].bottom_value = adc_value + ADC_BOTTOM_DEADZONE;
                        scans_without_change = SCANS_WITHOUT_CHANGE;
                        #if DEBUG_CALIBRATION == true
                        dprintf("key %i: new bottom value %i\n", matrix_index, adc_value);
                        #endif
                        calibration_done = false;
                    }
                    #endif//else INVERT_ADC == TRUE
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
        //
        if(scans_without_change/2 >= SCANS_WITHOUT_CHANGE){
            //TODO: Remove this
            // gpio_toggle_pin(B2);

            #if HE_NO_EEPROM == FALSE
            //TODO: Save calibration data to eeprom
            save_calibration();
            #else //NO_EEPROM == FALSE
            // Print the calibration values of each switch so that they can be adjusted in the config
            printf("\"top_values%s\": [ %u", side, he_matrix[0].top_value);
            // Reset the pressed states on all switches
            he_matrix[0].pressed = false;
            //TODO: If only one key is used (per half), this will cause issues
            for(uint8_t index = 1; index < switch_num; index++){
                printf(", %u", he_matrix[index].top_value);
                he_matrix[index].pressed = false;
            }

            printf(" ],\n\"bottom_values%s\": [ %u", side, he_matrix[0].bottom_value);
            for(uint8_t index = 1; index < switch_num; index++){
                printf(", %u", he_matrix[index].bottom_value);
            }
            print(" ]\nYou can paste these lines into keyboard.json under hall_effect.config\n\n");
            #endif //else NO_EEPROM == FALSE

            scans_without_change = 0;
            calibration_done = true;
            #ifdef SPLIT_KEYBOARD
            if(is_keyboard_master()) {
                // uint8_t slave_switch_num;
                // if(is_keyboard_left()) {
                //     slave_switch_num = SWITCH_NUM_R;
                // } else {
                //     slave_switch_num = SWITCH_NUM_L;
                // }
                bool slave_state = false;
                if(transaction_rpc_recv(HE_CALIBRATION_STATE_SYNC, 8, &slave_state)) {
                    printf("Received state %u\n", slave_state);
                } else { print("Failed state sync\n"); }
                // if(slave_state == true) {
                //     #if HE_NO_EEPROM == TRUE
                //     transaction_rpc_recv(HE_CALIBRATION_S2M_SYNC, sizeof(slave_data), &slave_data);
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
            }
            #endif
        }
    }
}


//MARK: Evaluate
//TODO: Dynamically change type of arg based on matrix size
// Create a matrix_index_t enum with nested #if statements checking the matrix size
static inline bool evaluate_value(uint8_t index, uint16_t value) {
    bool prev_pressed = he_matrix[index].pressed;

#if INVERT_ADC == TRUE
    switch(he_matrix[index].mode[current_he_profile]) {
        case none:
        #if defined USE_NONE
        if(value > he_matrix[index].trigger_value[current_he_profile]) {
            he_matrix[index].pressed = true;
        } else if(value < he_matrix[index].release_value[current_he_profile]) {
            he_matrix[index].pressed = false;
        } else {
            return false;
        }
        #endif //defined RAPID_TRIGGER
        break;

        case rapid_trigger:
        #if defined USE_RAPID_TRIGGER
        // Rapid trigger is only active when the switch is lower than the trigger and release height
        if(value > he_matrix[index].trigger_value[current_he_profile]) {
            // Set the new lowest value if needed
            if(value > he_matrix[index].rt_threshold + ADC_SMOOTHING) {
                he_matrix[index].pressed = true;
                he_matrix[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value + he_matrix[index].rt_threshold) < he_matrix[index].rt_release_value[current_he_profile]) {
                he_matrix[index].pressed = false;
                // Set the new activation threshold
                he_matrix[index].rt_threshold = value + he_matrix[index].rt_press_value[current_he_profile];
            }
        } else if(value < he_matrix[index].release_value[current_he_profile]) {
            // If the switch is not pressed past the threshold, reset it
            he_matrix[index].pressed = false;
            he_matrix[index].rt_threshold = he_matrix[index].trigger_value[current_he_profile];
        }
        #endif
        break;

        case continuous_rapid_trigger:
        #if defined USE_CONTINUOUS_RAPID_TRIGGER
        // Rapid trigger activates below the trigger height, but only stops when fully released
        if(he_matrix[index].rt_active || (value > he_matrix[index].trigger_value[current_he_profile] + ADC_SMOOTHING)) {
            he_matrix[index].rt_active = true;
            // Set the new lowest value if needed
            if(value > he_matrix[index].rt_threshold + ADC_SMOOTHING || value > he_matrix[index].bottom_value) {
                he_matrix[index].pressed = true;
                he_matrix[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value + he_matrix[index].rt_threshold) < he_matrix[index].rt_release_value[current_he_profile]) {
                he_matrix[index].pressed = false;
                // Set the new activation threshold
                he_matrix[index].rt_threshold = value + he_matrix[index].rt_press_value[current_he_profile];
            }  else if(value < he_matrix[index].top_value) {
            he_matrix[index].pressed = false;
            he_matrix[index].rt_threshold = he_matrix[index].trigger_value[current_he_profile];
            he_matrix[index].rt_active = false;
            }
        // Check if the switch has been released completely
        } else if(value < he_matrix[index].top_value) {
            he_matrix[index].pressed = false;
            he_matrix[index].rt_threshold = he_matrix[index].trigger_value[current_he_profile];
            he_matrix[index].rt_active = false;
        }
        #endif
        break;

        case constant_rapid_trigger:
        #if defined USE_CONSTANT_RAPID_TRIGGER
        // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
        if((value > he_matrix[index].rt_threshold + ADC_SMOOTHING) || value > he_matrix[index].bottom_value) {
            he_matrix[index].pressed = true;
            he_matrix[index].rt_threshold = value;
        // Check if the key has been released past the threshold or completely
        } else if((value + he_matrix[index].rt_threshold + ADC_SMOOTHING) < he_matrix[index].rt_release_value[current_he_profile]
                  || value < he_matrix[index].top_value) {
            he_matrix[index].pressed = false;
            // Set the new activation threshold
            he_matrix[index].rt_threshold = value + he_matrix[index].rt_press_value[current_he_profile];
        }
        #endif
        break;

        default:
        #if defined USE_SPECIAL
        //TODO: Call special key eval function here
        //evaluate_special(index, value)
        #endif //defined USE_SPECIAL
    }

#else //if INVERT_ADC == TRUE
    switch(he_matrix[index].mode[current_he_profile]) {
        case none:
        #if defined USE_NONE
        if(value < he_matrix[index].trigger_value[current_he_profile]) {
            he_matrix[index].pressed = true;
        } else if(value > he_matrix[index].release_value[current_he_profile]) {
            he_matrix[index].pressed = false;
        } else {
            return false;
        }
        #endif //defined USE_NONE
        break;

        case rapid_trigger:
        #if defined USE_RAPID_TRIGGER
        // Rapid trigger is only active when the switch is lower than the trigger and release height
        if(value < he_matrix[index].trigger_value[current_he_profile]) {
            // Set the new lowest value if needed
            if(value < he_matrix[index].rt_threshold - ADC_SMOOTHING) {
                he_matrix[index].pressed = true;
                he_matrix[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value - he_matrix[index].rt_threshold) > he_matrix[index].rt_release_value[current_he_profile]) {
                he_matrix[index].pressed = false;
                // Set the new activation threshold
                he_matrix[index].rt_threshold = value - he_matrix[index].rt_press_value[current_he_profile];
            }
        //TODO: Check if it is faster to check if the key state is released, and only set values if they're not set already
        } else if(value > he_matrix[index].release_value[current_he_profile]) {
            // If the switch is not pressed past the threshold, reset it
            he_matrix[index].pressed = false;
            he_matrix[index].rt_threshold = he_matrix[index].trigger_value[current_he_profile];
        }
        #endif // defined USE_RAPID_TRIGGER
        break;

        case continuous_rapid_trigger:
        #if defined USE_CONTINUOUS_RAPID_TRIGGER
        // Rapid trigger activates below the trigger height, but only stops when fully released
        if(he_matrix[index].rt_active || (value < he_matrix[index].trigger_value[current_he_profile] - ADC_SMOOTHING)) {
            he_matrix[index].rt_active = true;
            // Set the new lowest value if needed
            if(value < he_matrix[index].rt_threshold - ADC_SMOOTHING || value < he_matrix[index].bottom_value) {
                he_matrix[index].pressed = true;
                he_matrix[index].rt_threshold = value;
            // Check if the key has been released past the threshold
            } else if((value - he_matrix[index].rt_threshold) > he_matrix[index].rt_release_value[current_he_profile]) {
                he_matrix[index].pressed = false;
                // Set the new activation threshold
                he_matrix[index].rt_threshold = value - he_matrix[index].rt_press_value[current_he_profile];
            } else if(value > he_matrix[index].top_value) {
            he_matrix[index].pressed = false;
            he_matrix[index].rt_threshold = he_matrix[index].trigger_value[current_he_profile];
            he_matrix[index].rt_active = false;
            }
        // Check if the switch has been released completely
        } else if(value > he_matrix[index].top_value) {
            he_matrix[index].pressed = false;
            he_matrix[index].rt_threshold = he_matrix[index].trigger_value[current_he_profile];
            he_matrix[index].rt_active = false;
        }
        #endif // defined USE_CONTINUOUS_RAPID_TRIGGER
        break;

        case constant_rapid_trigger:
        #if defined USE_CONSTANT_RAPID_TRIGGER
        // Check if the key has been pressed past far enough for rapid trigger to activate it, or pressed down completely
        if(value < he_matrix[index].rt_threshold - ADC_SMOOTHING || value < he_matrix[index].bottom_value) {
            he_matrix[index].pressed = true;
            he_matrix[index].rt_threshold = value;

        // Check if the key has been released far enough
        } else if((value - he_matrix[index].rt_threshold) > he_matrix[index].rt_release_value[current_he_profile]
                  || value > he_matrix[index].top_value) {
            he_matrix[index].pressed = false;
            // Set the new activation threshold
            he_matrix[index].rt_threshold = value - he_matrix[index].rt_press_value[current_he_profile];
        }
        #endif // defined USE_CONSTANT_RAPID_TRIGGER
        break;

        default:
        #if defined USE_SPECIAL
        //TODO: Call special key eval function here
        //evaluate_special(index, value)
        #endif //defined USE_SPECIAL
    }
#endif //if INVERT_ADC == TRUE else

    return !(prev_pressed == he_matrix[index].pressed);
}


//MARK: Update bounds
#if DYNAMIC_CALIBRATION == TRUE
inline bool update_switch_bounds(uint8_t index, uint16_t value) {
    #if INVERT_ADC == FALSE
    if(value > he_matrix[index].top_value + HE_DC_DELTA + ADC_TOP_DEADZONE){
        he_matrix[index].top_value = value - ADC_TOP_DEADZONE;
        return true;
    } else if (value < he_matrix[index].bottom_value - HE_DC_DELTA - ADC_BOTTOM_DEADZONE) {
        he_matrix[index].bottom_value = value + ADC_BOTTOM_DEADZONE;
        return true;
    }

    #else // if INVERT_ADC == FALSE
    if(value < he_matrix[index].top_value - HE_DC_DELTA - ADC_TOP_DEADZONE){
        he_matrix[index].top_value = value + ADC_TOP_DEADZONE;
        return true;
    } else if (value > he_matrix[index].bottom_value + HE_DC_DELTA + ADC_BOTTOM_DEADZONE) {
        he_matrix[index].bottom_value = value - ADC_BOTTOM_DEADZONE;
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
        he_matrix[key].row = row;
        he_matrix[key].col = col;

        for(uint8_t profile = 0; profile < HE_PROFILE_NUM; profile++){
            he_matrix[key].mode[profile] = key_modes[profile][key];
        }
    }
}


//MARK: Bootmagic
void _bootmagic(void) {
    eeconfig_disable();
    bootloader_jump();
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
#if HE_PROFILE_NUM > 1
#ifndef SPLIT_KEYBOARD
void switch_to_profile(uint8_t profile) { current_he_profile = profile; }
#else
void switch_to_profile(uint8_t profile) {
    if(is_keyboard_master()) {
        current_he_profile = profile;
        // Send the new profile to the slave
        transaction_rpc_send(HE_PROFILE_SYNC, 8, &current_he_profile);
    }
}
#endif // ifndef SPLIT_KEYBOARD else

uint8_t get_current_profile(void) { return current_he_profile; }


//MARK: Layer state
layer_state_t layer_state_set_kb(layer_state_t state) {
    uint8_t highest_layer = get_highest_layer(state);
    for(uint8_t profile = 0; profile < HE_PROFILE_NUM; profile++) {
        // Profile layers is a bitmap, where a 1 means that the profile should be used if on that layer
        // If the highest active layer is in the layers list of that profile, activate it
        if(profiles[profile].layers & (1 << highest_layer)) {
            switch_to_profile(profile);

            // Only the lowest number profile should apply
            return layer_state_set_user(state);
        }
    }
    // If profile switch mode is default, switch to the default profile if layer is not set for any profile
    #if PROFILE_SWITCH_MODE == DEFAULT_PROFILE
    switch_to_profile(HE_DEFAULT_PROFILE);
    #endif

    // Need to call the user function
    return layer_state_set_user(state);
}
#endif // if HE_PROFILE_NUM > 1


#ifdef SPLIT_KEYBOARD
#if KEYBOARD_SIDE == UNKNOWN
//MARK: Split side
void assign_split_side(bool side) {
    if(side == RIGHT) {
        memcpy(&mux_to_num, &mux_to_num_r, sizeof(mux_to_num_r));
        memcpy(&num_to_matrix, &num_to_matrix_r, sizeof(num_to_matrix_r));
        memcpy(&matrix_to_num, &matrix_to_num_r, sizeof(matrix_to_num_r));
        memcpy(&num_to_mux, &num_to_mux_r, sizeof(num_to_mux_r));
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

        #if HE_INIT_KEY_NUM > 0
        memcpy(&init_keys, &init_keys_r, sizeof(init_keys_r));
        memcpy(&init_functions, &init_functions_r, sizeof(init_functions_r));
        #endif
    }
}
#endif // if KEYBOARD_SIDE == UNKNOWN


//TODO: Test if this works to add onto SPLIT_TRANSACTION_IDS_KB
// #ifdef SPLIT_TRANSACTION_IDS_KB
// #define KB_TRANSACTIONS SPLIT_TRANSACTION_IDS_KB
// #undef SPLIT_TRANSACTION_IDS_KB
// #define SPLIT_TRANSACTION_IDS_KB KB_TRANSACTIONS, HE_PROFILE_SYNC, HE_CALIBRATION_SYNC
// #else
// #define SPLIT_TRANSACTION_IDS_KB HE_PROFILE_SYNC, HE_CALIBRATION_SYNC
// #endif


// MARK: Profile sync
void sync_profile_state(uint8_t in_buflen, const void* in_data, uint8_t out_buflen, void* out_data) {
    current_he_profile = *(const uint8_t*)in_data;
}

void sync_calibration_state(uint8_t in_buflen, const void* in_data, uint8_t out_buflen, void* out_data) {
    uint8_t* calibration_state = (uint8_t*)out_data;
    *calibration_state = calibration_done;
}

//MARK: Cal Sync
// uint64_t calibration_data[DATA_BUFFER_LEN];

void split_calibration(uint8_t in_buflen, const void* in_data, uint8_t out_buflen, void* out_data) {
    calibrate_switches();
}

void send_calibration_data(uint8_t in_buflen, const void* in_data, uint8_t out_buflen, void* out_data) {
    uint8_t slave_switch_num = 0;
    slave_to_master_t* cal_data = (slave_to_master_t*)out_data;
    if(is_keyboard_left()) {
        slave_switch_num = SWITCH_NUM_R;
    } else {
        slave_switch_num = SWITCH_NUM_L;
    }

    // Gather the slave side calibration data
    for(uint8_t index = 0; index < slave_switch_num; index++){
        cal_data->top_values[index] = he_matrix[index].top_value;
        cal_data->bottom_values[index] = he_matrix[index].bottom_value;
    }
}

void keyboard_post_init_kb(void) {
    transaction_register_rpc(HE_PROFILE_SYNC, sync_profile_state);
    transaction_register_rpc(HE_CALIBRATION_M2S_SYNC, split_calibration);
    transaction_register_rpc(HE_CALIBRATION_S2M_SYNC, send_calibration_data);
    transaction_register_rpc(HE_CALIBRATION_STATE_SYNC, sync_calibration_state);

    keyboard_post_init_user();
}
#endif // ifdef SPLIT_KEYBOARD

//TODO: Check if I this is a better improvement for non-split keyboards
//      Check how to syncing works, if it's on a timer or smth else, and if throttling
//      the scan rate there would help
#ifdef PRIORITY_MUXES
uint8_t matrix_scan_priority(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;
    uint8_t last_channel = 0;
    uint16_t adc_value = 0;
    set_mux_channel(0);

    //Priority mux is an array of mux_index, adc_channel_index, matrix_index
    for(uint8_t index = 0; index < PRIORITY_MUX_NUM; index++) {
        uint8_t mux_channel = priority_muxes[index][0];
        uint8_t adc_channel = priority_muxes[index][1];
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
        #ifdef DEBUG_SCAN_VALUE
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
            current_matrix[he_matrix[matrix_index].row] ^= 1 << he_matrix[matrix_index].col;
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
