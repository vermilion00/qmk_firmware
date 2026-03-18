#include "analog_matrix.h"

void mixed_matrix_init(void);
uint8_t mixed_matrix_scan(void);

#ifndef MATRIX_INPUT_PRESSED_STATE
#    define MATRIX_INPUT_PRESSED_STATE 0
#endif
