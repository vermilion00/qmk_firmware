#include QMK_KEYBOARD_H
#include "color.h"
#include "info_config.h"
#include "joystick_aliases.h"
#include "keymap_us.h"
#include "action.h"
#include "action_layer.h"
#include "keycodes.h"
#include "modules/getreuer/socd_cleaner/socd_cleaner.h"
#include "modules/getreuer/select_word/select_word.h"
#include "quantum.h"
#include "quantum_keycodes.h"
#include "pointing_device.h"

#define MOUSE_LAYER_TIME 200
#define DRAGSCROLL_TIME 160

#define _CLMK 0
#define _GAME 1
// #define _GMQW 2
#define _GMPD 2
#define _MOUSE 3
#define _FN 4
#define _LALT 5
#define _FN2 6

#define CARET_VAL 50
#define SCROLL_VAL 30

static bool mouse_lock = false;
static bool mslk = false;
static bool caret_mode = false;
static bool scrolling_mode = false;
static uint32_t mouse_timer = 0;
static int8_t tempx = 0;
static int8_t tempy = 0;
static bool scroll_prev = false;
static bool caret_prev = false;
static uint32_t dragscroll_timer;
static bool game_layer = false;
static bool lalt_held = false;

enum custom_keycodes {
	MO_LALT = SAFE_RANGE,
	CK_MSOF,
	ALT_TAB,
    LCTAB,
	CK_DSCL,
	CK_CRET,
	CK_MSLK,
	KC_SS
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_CLMK] = LAYOUT(
     QK_GESC, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                    			          KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
     KC_TAB,  KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,                    			          KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
     KC_BSPC, KC_A,    KC_R,    KC_S,    KC_T,    KC_G,                    			          KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT,
     KC_LGUI, KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,                    			          KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, KC_BSLS,
                               KC_LABK, KC_RABK, 								   						  	                            KC_PLUS, KC_EQL,
										                 ALT_TAB, KC_SPC,  MO_LALT,         MO(_FN),  KC_LSFT, KC_MPLY,
												                      KC_LCTL, KC_LALT,			KC_ENT
  ),

// Default
  [_GAME] = LAYOUT(
     KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                    			          KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
     KC_TAB,  KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,                    			          KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
     KC_W,    KC_A,    KC_R,    KC_S,    KC_T,    KC_G,                    			          KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT,
     KC_E,    KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,                    			          KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, MO(_FN),
                               KC_LBRC, KC_RBRC, 								   						  	                            KC_MPRV, KC_MNXT,
						                                 KC_P,    KC_SPC,  MO(_LALT),        KC_BSPC, KC_LSFT, KC_MPLY,
									                                  KC_LCTL, KC_LALT,			 KC_ENT
  ),

//   [_GMQW] = LAYOUT(
//      KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                    			          KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
//      KC_TAB , KC_Q,    KC_Q,    KC_W,    KC_E,    KC_R,                   			          KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
//      KC_BSPC, KC_LSFT, KC_A,    KC_S,    KC_D,    KC_G,                    			          KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT,
//      KC_EQL,  KC_V,    KC_X,    KC_Y,    KC_C,    KC_Z,                    			          KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, TG(_GMQW),
//                                KC_LBRC, KC_RBRC,													 	  	                            KC_PLUS, KC_EQL,
// 										                 KC_O,    KC_SPC,  MO_LALT,			 MO(_FN), KC_LSFT, ALT_TAB,
// 												                      KC_LCTL, KC_LALT,			 KC_ENT
//   ),

  [_GMPD] = LAYOUT(
     KC_ESC,  JS_RNZ,  JS_RPZ,  JS_RPY,  JS_RNY,  KC_5,                    			          KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
    TG(_GMPD),KC_Q,    JS_LT,   JS_LPY,  JS_RT,   KC_B,                    			          KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
     KC_BSPC, JS_RPX,  JS_LNX,  JS_LNY,  JS_LPX,  KC_G,                    			          KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    DB_TOGG,
     JS_RNX,  KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,                    			          KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, _______,
                               KC_LBRC, KC_RBRC, 								   						  	                            KC_MPRV, KC_MNXT,
						                                 TG(_GMPD),KC_SPC, MO(_LALT),        KC_BSPC, KC_LSFT, KC_MPLY,
									                                  KC_LCTL, KC_LALT,			 KC_ENT
  ),

