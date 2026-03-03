#pragma once

#include "info_config.h"
//TODO: Decide if I want to have multiple names for one button for one scheme

#if !defined XBOX_LAYOUT && !defined PLAYSTATION_LAYOUT && !defined NINTENDO_LAYOUT
#   define XBOX_LAYOUT
#endif

/* XBOX button naming */
#ifdef XBOX_LAYOUT
#define JS_A  JS_0
#define JS_B  JS_1
#define JS_X  JS_2
#define JS_Y  JS_3
#define JS_LB JS_4
#define JS_RB JS_5
#define JS_BACK JS_6
#define JS_STRT JS_7
#define JS_START JS_7
#define JS_LS JS_8
#define JS_RS JS_9
//TODO: Check if these are correct. Do the JS_n buttons even map to the expected buttons correctly?
#define JS_DPAD_UP JS_10
#define JS_DPU JS_10
#define JS_DPAD_DOWN JS_11
#define JS_DPD JS_11
#define JS_DPAD_LEFT JS_12
#define JS_DPL JS_12
#define JS_DPAD_RIGHT JS_13
#define JS_DPR JS_13
#define JS_LT JS_LNZ
#define JS_RT JS_LPZ
#endif

/* Playstation button naming */
#ifdef PLAYSTATION_LAYOUT
#define JS_X JS_0
#define JS_CROSS JS_0
#define JS_CRSS JS_0
#define JS_O JS_1
#define JS_CIRCLE JS_1
#define JS_CRCL JS_1
#define JS_SQUARE JS_2
#define JS_SQRE JS_2
#define JS_TRIANGLE JS_3
#define JS_TNGL JS_3
#define JS_L1 JS_4
#define JS_R1 JS_5
#define JS_L2 JS_LT
#define JS_R2 JS_RT
#define JS_SHARE JS_6
#define JS_SHRE JS_6
#define JS_OPTIONS JS_7
#define JS_OPTN JS_7
#define JS_L3 JS_8
#define JS_R3 JS_9
#define JS_DPAD_UP JS_10
#define JS_DPU JS_10
#define JS_DPAD_DOWN JS_11
#define JS_DPD JS_11
#define JS_DPAD_LEFT JS_12
#define JS_DPL JS_12
#define JS_DPAD_RIGHT JS_13
#define JS_DPR JS_13
#endif

/* Nintendo button naming */
#ifdef NINTENDO_LAYOUT
#define JS_B JS_0
#define JS_A JS_1
#define JS_Y JS_2
#define JS_X JS_3
#define JS_L JS_4
#define JS_R JS_5
#define JS_MINUS JS_6
#define JS_MINS JS_6
#define JS_PLUS JS_7
#define JS_LS JS_8
#define JS_RS JS_9
#define JS_DPAD_UP JS_10
#define JS_DPU JS_10
#define JS_DPAD_DOWN JS_11
#define JS_DPD JS_11
#define JS_DPAD_LEFT JS_12
#define JS_DPL JS_12
#define JS_DPAD_RIGHT JS_13
#define JS_DPR JS_13
#define JS_ZL JS_LT
#define JS_ZR JS_RT
#endif
