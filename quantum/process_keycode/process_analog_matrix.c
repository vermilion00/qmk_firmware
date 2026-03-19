#include <stdint.h>
#include "action.h"
#include "info_config.h"
#include "joystick.h"
#include "keycodes.h"
#include "keymap_introspection.h"
#include "print.h"
#include "debug.h"
#include "analog_matrix.h"
#include "matrix.h"
#include "process_analog_matrix.h"
#ifdef JOYSTICK_ENABLE
#   include "analog_joystick.h"
#endif

//MARK: AM Process
bool process_analog_matrix(uint16_t keycode, keyrecord_t *record) {
    switch(keycode) {
        case AM_CALIBRATE:
            if(record->event.pressed) {
                calibrate_switches(false);
            }
            return false;

        case AM_PRINT_CALIBRATION:
            if(record->event.pressed) {
                print_calibration_data();
                _sync_cal();
            }
            return false;

        case AM_LOCK_PROFILE:
            if(record->event.pressed) {
                manual_profile_lock = !manual_profile_lock;
                break;
            }
            return false;

        case AM_TOGGLE_PRIORITY:
            #ifdef USE_PRIORITY_MODE
            if(record->event.pressed) {
                priority_mode = !priority_mode;
                break;
            }
            #endif
            return false;

        case ANALOG_MATRIX_PROFILE_RANGE:
            if (record->event.pressed) {
                // Sanity check that the profile exists
                if((keycode & 0x1F) >= AM_PROFILE_NUM) {
                    dprintf("Profile %u doesn't exist! Remember that profiles are 0-indexed.", active_profile);
                    return false;
                }

                // Turn on manual profile lock if switching to a new profile, turn it off when switching to the currently active profile
                if((keycode & 0x1F) == active_profile) {
                    manual_profile_lock = false;
                } else {
                    // 32 profiles max
                    set_active_profile(keycode&0x1F);
                    manual_profile_lock = true;
                }
            }
            return false;
    }
    return true;
}

#ifdef SPLIT_KEYBOARD
extern uint8_t thisHand;
#else
const uint8_t thisHand = 0;
#endif

//MARK: Joystick
#ifdef JOYSTICK_ENABLE
bool process_analog_joystick(uint16_t keycode) {
    //TODO: Check if it's faster to check for the keycodes
    if(!joystick_state.dirty || !joystick_layer) return true;

    switch (keycode) {
        case JOYSTICK_AXIS_RANGE:
            // LED_ON;
            // const uint8_t index = matrix_to_num[record->event.key.row - thisHand][record->event.key.col] - 1;
            // Subtract the first axis keycode to get the axis index
            #ifndef USE_JOYSTICK
            // update_joystick_value(keycode - JS_LEFT_POSITIVE_X, key_config[index].joystick_value);
            evaluate_joystick_axis(keycode - QK_AM_JOYSTICK_AXIS);
            #else
            update_joystick_value(keycode - JS_LEFT_POSITIVE_X, key_config[index].trigger_value[active_profile]);
            #endif
            return false;
    }
    return true;
}

// bool process_analog_joystick(keyrecord_t* record) {
//     if(!joystick_state.dirty) return false;
//     // LED_ON;

//     uint16_t keycode = get_record_keycode(record, true);

//     switch (keycode) {
//         case JOYSTICK_AXIS_RANGE:
//             const uint8_t index = matrix_to_num[record->event.key.row - thisHand][record->event.key.col] - 1;
//             // Subtract the first axis keycode to get the axis index
//             #ifndef USE_JOYSTICK
//             update_joystick_value(keycode - JS_LEFT_POSITIVE_X, key_config[index].joystick_value);
//             #else
//             update_joystick_value(keycode - JS_LEFT_POSITIVE_X, key_config[index].trigger_value[active_profile]);
//             #endif
//             return false;
//     }
//     return false;
// }
#endif


void print_calibration_data(void) {
    char side[7] = "";
    #ifdef SPLIT_KEYBOARD
    if(!is_keyboard_left()) {
        char right[] = "_right";
        memcpy(&side, right, sizeof(right));
    }
    #endif

    // The deadzone needs to be removed from the key_config values
    #ifndef INVERT_ADC
    int16_t adjustment = ADC_TOP_DEADZONE;
    #else
    int16_t adjustment = -ADC_TOP_DEADZONE;
    #endif
    printf("\"top_values%s\": [ %u", side, key_config[0].top_value + adjustment);
    if(switch_num > 1) {
        for(uint8_t index = 1; index < switch_num; index++){
            printf(", %u", key_config[index].top_value + adjustment);
        }
    }

    #ifndef INVERT_ADC
    adjustment = ADC_BOTTOM_DEADZONE;
    #else
    adjustment = -ADC_BOTTOM_DEADZONE;
    #endif
    printf(" ],\n\"bottom_values%s\": [ %u", side, key_config[0].bottom_value - adjustment);
    if(switch_num > 1) {
        for(uint8_t index = 1; index < switch_num; index++){
            printf(", %u", key_config[index].bottom_value - adjustment);
        }
    }
    print(" ]\n");
}
