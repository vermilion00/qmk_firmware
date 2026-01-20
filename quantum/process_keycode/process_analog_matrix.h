#include <stdint.h>
#include "action.h"
#include "analog_matrix.h"

bool process_analog_matrix(uint16_t keycode, keyrecord_t *record);

void print_calibration_data(void);

uint8_t get_active_profile(void);

void set_active_profile(uint8_t profile);

