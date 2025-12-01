#include "keycodes.h"
#include QMK_KEYBOARD_H

// const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {{{ KC_A, KC_B } }};
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_A, KC_B,
        KC_C, KC_D
    )
};

void keyboard_post_init_user(void) {
    // gpio_set_pin_input_high("C13");

    debug_enable = true;
    debug_matrix = true;
    debug_keyboard = true;
}
