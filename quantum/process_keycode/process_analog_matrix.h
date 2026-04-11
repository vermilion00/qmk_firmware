#include <stdint.h>
#include "action.h"
#include "analog_matrix.h"
#ifdef JOYSTICK_ENABLE
#   include "quantum.h"
#endif

//TODO: Just put these in analog_joystick instead
bool process_analog_matrix(uint16_t keycode, keyrecord_t *record);

void print_calibration_data(void);

