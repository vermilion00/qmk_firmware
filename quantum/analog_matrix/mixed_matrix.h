#include "analog_matrix.h"

void mixed_matrix_init(void);
uint8_t mixed_matrix_scan(void);

#ifndef MATRIX_INPUT_PRESSED_STATE
#    define MATRIX_INPUT_PRESSED_STATE 0
#endif

#if (COL_PIN_NUM <= 8)
typedef uint8_t mech_row_t;
#elif (COL_PIN_NUM <= 16)
typedef uint16_t mech_row_t;
#elif (COL_PIN_NUM <= 32)
typedef uint32_t mech_row_t;
#else
#    error "COL_PIN_NUM: invalid value"
#endif
