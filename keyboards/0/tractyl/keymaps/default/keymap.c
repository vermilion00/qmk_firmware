#include QMK_KEYBOARD_H
#include "pointing_device.h"

#define constrain_hid(amt) ((amt) < -127 ? -127 : ((amt) > 127 ? 127 : (amt)))

#define _CLMK 0
#define _GMCL 1
#define _GMQW 2
#define _MOUSE 3
#define _FN 4
#define _LALT 5
#define _FN2 6

#define CARET_VAL 50
#define SCROLL_VAL 40

/* ===== PROBLEMS ======
-Using Right side as master doesn't work, no inputs from left side received
-WAIT_FOR_USB or USB_POLLING_INTERVAL need to be commented out before flashing slave half, or else it won't work
-Sensitivity bug -> Variance in distance to sensor most likely
-Fix encoder sometimes activating on keyboard boot - Maybe fixed now?
-Encoders don't work properly: Skip steps, activating the other way, etc
*/

static bool LALT_HELD;
static bool FN_HELD;

static bool mouse_lock = false;
static bool mslk = false;
static bool caret_mode = false;
static bool scrolling_mode = false;
static uint16_t mouse_timer = 0;
static int8_t tempx = 0;
static int8_t tempy = 0;
static bool scroll_prev = false;
static bool caret_prev = false;
static unsigned int dragscroll_timer;

