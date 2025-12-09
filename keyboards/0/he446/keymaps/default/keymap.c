#include "keycodes.h"
#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
     QK_GESC, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,
     KC_TAB,  KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,
     KC_BSPC, KC_A,    KC_R,    KC_S,    KC_T,    KC_G,
     KC_LGUI, KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,
                       KC_LBRC, TG(1),
										 KC_MPLY, KC_SPC,  KC_1,
												  KC_LCTL, KC_LALT
  ),
  [1] = LAYOUT(
     QK_GESC, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,
     KC_TAB,  KC_Y,    KC_U,    KC_L,    KC_J,    KC_SCLN,
     KC_BSPC, KC_A,    KC_R,    KC_S,    KC_T,    KC_G,
     KC_LGUI, KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,
                       KC_LBRC, TG(1),
										 KC_MPLY, KC_SPC,  KC_1,
												  KC_LCTL, KC_LALT
  )
};

// const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {{{ KC_A } }};

// const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
//     [0] = LAYOUT(
//         KC_A, KC_B, KC_C, KC_D,
//         KC_E, KC_F, KC_G, KC_H
//     )
// };

// const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
//     [0] = LAYOUT(
//         KC_A, KC_B, KC_C, KC_D
//     )
// };

void keyboard_post_init_user(void) {
    debug_enable = true;
    debug_matrix = true;
    debug_keyboard = true;
}