//   [_GMPD] = LAYOUT(
//      KC_ESC,  JS_RNZ,    JS_RPZ,  KC_3,    JS_RPY,  KC_5,                    			          KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
//      KC_TAB,  KC_Q,    JS_LT,   JS_LPY,  JS_RT,   KC_B,                    			          KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
//      JS_RPX,  KC_A,    JS_LNX,  JS_LNY,  JS_LPX,  KC_G,                    			          KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT,
//      JS_RNX,  KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,                    			          KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, MO(_FN),
//                                KC_LBRC, KC_RBRC, 								   						  	                            KC_MPRV, KC_MNXT,
// 						                                 KC_P,    KC_SPC,  MO(_LALT),        KC_BSPC, KC_LSFT, KC_MPLY,
// 									                                  KC_LCTL, KC_LALT,			 KC_ENT
//   ),

//   [_GMPD] = LAYOUT(
//      JS_0,  JS_1,    JS_2,    JS_3,    JS_4,    JS_5,                    			          JS_10,    JS_11,    JS_12,    JS_13,    KC_0,    KC_BSPC,
//      KC_TAB,  KC_Q,    JS_LT,    JS_LPY,  JS_RT,    KC_B,                    			          KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_MINS,
//      KC_W,    KC_A,    JS_LNX,    JS_LNY,    JS_LPX,    KC_G,                    			          KC_K,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT,
//      KC_E,    KC_V,    JS_RNX,    KC_D,    JS_RPX,    KC_Z,                    			          KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, MO(_FN),
//                                KC_LBRC, KC_RBRC, 								   						  	                            KC_MPRV, KC_MNXT,
// 						                                 KC_P,    KC_SPC,  MO(_LALT),        KC_BSPC, KC_LSFT, KC_MPLY,
// 									                                  KC_LCTL, KC_LALT,			 KC_ENT
//   ),

//   Mixed gamepad matrix
//   [_GMPD] = LAYOUT(
//      JS_0,  JS_1,    JS_2,    JS_3,    JS_4,    JS_5,                    			          JS_10,    JS_11,    JS_12,    JS_13,    JS_14,    JS_15,
//      KC_TAB,  KC_Q,    JS_LT,    JS_LPY,    JS_RT,    KC_B,                    			          JS_6,    JS_7,    JS_RPY,    JS_9,    KC_SCLN, KC_MINS,
//      KC_W,    KC_A,    JS_LNX,    JS_LNY,    JS_LPX,    KC_G,                    			          KC_K,    JS_RNX,    JS_RNY,    JS_RPX,    KC_O,    KC_QUOT,
//      KC_E,    KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,                    			          KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, MO(_FN),
//                                KC_LBRC, KC_RBRC, 								   						  	                            KC_MPRV, KC_MNXT,
// 						                                 KC_P,    KC_SPC,  MO(_LALT),        KC_BSPC, KC_LSFT, KC_MPLY,
// 									                                  KC_LCTL, KC_LALT,			 KC_ENT
//   ),

  // All left half
//   [_GMPD] = LAYOUT(
//      JS_0,    JS_1,    JS_2,    JS_3,    JS_4,    JS_5,                    			          JS_10,    JS_11,    JS_12,    JS_13,    JS_14,    JS_15,
//      KC_TAB,  KC_Q,    JS_LT,    JS_LPY,    JS_RT,    KC_B,                    			          JS_6,    JS_7,    JS_8,    JS_9,    KC_SCLN, KC_MINS,
//      JS_RPZ,  KC_A,    JS_LNX,    JS_LNY,    JS_LPX,    KC_G,                    			          JS_16,    JS_17,    JS_18,    KC_NO,    KC_O,    KC_QUOT,
//      JS_RNZ,  KC_V,    JS_RNX,    JS_RPY,    JS_RPX,    JS_RNY,                    			          KC_M,    KC_H,    KC_COMM, KC_DOT,  KC_SLSH, MO(_FN),
//                                KC_LBRC, KC_RBRC, 								   						  	                            KC_MPRV, KC_MNXT,
// 						                                 KC_P,    KC_SPC,  MO(_LALT),        KC_BSPC, KC_LSFT, KC_MPLY,
// 									                                  KC_LCTL, KC_LALT,			 KC_ENT
//   ),