enum custom_keycodes {
	MO_FN = SAFE_RANGE,
	MO_LALT,
	CK_MSOF,
	//CK_SCLN,
	ALT_TAB,
	//MC_CBR,
	//MC_BRC,
	//MC_PRN,
	MC_COPY,
	MC_PASTE,
	MC_CUT,
	CK_DSCL,
	CK_CRET,
	CK_MSLK,
	KC_SS,
	ENCODER_RIGHT_CW,
	ENCODER_RIGHT_CCW
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_CLMK] = LAYOUT(
     QK_GESC, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                    			 KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
     KC_TAB,  KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,                    			 KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
     KC_BSPC, KC_A,    KC_R,    KC_S,    KC_T,    KC_G,                    			 KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT,
     KC_LGUI, KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,                    			 KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, KC_BSLS,
                       KC_LBRC, KC_RBRC, 								   						  	   KC_PLUS, KC_EQL,
										 KC_MPLY, KC_SPC,  MO_LALT,         MO_FN,   KC_LSFT, ALT_TAB,
												  KC_LCTL, KC_LALT,			KC_ENT
  ),

  [_GMCL] = LAYOUT(
     KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                    			 KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
     KC_TAB,  KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,                    			 KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
     KC_BSPC, KC_A,    KC_R,    KC_S,    KC_T,    KC_G,                    			 KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT,
     KC_DEL,  KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,                    			 KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, MO_FN,
                       KC_LBRC, KC_RBRC, 								   						  	   KC_PLUS, KC_EQL,
										 KC_MPLY, KC_SPC,  KC_LSFT,         KC_BSPC, KC_LSFT, ALT_TAB,
												  KC_LCTL, KC_LALT,			KC_ENT
  ),

  [_GMQW] = LAYOUT(
     KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                    			 KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
     KC_TAB , KC_Q,    KC_Q,    KC_W,    KC_E,    KC_R,                   			 KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
     KC_BSPC, KC_LSFT, KC_A,    KC_S,    KC_D,    KC_G,                    			 KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT,
     KC_EQL,  KC_V,    KC_X,    KC_Y,    KC_C,    KC_Z,                    			 KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, KC_BSLS,
                       KC_LBRC, KC_RBRC,													 	  	   KC_PLUS, KC_EQL,
										 KC_MPLY, KC_SPC,  MO_LALT,			  MO_FN, KC_LSFT, ALT_TAB,
												  KC_LCTL, KC_LALT,			  KC_ENT
  ),

  [_MOUSE] = LAYOUT(
     KC_ESC,  CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,                 			 CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,
     KC_TAB,  CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,                 			 MC_CUT,  CK_DSCL, CK_CRET, CK_MSLK, CK_MSOF, CK_MSOF,
     KC_BSPC, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,                 			 MC_COPY, MS_BTN1, MS_BTN2, MS_BTN3, KC_MPLY, KC_QUOT,
     KC_LGUI, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,                 			 MC_PASTE,MS_BTN4, MS_BTN5, KC_F5,   CK_MSOF, KC_BSLS,
                       CK_MSOF, CK_MSOF,											              	   KC_PLUS, KC_EQL,
										 _______, CK_MSOF, KC_LSFT,			MO_FN,   KC_LSFT, _______,
												  KC_LCTL, KC_LALT,			KC_ENT
  ),

  [_FN] = LAYOUT(
     KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                   			 KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_BSPC,
     KC_TAB,  KC_PGUP, KC_PGUP, KC_UP,   KC_HOME, KC_SCLN,                 		 	 KC_COMM, KC_P7,   KC_P8,   KC_P9,   KC_SCLN, KC_SS,
     KC_DEL,  KC_HOME, KC_LEFT, KC_DOWN, KC_RGHT, KC_AT,              	   			 KC_COMM, KC_P4,   KC_P5,   KC_P6,   KC_P0,   KC_QUOT,
     KC_LGUI, KC_PGDN, KC_PGDN, KC_DOWN, KC_END,  KC_NUBS,             	   			 KC_DOT,  KC_P1,   KC_P2,   KC_P3,   KC_P0,   KC_DOT,
                       DM_PLY1, TG(_GMQW),													           KC_PLUS, KC_EQL,
										 KC_MUTE, KC_SPC,  _______,	   		_______, KC_LSFT, QK_BOOT,
												  KC_LCTL, KC_LALT,	   		KC_ENT
  ),

  [_LALT] = LAYOUT(
     KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                   			 KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_DEL,
     KC_TAB,  KC_PGUP, KC_LBRC,  KC_COLN, KC_RBRC, KC_LPRN,             	   		 KC_COMM, KC_P7,   KC_P8,   KC_P9,   KC_SCLN, KC_SS,
     KC_DEL,  KC_RCBR, KC_LCBR,  KC_UNDS, KC_RCBR, KC_AT,              	   			 KC_QUOT, KC_P4,   KC_P5,   KC_P6,   KC_P0,   KC_QUOT,
     KC_LGUI, KC_PGDN, KC_LPRN,  KC_HASH, KC_RPRN, KC_SCLN,            	   			 KC_DOT,  KC_P1,   KC_P2,   KC_P3,   KC_P0,   KC_BSLS,
                       KC_LABK, KC_RABK,													 	 	   _______, _______,
										 _______, _______, _______,	  	    MO(_FN2),_______, _______,
												  _______, KC_LSFT,	   		_______
  ),

  [_FN2] = LAYOUT(
     QK_BOOT, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                   			 KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_BSPC,
     KC_TAB,  KC_PGUP, KC_PGUP, KC_UP,   KC_HOME, KC_SCLN,            	   			 KC_COMM, KC_P7,   KC_P8,   KC_P9,   KC_SCLN, KC_NUM,
     KC_DEL,  KC_HOME, KC_LEFT, KC_DOWN, KC_RGHT, KC_AT,            	   			 KC_COMM, KC_P4,   KC_P5,   KC_P6,   KC_P0,   KC_F11,
     KC_LGUI, KC_PGDN, KC_PGDN, KC_HASH, SOCDON,  SOCDOFF,                 			 KC_DOT,  KC_P1,   KC_P2,   KC_P3,   KC_P0,   KC_F12,
                       DM_PLY1, TG(_GMQW),												     		   DM_REC1, DM_REC2,
										 KC_MUTE, _______, _______,		    _______, _______, KC_LALT,
												  _______, _______,			_______
  ),
};

