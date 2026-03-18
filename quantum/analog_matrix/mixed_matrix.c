#include "mixed_matrix.h"
#include "analog_matrix.h"
#include "matrix.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "debounce.h"
#include "atomic_util.h"

#include "print.h"

#ifdef SPLIT_KEYBOARD
#    include "split_common/split_util.h"
#    include "split_common/transactions.h"
//TODO: Currently, there's no point in using ROW_PIN_NUM, as we never use the actual row count of the mech matrix
//      The distinction is important in normal QMK because row_pins != MATRIX_ROWS, but here we don't use the actual amount of matrix rows
//TODO: Go through and check if it's true, if yes then use row_pin_num everywhere
// #   define SPLIT_ROW_PIN_NUM (ROW_PIN_NUM / 2)
#   define SPLIT_ROW_PIN_NUM ROW_PIN_NUM
#else
#   define SPLIT_ROW_PIN_NUM ROW_PIN_NUM
#endif

#ifndef DIODE_DIRECTION
#   define DIODE_DIRECTION COL2ROW
#endif

mech_row_t mech_matrix[ROW_PIN_NUM] = {0};
#if ANALOG_DEBOUNCE > 0
#   define MECH_MATRIX(row) mech_matrix[row]
#else
mech_row_t raw_mech_matrix[ROW_PIN_NUM] = {0};
#   define MECH_MATRIX(row) raw_mech_matrix[row]
#endif

//TODO: Add redefines for -s r
#ifdef AM_DIRECT_PINS
static SPLIT_MUTABLE pin_t direct_pins[DIRECT_PIN_NUM] = AM_DIRECT_PINS;
#elif (DIODE_DIRECTION == ROW2COL) || (DIODE_DIRECTION == COL2ROW)
static SPLIT_MUTABLE pin_t row_pins[ROW_PIN_NUM] = AM_ROW_PINS;
static SPLIT_MUTABLE pin_t col_pins[COL_PIN_NUM] = AM_COL_PINS;
#endif

SPLIT_MUTABLE uint8_t rc_to_matrix[ROW_PIN_NUM][COL_PIN_NUM][2] = RC_TO_MATRIX;

static inline uint8_t readMatrixPin(pin_t pin) {
    if (pin != NO_PIN) {
        return (gpio_read_pin(pin) == MATRIX_INPUT_PRESSED_STATE) ? 0 : 1;
    } else {
        return 1;
    }
}
static inline void gpio_atomic_set_pin_output_low(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_set_pin_output(pin);
        gpio_write_pin_low(pin);
    }
}
static inline void gpio_atomic_set_pin_output_high(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_set_pin_output(pin);
        gpio_write_pin_high(pin);
    }
}
static inline void gpio_atomic_set_pin_input_high(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_set_pin_input_high(pin);
    }
}

// Modified matrix code optimized for mixed matrix

#ifdef AM_DIRECT_PINS
__attribute__((weak)) void matrix_init_pins(void) {
    for (int col = 0; col < COL_PIN_NUM; col++) {
        pin_t pin = direct_pins[col];
        if (pin != NO_PIN) {
            gpio_set_pin_input_high(pin);
        }
    }
}

__attribute__((weak)) void matrix_read_cols_on_row(mech_row_t current_matrix[], uint8_t current_row) {
    // Start with a clear matrix row
    mech_row_t current_row_value = 0;

    mech_row_t row_shifter = (mech_row_t)1;
    for (uint8_t col_index = 0; col_index < COL_PIN_NUM; col_index++, row_shifter <<= 1) {
        pin_t pin = direct_pins[col_index];
        current_row_value |= readMatrixPin(pin) ? 0 : row_shifter;
    }

    // Update the matrix
    current_matrix[current_row] = current_row_value;
}

#elif defined(DIODE_DIRECTION)
#    if defined(AM_ROW_PINS) && defined(AM_COL_PINS)
#        if (DIODE_DIRECTION == COL2ROW)

static bool select_row(uint8_t row) {
    pin_t pin = row_pins[row];
    if (pin != NO_PIN) {
        gpio_atomic_set_pin_output_low(pin);
        return true;
    }
    return false;
}