// All right half
//   [_GMPD] = LAYOUT(
//      KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                    			          JS_0,    JS_1,    JS_2,    JS_3,    JS_4,    JS_5,
//      KC_TAB,  KC_Q,    JS_9,    JS_Y,    JS_11,   JS_12,                    			      JS_6,    JS_LT,   JS_RPY,  JS_RT,   KC_SCLN, KC_MINS,
//      KC_W,    KC_A,    JS_X,    JS_A,    JS_B,    KC_G,                    			          JS_7,    JS_RNX,  JS_RNY,  JS_RPX,  KC_O,    KC_QUOT,
//      KC_E,    KC_V,    KC_X,    KC_D,    KC_C,    KC_Z,                    			          JS_8,    JS_LNX,  JS_LPY,  JS_LPX,  JS_LNY, MO(_FN),
//                                KC_LBRC, KC_RBRC, 								   						  	                            KC_MPRV, KC_MNXT,
// 						                                 KC_P,    KC_SPC,  MO(_LALT),        KC_BSPC, KC_LSFT, KC_MPLY,
// 									                                  KC_LCTL, KC_LALT,			 KC_ENT
//   ),

  [_MOUSE] = LAYOUT(
     KC_ESC,  CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,                 			          CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,
     KC_TAB,  CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,                 			          CK_MSOF, CK_DSCL, CK_CRET, CK_MSLK, CK_MSOF, CK_MSOF,
     KC_BSPC, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,                 			          CK_MSOF, MS_BTN1, MS_BTN2, MS_BTN3, KC_MPLY, KC_QUOT,
     KC_LGUI, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF, CK_MSOF,                 			          CK_MSOF, MS_BTN4, MS_BTN5, KC_F5,   CK_MSOF, KC_BSLS,
                               CK_MSOF, CK_MSOF,											              	                            KC_PLUS, KC_EQL,
										                 _______, CK_MSOF, MO(_LALT),	    MO(_FN),  KC_LSFT, _______,
												                      CK_MSOF, KC_LSFT,			KC_ENT
  ),

  [_FN] = LAYOUT(
     KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                   			          KC_F6,   KC_F7,   KC_F8,   KC_F9,  KC_F10,  KC_BSPC,
     KC_TAB,  KC_PGUP, KC_UP,   KC_UP,   KC_HOME, KC_SCLN,                 		 	          KC_COMM, KC_7,    KC_8,    KC_9,   KC_SCLN, KC_SS,
     KC_DEL,  KC_HOME, KC_LEFT, KC_DOWN, KC_RGHT, KC_AT,              	   			          KC_COMM, KC_4,    KC_5,    KC_6,   KC_0,    KC_GRV,
     TG(1),   KC_PGDN, SELWBAK, SELWORD, KC_END,  KC_NUBS,             	   			          KC_DOT,  KC_1,    KC_2,    KC_3,   KC_0,    TG(_GMPD),
                               _______, KC_TAB,  													                                    KC_MPRV, KC_MNXT,
										                 LCTAB,   KC_SPC,  MO(_LALT),	    _______,  KC_LSFT, QK_BOOT,
												         KC_LCTL, KC_LALT,	   		            KC_LALT
  ),

  [_LALT] = LAYOUT(
     QK_BOOT, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                   			          KC_F6,   KC_F7,   KC_F8,  KC_F9,  KC_F10,  KC_DEL,
    TG(_GMPD),_______, KC_LBRC, KC_COLN, KC_RBRC, KC_LPRN,             	   		              KC_COMM, KC_7,    KC_8,   KC_9,   KC_SCLN, KC_SS,
     KC_DEL,  KC_RCBR, KC_LCBR, KC_UNDS, KC_RCBR, KC_AT,              	   			          KC_QUOT, KC_4,    KC_5,   KC_6,   KC_0,    KC_GRV,
     KC_LGUI, TG(1),   KC_LPRN, KC_HASH, KC_RPRN, KC_MPLY,            	   			          KC_DOT,  KC_1,    KC_2,   KC_3,   KC_0,    TG(_GMPD),
                               KC_MPRV, KC_MNXT,  													 	 	                            _______,_______,
										                 LCTAB, _______, _______,	  	     MO(_FN2),_______, _______,
												                      _______, MO(_FN2),	   		 _______
  ),

  [_FN2] = LAYOUT(
     QK_BOOT, JS_0,    JS_1,    JS_2,    JS_3,    KC_F5,                   			          AM_LOCK,   AM_AP(0),   AM_AP(1),   AM_AP(2),  KC_F10,  KC_BSPC,
     AM_CLTP, KC_PGUP, KC_PGUP, KC_UP,   KC_HOME, KC_SCLN,                 		 	          KC_COMM, KC_7,    KC_8,    KC_9,   KC_SCLN, KC_SS,
     AM_CLBR, KC_HOME, KC_LEFT, KC_DOWN, KC_RGHT, KC_AT,              	   			          KC_COMM, KC_4,    KC_5,    KC_6,   KC_0,    KC_QUOT,
     DB_TOGG, EE_CLR, KC_PGDN, KC_DOWN, KC_END,  KC_MPLY,             	   			          KC_DOT,  KC_1,    KC_2,    KC_3,   KC_0,    KC_DOT,
                               KC_MPRV, KC_MNXT,  													                                    KC_MPRV, KC_MNXT,
										         LCTAB, KC_SPC,  _______,	        _______,  KC_LSFT, QK_BOOT,
												                      KC_LCTL, _______,	   		KC_LALT
  )
};