socd_cleaner_t socd_opposing_pairs[] = {
	{{KC_R, KC_T}, SOCD_CLEANER_LAST},
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [_CLMK] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT),  ENCODER_CCW_CW(ENCODER_RIGHT_CCW, ENCODER_RIGHT_CW)  },
    [_GMCL] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT),  ENCODER_CCW_CW(ENCODER_RIGHT_CCW, ENCODER_RIGHT_CW)  },
    [_GMQW] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT),  ENCODER_CCW_CW(ENCODER_RIGHT_CCW, ENCODER_RIGHT_CW)  },
    [_MOUSE] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT),  ENCODER_CCW_CW(ENCODER_RIGHT_CCW, ENCODER_RIGHT_CW)  },
    [_FN] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT),  ENCODER_CCW_CW(ENCODER_RIGHT_CCW, ENCODER_RIGHT_CW)  },
    [_LALT] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT),  ENCODER_CCW_CW(ENCODER_RIGHT_CCW, ENCODER_RIGHT_CW)  },
    [_FN2] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT),  ENCODER_CCW_CW(ENCODER_RIGHT_CCW, ENCODER_RIGHT_CW)  },
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
		case MS_BTN1:
			if(record->event.pressed){
				mouse_timer = timer_read();
				scrolling_mode = false;
				caret_mode = false;
				mouse_lock = true;
			}else{
				if(!mslk){
					mouse_lock = false;
				}
			}
			return true;
		case CK_MSOF:
			if(record->event.pressed){
				layer_off(_MOUSE);
				scrolling_mode = false;
				caret_mode = false;
				mouse_lock = false;
				tap_code(KC_TRNS);
			}
			return true;
		case MO_FN:
			if (record->event.pressed) {
				layer_on(_FN);
				FN_HELD = true;
			} else {
				layer_off(_FN);
				FN_HELD = false;
			}
			return false;
		case MO_LALT:
			if (record->event.pressed) {
				layer_on(_LALT);
				FN_HELD = true;
				caret_mode = true;
			} else {
				layer_off(_LALT);
				FN_HELD = false;
				caret_mode = false;
			}
			return false;
		case KC_LALT:
			if (record->event.pressed) {
				LALT_HELD = true;
			} else {
				LALT_HELD = false;
			}
			return true;
		case ALT_TAB:
			if (record->event.pressed) {
					register_code(KC_LALT);
					tap_code(KC_TAB);
					LALT_HELD = true;
			} else {
				unregister_code(KC_LALT);
				LALT_HELD = false;
			}
			return false;
		case MC_COPY:
			if(record->event.pressed){
				register_code(KC_LCTL);
				tap_code(KC_C);
				unregister_code(KC_LCTL);
			}
			return false;
		case MC_PASTE:
			if(record->event.pressed){
				register_code(KC_LCTL);
				tap_code(KC_V);
				unregister_code(KC_LCTL);
			}
			return false;
		case MC_CUT:
			if(record->event.pressed){
				register_code(KC_LCTL);
				tap_code(KC_X);
				unregister_code(KC_LCTL);
			}
			return false;
		case CK_DSCL:
			if(record->event.pressed){
				dragscroll_timer = timer_read();
				scroll_prev = scrolling_mode;
				scrolling_mode = true;
				if(!mslk){
					mouse_lock = !mouse_lock;
				}
			}else{
				if(timer_elapsed(dragscroll_timer) < 200){
					scrolling_mode = !scroll_prev;
				}else{
					if(!mslk){
						mouse_lock = false;
					}
					scrolling_mode = false;
				}
			}
			return false;
		case CK_CRET:
			if(record->event.pressed){
				dragscroll_timer = timer_read();
				caret_prev = caret_mode;
				caret_mode = true;
			}else{
				if(timer_elapsed(dragscroll_timer) < 200){
					caret_mode = !caret_prev;
				}else{
					caret_mode = false;
				}
			}
			return false;
		case CK_MSLK:
			if(record->event.pressed){
				mouse_lock = !mouse_lock;
				mslk = !mslk;
			}
			return false;
		case KC_SS:
			if(record->event.pressed){
				register_code16(KC_LALT);
				tap_code(KC_P0);
				tap_code(KC_P2);
				tap_code(KC_P2);
				tap_code(KC_P3);
				unregister_code16(KC_LALT);
			}
			return false;
		case ENCODER_RIGHT_CCW:
			if(record->event.pressed){
				if(LALT_HELD){
					register_code(KC_LSFT);
					tap_code(KC_TAB);
					unregister_code(KC_LSFT);
				} else {
					tap_code_delay(KC_MPRV, 10);
				}
			}
			return false;
		case ENCODER_RIGHT_CW:
			if(record->event.pressed){
				if(LALT_HELD){
					tap_code(KC_TAB);
				} else {
					tap_code_delay(KC_MNXT, 10);
				}
			}
			return false;
	}
	return true;
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
	if(mouse_report.x != 0 || mouse_report.y != 0){
		if(!layer_state_is(_GMCL)){
			if(!layer_state_is(_MOUSE)){
				layer_on(_MOUSE);
			}
			mouse_timer = timer_read();
		}
	}else if((timer_elapsed(mouse_timer) > 500) && layer_state_is(_MOUSE) && !mouse_lock){
		layer_off(_MOUSE);
	}
	short x = mouse_report.x, y = mouse_report.y;
	x = (x > 0 ? x * x / 3 + x : -x * x / 3 + x);
	y = (y > 0 ? y * y / 3 + y : -y * y / 3 + y);

	mouse_report.x = constrain_hid(x);
	mouse_report.y = constrain_hid(y);
	if(scrolling_mode){
		caret_mode = false;
		tempx += mouse_report.x;
		tempy += mouse_report.y;
		mouse_report.x = 0;
		mouse_report.y = 0;
		if(tempy > SCROLL_VAL){
			tap_code(MS_WHLD);
			tempx = 0;
			tempy = 0;
		}else if(tempy < -SCROLL_VAL){
			tap_code(MS_WHLU);
			tempx = 0;
			tempy = 0;
		}else if(tempx > SCROLL_VAL - 10){
			tap_code(MS_WHLR);
			tempx = 0;
			tempy = 0;
		}else if(tempx < -SCROLL_VAL - 10){
			tap_code(MS_WHLL);
			tempx = 0;
			tempy = 0;
		}
	}else if(caret_mode){
		tempx += mouse_report.x;
		mouse_report.x = 0;
		tempy += mouse_report.y;
		mouse_report.y = 0;
		if(tempx > CARET_VAL){
			tap_code(KC_RGHT);
			tempx = 0;
			tempy = 0;
		}else if(tempx < -CARET_VAL){
			tap_code(KC_LEFT);
			tempx = 0;
			tempy = 0;
		}else if(tempy > CARET_VAL + 10){
			tap_code(KC_DOWN);
			tempx = 0;
			tempy = 0;
		}else if(tempy < -CARET_VAL + 10){
			tap_code(KC_UP);
			tempx = 0;
			tempy = 0;
		}
    }
    return mouse_report;
}