static void unselect_row(uint8_t row) {
    pin_t pin = row_pins[row];
    if (pin != NO_PIN) {
#            ifdef MATRIX_UNSELECT_DRIVE_HIGH
        gpio_atomic_set_pin_output_high(pin);
#            else
        gpio_atomic_set_pin_input_high(pin);
#            endif
    }
}

static void unselect_rows(void) {
    for (uint8_t x = 0; x < ROW_PIN_NUM; x++) {
        unselect_row(x);
    }
}

__attribute__((weak)) void matrix_init_pins(void) {
    unselect_rows();
    for (uint8_t x = 0; x < ROW_PIN_NUM; x++) {
        if (col_pins[x] != NO_PIN) {
            gpio_atomic_set_pin_input_high(col_pins[x]);
        }
    }
}

__attribute__((weak)) void matrix_read_cols_on_row(mech_row_t current_matrix[], uint8_t current_row) {
    // Start with a clear matrix row
    matrix_row_t current_row_value = 0;

    if (!select_row(current_row)) { // Select row
        return;                     // skip NO_PIN row
    }
    matrix_output_select_delay();

    // For each col...
    mech_row_t row_shifter = (mech_row_t)1;
    for (uint8_t col_index = 0; col_index < COL_PIN_NUM; col_index++, row_shifter <<= 1) {
        uint8_t pin_state = readMatrixPin(col_pins[col_index]);

        // Populate the matrix row with the state of the col pin
        current_row_value |= pin_state ? 0 : row_shifter;
    }

    // Unselect row
    unselect_row(current_row);
    matrix_output_unselect_delay(current_row, current_row_value != 0); // wait for all Col signals to go HIGH

    // Update the matrix
    current_matrix[current_row] = current_row_value;
}

#        elif (DIODE_DIRECTION == ROW2COL)
static bool select_col(uint8_t col) {
    pin_t pin = col_pins[col];
    if (pin != NO_PIN) {
        gpio_atomic_set_pin_output_low(pin);
        return true;
    }
    return false;
}

static void unselect_col(uint8_t col) {
    pin_t pin = col_pins[col];
    if (pin != NO_PIN) {
#            ifdef MATRIX_UNSELECT_DRIVE_HIGH
        gpio_atomic_set_pin_output_high(pin);
#            else
        gpio_atomic_set_pin_input_high(pin);
#            endif
    }
}

static void unselect_cols(void) {
    for (uint8_t x = 0; x < MATRIX_COLS; x++) {
        unselect_col(x);
    }
}

__attribute__((weak)) void matrix_init_pins(void) {
    unselect_cols();
    for (uint8_t x = 0; x < ROW_PIN_NUM; x++) {
        if (row_pins[x] != NO_PIN) {
            gpio_atomic_set_pin_input_high(row_pins[x]);
        }
    }
}

__attribute__((weak)) void matrix_read_rows_on_col(mech_row_t current_matrix[], uint8_t current_col, mech_row_t row_shifter) {
    bool key_pressed = false;

    // Select col
    if (!select_col(current_col)) { // select col
        return;                     // skip NO_PIN col
    }
    matrix_output_select_delay();

    // For each row...
    for (uint8_t row_index = 0; row_index < ROW_PIN_NUM; row_index++) {
        // Check row pin state
        if (readMatrixPin(row_pins[row_index]) == 0) {
            // Pin LO, set col bit
            current_matrix[row_index] |= row_shifter;
            key_pressed = true;
        } else {
            // Pin HI, clear col bit
            current_matrix[row_index] &= ~row_shifter;
        }
    }

    // Unselect col
    unselect_col(current_col);
    matrix_output_unselect_delay(current_col, key_pressed); // wait for all Row signals to go HIGH
}

#        else
#            error DIODE_DIRECTION must be one of COL2ROW or ROW2COL!
#        endif
#    endif // defined(AM_ROW_PINS) && defined(AM_COL_PINS)
#else
#    error DIODE_DIRECTION is not defined!
#endif

