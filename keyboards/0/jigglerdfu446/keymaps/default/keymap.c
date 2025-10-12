#include QMK_KEYBOARD_H

bool jiggle_active = true;
bool jiggle_dir = false;
uint16_t jiggle_freq = 15000; //15 sec
uint16_t jiggle_timer = 0;

//Pin C13 is connected to the User key, active low, but qmk doesn't pick it up
//Direct Pin problem?

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {{{ KC_A }}};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
		case KC_A:
			if(record->event.pressed){
				jiggle_active = !jiggle_active;
			}
			return false;
	}
	return true;
};

void matrix_scan_user(void){
	if(jiggle_active){
		if(timer_elapsed(jiggle_timer) > jiggle_freq){
			jiggle_timer = timer_read();
			if(jiggle_dir){
				tap_code(MS_LEFT);
			}else{
				tap_code(MS_RGHT);
			}
			jiggle_dir = !jiggle_dir;
		}
	}
};

void keyboard_post_init_user(void) {
    // gpio_set_pin_input_high("C13");

    // debug_enable = true;
    // debug_matrix = true;
    // debug_keyboard = true;
}