layer_state_t layer_state_set_user(layer_state_t state){
    switch(get_highest_layer(state)){
		case _CLMK:
            if(rgblight_is_enabled()){
				rgblight_disable_noeeprom();
			}
            break;
		case _FN:
			if(!rgblight_is_enabled()){
				rgblight_enable_noeeprom();
			}
			rgblight_sethsv_noeeprom(HSV_RED);
			break;
		case _LALT:
			if(!rgblight_is_enabled()){
				rgblight_enable_noeeprom();
			}
			rgblight_sethsv_noeeprom(HSV_GREEN);
			break;
		case _MOUSE:
			if(!rgblight_is_enabled()){
				rgblight_enable_noeeprom();
			}
			rgblight_sethsv_noeeprom(HSV_TEAL);
			break;
		case _FN2:
			if(!rgblight_is_enabled()){
				rgblight_enable_noeeprom();
			}
			rgblight_sethsv_noeeprom(HSV_PURPLE);
			break;
		case _GMCL:
			if(!rgblight_is_enabled()){
				rgblight_enable_noeeprom();
			}
			rgblight_sethsv_noeeprom(HSV_TURQUOISE);
			break;
        default:
            rgblight_sethsv_noeeprom(HSV_YELLOW);
            break;
    }
    return state;
}

void keyboard_post_init_user(void) {
  rgblight_disable_noeeprom(); // Enables RGB, without saving settings
  rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);

//   debug_enable = true;
//   debug_matrix = true;
//   debug_keyboard = true;
//   debug_mouse = true;
}