//MARK: Matrix init
void mixed_matrix_init(void) {
#if defined SPLIT_KEYBOARD && KEYBOARD_SIDE == UNKNOWN
    // Set pinout for right half if pinout for that half is defined
    //TODO: No need for ROW_PIN_NUM_R etc, because QMK already assumes equal pin amounts per half
    if (!is_keyboard_left()) {
        const uint8_t rc_to_matrix_r[ROW_PIN_NUM][COL_PIN_NUM][2] = RC_TO_MATRIX_R;
        memcpy(&rc_to_matrix, &rc_to_matrix_r, sizeof(rc_to_matrix));
#    ifdef DIRECT_PINS_RIGHT
        const pin_t direct_pins_right[DIRECT_PIN_NUM] = DIRECT_PINS_RIGHT;
        memcpy(&direct_pins, &direct_pins_right, sizeof(direct_pins));
#    endif
#    ifdef AM_ROW_PINS_R
        const pin_t row_pins_right[ROW_PIN_NUM] = AM_ROW_PINS_R;
        // for (uint8_t i = 0; i < ROW_PIN_NUM; i++) {
        //     row_pins[i] = row_pins_right[i];
        // }
        //TODO: Isn't memcpy faster?
        memcpy(&row_pins, &row_pins_right, sizeof(row_pins));
#    endif
#    ifdef AM_COL_PINS_R
        const pin_t col_pins_right[AM_COL_PINS] = AC_COL_PINS_R;
        // for (uint8_t i = 0; i < AM_COL_PINS; i++) {
        //     col_pins[i] = col_pins_right[i];
        // }
        memcpy(&col_pins, &col_pins_right, sizeof(col_pins));
#    endif
    }

    // thisMechHand = isLeftHand ? 0 : (ROW_PIN_NUM);
    // thatMechHand = ROW_PIN_NUM - thisMechHand;
#endif // ifdef SPLIT_KEYBOARD

    // initialize key pins
    matrix_init_pins();

    // initialize matrix state: all keys off
    memset(mech_matrix, 0, sizeof(mech_matrix));
    memset(raw_mech_matrix, 0, sizeof(raw_mech_matrix));

    debounce_init(ROW_PIN_NUM);
}

//MARK: Matrix scan
uint8_t mixed_matrix_scan(void) {
    mech_row_t curr_mech_matrix[ROW_PIN_NUM] = {0};

#if defined(DIRECT_PINS) || (DIODE_DIRECTION == COL2ROW)
    // Set row, read cols
    for (uint8_t row = 0; row < ROW_PIN_NUM; row++) {
        matrix_read_cols_on_row(curr_mech_matrix, row);
        // uint8_t state = curr_mech_matrix[0];
        // printf("%u\n", state);
    }
#elif (DIODE_DIRECTION == ROW2COL)
    // Set col, read rows
    mech_row_t row_shifter = (mech_row_t)1;
    for (uint8_t col = 0; col < COL_PIN_NUM; col++, row_shifter <<= 1) {
        matrix_read_rows_on_col(curr_matrix, col, row_shifter);
    }
#endif

    // MECH_MATRIX becomes mech_matrix if no debouncing is used, and raw_mech_matrix if it is used, to avoid using superflous arrays
    bool changed = memcmp(MECH_MATRIX(0), curr_mech_matrix, sizeof(curr_mech_matrix)) != 0;
    if (changed) memcpy(MECH_MATRIX(0), curr_mech_matrix, sizeof(curr_mech_matrix));

    // Debounce only the smaller mech matrix. If the full matrix is debounced, skip this step
    #if ANALOG_DEBOUNCE == 0
    changed = debounce(raw_mech_matrix, mech_matrix, ROW_PIN_NUM, changed);
    #endif

    // No need to run conversions if nothing has changed
    if (!changed) return false;

    // Convert the mech matrix position to the actual matrix position
    for(uint8_t row = 0; row < ROW_PIN_NUM; row++) {
        for(uint8_t col = 0; col < COL_PIN_NUM; col++) {
            const uint8_t matrix_row = rc_to_matrix[row][col][0];
            if(matrix_row == 255) continue;

            const uint8_t matrix_col = rc_to_matrix[row][col][1];
            // Get the state of the mechanical matrix position
            const mech_row_t state = (mech_matrix[row] >> col) & (mech_row_t)1;
            // Clear the mech key position
            matrix_row_t row_state = matrix[matrix_row] & ~(1 << matrix_col);
            // Set the new state of that key
            matrix[matrix_row] = row_state | (state << matrix_col);
        }
    }

    return true;
}