socd_cleaner_t socd_opposing_pairs[] = {
	{{KC_R, KC_T}, SOCD_CLEANER_LAST},
};


bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
		case MS_BTN1:
			if(record->event.pressed){
				mouse_timer = timer_read32();
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
				tap_code(KC_TRNS);
				layer_off(_MOUSE);
                scrolling_mode = false;
				caret_mode = false;
				mouse_lock = false;
			}
			return true;

		case MO_LALT:
			if (record->event.pressed) {
                if(lalt_held) {
                    unregister_code16(KC_LALT);
                    layer_on(_FN2);
                } else {
                    layer_on(_LALT);
                    caret_mode = true;
                }
			} else {
				layer_off(_LALT);
                layer_off(_FN2);
				caret_mode = false;
			}
			return false;

        case KC_LALT:
            if (record->event.pressed) {
                lalt_held = true;
            } else {
                lalt_held = false;
            }
            return true;

		case ALT_TAB:
			if (record->event.pressed) {
                register_code(KC_LALT);
                tap_code(KC_TAB);
                layer_on(_FN);
                // alt_tab = true;
			} else {
                layer_off(_FN);
				unregister_code(KC_LALT);
				// alt_tab = false;
			}
			return false;

        case LCTAB:
            if (record->event.pressed) {
                register_code(KC_LCTL);
                tap_code(KC_TAB);
            } else {
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
				if(timer_elapsed(dragscroll_timer) < DRAGSCROLL_TIME){
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
				if(timer_elapsed(dragscroll_timer) < DRAGSCROLL_TIME){
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
	}
	return true;
}

void pointing_device_init_kb(void) {
    pointing_device_set_cpi(12000);
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    // Only run this code if a game layer isn't active
    if(game_layer) return mouse_report;

    if(mouse_report.x != 0 || mouse_report.y != 0){
        if(!layer_state_is(_MOUSE)){
            layer_on(_MOUSE);
        }
        mouse_timer = timer_read32();

    }else if((timer_elapsed32(mouse_timer) > MOUSE_LAYER_TIME) && layer_state_is(_MOUSE) && !mouse_lock){
        layer_off(_MOUSE);
    }

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
            game_layer = false;
            if(rgblight_is_enabled()){
				rgblight_disable_noeeprom();
			}
            socd_cleaner_enabled = false;
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

		case _GAME:
            game_layer = true;
			if(!rgblight_is_enabled()){
				rgblight_enable_noeeprom();
			}
			rgblight_sethsv_noeeprom(HSV_TURQUOISE);
            socd_cleaner_enabled = true;
			break;

		case _GMPD:
            game_layer = true;
			if(!rgblight_is_enabled()){
				rgblight_enable_noeeprom();
			}
			rgblight_sethsv_noeeprom(HSV_ORANGE);
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

    debug_enable = true;
    // debug_matrix = true;
    // debug_keyboard = true;
    // debug_mouse = true;
}




#define MATRIX(k0A, k0B, k0C, k0D, k0E, k0F, k6A, k6B, k6C, k6D, k6E, k6F, k1A, k1B, k1C, k1D, k1E, k1F, k7A, k7B, k7C, k7D, k7E, k7F, k2A, k2B, k2C, k2D, k2E, k2F, k8A, k8B, k8C, k8D, k8E, k8F, k3A, k3B, k3C, k3D, k3E, k3F, k9A, k9B, k9C, k9D, k9E, k9F, k4C, k4D, kAC, kAD, k5D, k4E, k4F, kBA, kAA, kAB, k5E, k5F, kBB) \
              {k0A, k0B, k0C, k0D, k0E, k0F, k1A, k1B, k1C, k1D, k1E, k1F, k2A, k2B, k2C, k2D, k2E, k2F, k3A, k3B, k3C, k3D, k3E, k3F, k4C, k4D, k5D, k4E, k4F, k5E, k5F, k6A, k6B, k6C, k6D, k6E, k6F, k7A, k7B, k7C, k7D, k7E, k7F, k8A, k8B, k8C, k8D, k8E, k8F, k9A, k9B, k9C, k9D, k9E, k9F, kAC, kAD, kBA, kAA, kAB, kBB}

//TODO: Find a way to extract info from here (Number of defined profiles etc) and allow declaring stuff like layers here? Prob too much of a hassle
//      Extract key modes as well, to define USE_*
// #define USE_CONSTANT_RAPID_TRIGGER
// const float trigger_height_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM] = {
//   [0] = MATRIX(
//      0.3, 0.3, 0.3, 0.3, 0.3, 0.3,                   			      0.3, 0.3, 0.3, 0.3, 0.3, 0.3,
//      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,                 		 	      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//      0.3, 0.3, 0.3, 0.3, 0.3, 0.3,              	   			      0.3, 0.3, 0.3, 0.3, 0.3, 0.3,
//      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,             	   			      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//                  2.5, 2.5,  											              2.5, 2.5,
//                              2.5, 2.5, 2.5,                      2.5, 2.5, 2.5,
//                                    2.5, 2.5,                      2.5
//   ),

//   [1] = MATRIX(
//      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,                   			      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,                 		 	      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,              	   			      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,             	   			      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//                  2.5, 2.5,  											              2.5, 2.5,
//                              2.5, 2.5, 2.5,                      2.5, 2.5, 2.5,
//                                    2.5, 2.5,                      2.5
//   ),
// };

// const float rt_press_distance_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM] = {
//   [0] = MATRIX(
//      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,                   			      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,                 		 	      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
//      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,              	   			      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,             	   			      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//                  2.5, 2.5,  											              2.5, 2.5,
//                              2.5, 2.5, 2.5,                      2.5, 2.5, 2.5,
//                                    2.5, 2.5,                      2.5
//   ),

//   [1] = MATRIX(
//      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,                   			      2.5, 2.5, 2.5, 2.5, 2.5, 2.5,
//      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,                 		 	      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
//      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,              	   			      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
//      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,             	   			      0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
//                  0.5, 0.5,  											              0.5, 0.5,
//                              0.5, 0.5, 0.5,                      0.5, 0.5, 0.5,
//                                    0.5, 0.5,                      0.5
//   ),
// };

// const uint8_t key_modes_config[AM_PROFILE_NUM][TOTAL_SWITCH_NUM] = {
//   [0] = MATRIX(
//      3, 3, 3, 3, 3, 3,                   			      3, 3, 3, 3, 3, 3,
//      3, 3, 3, 3, 3, 3,                 		 	      3, 3, 3, 3, 3, 3,
//      0, 0, 0, 0, 0, 0,              	   			      0, 0, 0, 0, 0, 0,
//      0, 0, 0, 0, 0, 0,             	   			      0, 0, 0, 0, 0, 0,
//                  0, 0,  											              0, 0,
//                              0, 0, 0,                      0, 0, 0,
//                                    0, 0,                      0
//   ),

//   [1] = MATRIX(
//      0, 0, 0, 0, 0, 0,                   			      3, 3, 3, 3, 3, 3,
//      3, 3, 3, 3, 3, 3,                 		 	      3, 3, 3, 3, 3, 3,
//      3, 3, 3, 3, 3, 3,              	   			      3, 3, 3, 3, 3, 3,
//      3, 3, 3, 3, 3, 3,             	   			      3, 3, 3, 3, 3, 3,
//                  3, 3,  											              3, 3,
//                              3, 3, 3,                      3, 3, 3,
//                                    3, 3,                      3
//   ),
// };




