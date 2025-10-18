#include "matrix.h"
#include <stdint.h>
#include <stdio.h>
#include "he_matrix.h"
// #include "debounce.h"
#include "atomic_util.h"
#include "print.h"

#if COLUMNS <= 8
uint8_t column_data = 0;
#elif COLUMNS <= 16
uint16_t column_data = 0;
#elif COLUMNS <= 32
uint32_t column_data = 0;
#else
#error "You must have less than 33 columns!"
#endif
uint8_t column = 0;
#if ROWS <= 8
uint8_t row_data = 0;
#elif ROWS <= 16
uint16_t row_data = 0;
#elif ROWS <= 32
uint32_t row_data = 0;
#else
#error "You must have less than 33 rows!"
#endif
uint8_t row = 0;

matrix_row_t matrix_get_row(uint8_t row) {
    row_data = 0;
    for(column = 0; column < COLUMNS; column++){
        row_data |= matrix[row][column].pressed << column;
    }
    return row_data;
}

void matrix_print(void) {
    // TODO: use print() to dump the current matrix state to console
    // Which way is it represented? I'm guessing LTR
    row_data = 0;
    char row_buf[32];
    for(row = 0; row < ROWS; row++){
        for(column = 0; column < COLUMNS; column++){
            row_data |= matrix[row][column].pressed << (ROWS - column);
        }
        print(row_data);
    }
}

void matrix_init(void) {
    // TODO: initialize hardware and global matrix state here
    //Load key matrix struct with calibration and distance data

    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}

uint8_t matrix_scan(void) {
    bool changed = false;

    // TODO: add matrix scanning routine here


    // This *must* be called for correct keyboard behavior
    matrix_scan_kb();

    return changed;
}

__attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }

__attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }

__attribute__((weak)) void matrix_init_user(void) {}

__attribute__((weak)) void matrix_scan_user(void) {}
