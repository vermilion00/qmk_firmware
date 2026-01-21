#include <stdint.h>
#include "action.h"
#include "keycodes.h"
#include "print.h"
#include "analog_matrix.h"
#include "process_analog_matrix.h"

bool process_analog_matrix(uint16_t keycode, keyrecord_t *record) {
    switch(keycode) {
        case AM_CALIBRATE:
            if(record->event.pressed) {
                calibrate_switches();
                for(uint8_t index = 0; index < switch_num; index++) {
                    key_config[index].pressed = false;
                }
                break;
            }
            return false;
        case AM_PRINT_CALIBRATION:
            if(record->event.pressed) {
                print_calibration_data();
                break;
            }
            return false;
        case AM_PRINT_PROFILE:
            if(record->event.pressed) {
                printf("\n%u\n", active_profile);
                break;
            }
            return false;
        case QK_AM_PROFILE ... QK_AM_PROFILE_MAX:
            if (record->event.pressed) {
                // 32 profiles max
                set_active_profile(keycode&0x1F);
                break;
            }
            return false;
    }
    return true;
}


// This currently only prints the data for the master side, plug in the other half to print its data
//TODO: Send slave data over and print it as well
void print_calibration_data(void) {
    char side[7] = "";
    #ifdef SPLIT_KEYBOARD
    if(!is_keyboard_left()) {
        char right[] = "_right";
        memcpy(&side, right, sizeof(right));
    }
    #endif

    printf("\"top_values%s\": [ %u", side, key_config[0].top_value);
    if(switch_num > 1) {
        for(uint8_t index = 1; index < switch_num; index++){
            printf(", %u", key_config[index].top_value);
        }
    }

    printf(" ],\n\"bottom_values%s\": [ %u", side, key_config[0].bottom_value);
    if(switch_num > 1) {
        for(uint8_t index = 1; index < switch_num; index++){
            printf(", %u", key_config[index].bottom_value);
        }
    }
    print(" ]\n");
}
