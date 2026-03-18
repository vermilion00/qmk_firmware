#include QMK_KEYBOARD_H
// #include "joystick_aliases.h"
#include "action.h"
#include "action_layer.h"
#include "analog_matrix.h"
// #include "joystick.h"
#include "keycodes.h"
// #include "modules/getreuer/socd_cleaner/socd_cleaner.h"
#include "quantum.h"
#include "quantum_keycodes.h"
// #include "pointing_device.h"

#define MOUSE_LAYER_TIME 400
#define DRAGSCROLL_TIME 200
// #define constrain_hid(amt) ((amt) < -127 ? -127 : ((amt) > 127 ? 127 : (amt))) test

#define _CLMK 0
#define _GMCL 1
#define _GMQW 2
#define _GMPD 3
#define _MOUSE 4
#define _FN 5
#define _LALT 6
#define _FN2 7

#define CARET_VAL 50
#define SCROLL_VAL 40

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_CLMK] = LAYOUT(
     KC_1,    KC_2,    KC_3,    KC_4, KC_5, KC_6, KC_7
  )
};

// void keyboard_post_init_user(void) {
//     // rgblight_disable_noeeprom(); // Enables RGB, without saving settings
//     // rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);

//     // debug_enable = true;
//     // debug_matrix = true;
//     // debug_keyboard = true;
//     // debug_mouse = true;
// }




