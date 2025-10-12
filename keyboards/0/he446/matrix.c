#include "matrix.h"
#include "he_matrix.h"
#include "he_config.h"
// #include "debounce.h"
#include "atomic_util.h"

matrix_row_t matrix_get_row(uint8_t row) {
    // TODO: return the requested row data

}

void matrix_print(void) {
    // TODO: use print() to dump the current matrix state to console
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
