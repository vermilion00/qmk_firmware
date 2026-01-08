#include <stdint.h>
#include "info_config.h"

#define NONE 0
#define RAPID_TRIGGER 1
#define CONTINUOUS_RAPID_TRIGGER 2
#define CONSTANT_RAPID_TRIGGER 3

#define RIGHT 0
#define LEFT 1
#define UNKNOWN 2

// Override KEYBOARD_SIDE if -s flag is used
#if defined SIDE_LEFT
#undef KEYBOARD_SIDE
#define KEYBOARD_SIDE LEFT

#elif defined SIDE_RIGHT
#undef KEYBOARD_SIDE
#define KEYBOARD_SIDE RIGHT
#endif

#ifndef KEYBOARD_SIDE
#   define KEYBOARD_SIDE UNKNOWN
#endif

#if !defined DEBUG_SCAN_VALUE && defined DEBUG_SCAN_VALUE_R
#   define DEBUG_SCAN_VALUE {255, 255}
#endif

// This is set up so that the normal definitions can be used, only need to assign if the side
// is set at init
#if !defined SPLIT_KEYBOARD || KEYBOARD_SIDE == LEFT
#define SPLIT_MUTABLE const

#elif KEYBOARD_SIDE == RIGHT
#define SPLIT_MUTABLE const

#undef ADC_PINS
#define ADC_PINS ADC_PINS_R
#ifdef MUX_PINS
#undef MUX_PINS
#define MUX_PINS MUX_PINS_R
#endif
#undef MUX_CHANNELS
#define MUX_CHANNELS MUX_CHANNELS_R
#ifdef POWER_PINS
#undef POWER_PINS
#define POWER_PINS POWER_PINS_R
#endif
#ifdef MATRIX_POWER_PIN_R
#undef MATRIX_POWER_PIN
#define MATRIX_POWER_PIN MATRIX_POWER_PIN_R
#endif

// Save the left switch num for the calibration sync
#define SWITCH_NUM_L SWITCH_NUM
#undef SWITCH_NUM
#define SWITCH_NUM SWITCH_NUM_R
#undef MUX_TO_NUM
#define MUX_TO_NUM MUX_TO_NUM_R
#undef NUM_TO_MATRIX
#define NUM_TO_MATRIX NUM_TO_MATRIX_R
#undef MATRIX_TO_NUM
#define MATRIX_TO_NUM MATRIX_TO_NUM_R
#undef NUM_TO_MUX
#define NUM_TO_MUX NUM_TO_MUX_R

#if HE_INIT_KEY_NUM_R > 0
#undef HE_INIT_KEY_NUM
#define HE_INIT_KEY_NUM HE_INIT_KEY_NUM_R
#undef HE_INIT_KEYS
#define HE_INIT_KEYS HE_INIT_KEYS_R
#undef HE_INIT_FUNCTIONS
#define HE_INIT_FUNCTIONS HE_INIT_FUNCTIONS_R
#endif

#undef TRIGGER_HEIGHT
#define TRIGGER_HEIGHT TRIGGER_HEIGHT_R
#undef RELEASE_HEIGHT
#define RELEASE_HEIGHT RELEASE_HEIGHT_R
#undef RT_PRESS_DISTANCE
#define RT_PRESS_DISTANCE RT_PRESS_DISTANCE_R
#undef RT_RELEASE_DISTANCE
#define RT_RELEASE_DISTANCE RT_RELEASE_DISTANCE_R

#undef KEY_MODES
#define KEY_MODES KEY_MODES_R
#ifdef HE_TOP_VALUES_R
#undef HE_TOP_VALUES
#define HE_TOP_VALUES HE_TOP_VALUES_R
#endif
#ifdef HE_BOTTOM_VALUES_R
#undef HE_BOTTOM_VALUES
#define HE_BOTTOM_VALUES HE_BOTTOM_VALUES_R
#endif

//TODO: Remove one of the following when I've decided
#ifdef PRIORITY_INDICES
#undef PRIORITY_INDICES
#define PRIORITY_INDICES PRIORITY_INDICES_R
#undef PRIORITY_INDEX_NUM
#define PRIORITY_INDEX_NUM PRIORITY_INDEX_NUM_R
#endif

#ifdef PRIORITY_MUXES
#undef PRIORITY_MUXES
#define PRIORITY_MUXES PRIORITY_MUXES_R
#undef PRIORITY_MUX_NUM
#define PRIORITY_MUX_NUM PRIORITY_MUX_NUM_R
#endif

#else // SPLIT_KEYBOARD defined but side is unknown at init
//TODO: Change the arrays to be const always, only the pointer that is used should be mutable
//      If I only need the pointer when side is unknown, then remove this
#define SPLIT_MUTABLE
#endif

#ifdef SPLIT_KEYBOARD
#if defined RPC_S2M_BUFFER_SIZE
#   warning "S2M buffer size needs to be at least equal to the number of switches on slave side * ADC resolution (10 bits by default)!"
#endif

#undef RPC_S2M_BUFFER_SIZE
// 32 bits per switch needed
#define RPC_S2M_BUFFER_SIZE 32 * MAX(SWITCH_NUM, SWITCH_NUM_R)
// #define DATA_BUFFER_LEN CEILING(MAX(SWITCH_NUM, SWITCH_NUM_R), 3)
#endif // defined SPLIT_KEYBOARD

#ifndef SWITCH_NUM_R
#   define SWITCH_NUM_R 0
#endif
#ifndef SWITCH_NUM_L
#   define SWITCH_NUM_L 0
#endif

#define DEFAULT_PROFILE 0
#define LAST_PROFILE 1

typedef enum rapid_trigger_t: uint8_t {
    none = 0,
    rapid_trigger,
    continuous_rapid_trigger,
    constant_rapid_trigger
} key_mode_t;

