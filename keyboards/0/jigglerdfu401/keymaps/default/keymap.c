#include QMK_KEYBOARD_H

bool jiggle_active = true;
bool jiggle_dir = false;
uint16_t jiggle_freq = 15000; //15 sec
uint16_t jiggle_timer = 0;



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
				tap_code(KC_MS_LEFT);
			}else{
				tap_code(KC_MS_RIGHT);
			}
			jiggle_dir = !jiggle_dir;
		}
	}
};




