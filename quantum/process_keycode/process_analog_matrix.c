#include <stdint.h>
#include "action.h"
#include "info_config.h"
#include "keycodes.h"
#include "keymap_introspection.h"
#include "print.h"
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

        case AM_CALIBRATE_TOP:
            if(record->event.pressed) {
                calibrate_top_value();
            }
            return false;

        case AM_PRINT_CALIBRATION:
            if(record->event.pressed) {
                print_calibration_data();
                _sync_cal();
            }
            return false;

        case AM_LOCK_PROFILE:
            #if AM_PROFILE_NUM > 1
            if(record->event.pressed) {
                manual_profile_lock = !manual_profile_lock;
                break;
            }
            #endif
            return false;

        case AM_TOGGLE_PRIORITY:
            #ifdef USE_PRIORITY_MODE
            if(record->event.pressed) {
                priority_mode = !priority_mode;
                break;
            }
            #endif
            return false;

        case AM_PROFILE_RANGE ... AM_PROFILE_RANGE_MAX:
            #if AM_PROFILE_NUM > 1
            if (record->event.pressed) {
                // Sanity check that the profile exists
                if((keycode & 0x1F) >= AM_PROFILE_NUM) {
                    printf("Profile %u doesn't exist! Remember that profiles are 0-indexed.", active_profile);
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
            #endif
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
//TODO: No need to return anything if used like this
bool process_analog_joystick(uint16_t keycode) {
    switch (keycode) {
        case ANALOG_JOYSTICK_KEYCODE_RANGE:
            // Subtract the first axis keycode to get the axis index
            evaluate_joystick_axis(keycode - AM_JOYSTICK_RANGE);
            return false;
    }
    return false;
}
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
