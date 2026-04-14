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
                set_profile_lock_state(!get_profile_lock_state());
            }
            #endif
            return false;

        case AM_TOGGLE_PRIORITY:
            #ifdef USE_PRIORITY_MODE
            if(record->event.pressed) {
                priority_mode = !priority_mode;
            }
            #endif
            return false;

        case AM_CLEAR_CALIBRATION:
            #ifndef AM_NO_EEPROM
            if(record->event.pressed) {
                clear_calibration();
            }
            #endif
            return false;

        case AM_INCREMENT_PROFILE:
            #if AM_PROFILE_NUM > 1
            if(record->event.pressed) {
                if(active_profile + 1 >= am_keyboard_data.profile_num) {
                    active_profile = 0;
                } else active_profile += 1;
            }
            #endif
            return false;

        case AM_DECREMENT_PROFILE:
            #if AM_PROFILE_NUM > 1
            if(record->event.pressed) {
                if(active_profile - 1 <= 0) {
                    active_profile = am_keyboard_data.profile_num - 1;
                } else active_profile -= 1;
            }
            #endif
            return false;

        case AM_PROFILE_RANGE ... AM_PROFILE_RANGE_MAX:
            #if AM_PROFILE_NUM > 1
            if (record->event.pressed) {
                const uint8_t profile = keycode & 0x000F;
                // Sanity check that the profile exists
                if(profile >= (am_keyboard_data.profile_num & 0x0F)) {
                    printf("Profile %u doesn't exist! Remember that profiles are 0-indexed.", active_profile);
                    return false;
                }

                // Turn on manual profile lock if switching to a new profile, turn it off when switching to the currently active profile
                if(profile == active_profile) {
                    set_profile_lock_state(false);
                } else {
                    // 16 profiles max
                    set_active_profile(profile);
                    set_profile_lock_state(true);
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


void print_calibration_data(void) {
    char side[7] = "";
    #ifdef SPLIT_KEYBOARD
    if(!is_keyboard_left()) {
        char right[] = "_right";
        memcpy(&side, right, sizeof(right));
    }
    #endif

    // The deadzones need to be removed from the key_config values
    #ifdef INVERT_ADC
    printf("\"top_values%s\": [ %u", side, key_config[0].top_value - top_deadzones[0]);
    if(switch_num > 1) {
        for(uint8_t index = 1; index < switch_num; index++){
            printf(", %u", key_config[index].top_value - top_deadzones[index]);
        }
    }
    #else
    printf("\"top_values%s\": [ %u", side, key_config[0].top_value + top_deadzones[0]);
    if(switch_num > 1) {
        for(uint8_t index = 1; index < switch_num; index++){
            printf(", %u", key_config[index].top_value + top_deadzones[index]);
        }
    }
    #endif

    #ifndef INVERT_ADC
    uint16_t adjustment = -ADC_BOTTOM_DEADZONE;
    #else
    uint16_t adjustment = ADC_BOTTOM_DEADZONE;
    #endif
    printf(" ],\n\"bottom_values%s\": [ %u", side, key_config[0].bottom_value + adjustment);
    if(switch_num > 1) {
        for(uint8_t index = 1; index < switch_num; index++){
            printf(", %u", key_config[index].bottom_value - adjustment);
        }
    }
    print(" ]\n");
}
