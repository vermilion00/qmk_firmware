#include <stdint.h>
#include "action.h"
#include "analog_matrix.h"
#ifdef JOYSTICK_ENABLE
#   include "quantum.h"
#endif

//TODO: Just put these in analog_joystick instead
bool process_analog_matrix(uint16_t keycode, keyrecord_t *record);

#ifdef JOYSTICK_ENABLE
bool process_analog_joystick(uint16_t keycode);
// bool process_analog_joystick(keyrecord_t* record);
#endif

void print_calibration_data(void);

