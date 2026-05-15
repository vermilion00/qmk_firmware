#pragma once

#include "info_config.h"
#include <stdint.h>

#define SPLIT_VIA_MUT
#define VIA_MUTABLE

// If VIA is enabled, all modes should be included by default unless specifically turned off
#if !defined USE_NONE
#   define USE_NONE
#endif
#if defined NO_NONE
#   undef USE_NONE
#endif
#if !defined USE_RAPID_TRIGGER
#   define USE_RAPID_TRIGGER
#endif
#if defined NO_RAPID_TRIGGER
#   undef USE_RAPID_TRIGGER
#endif
#if !defined USE_CONTINUOUS_RAPID_TRIGGER
#   define USE_CONTINUOUS_RAPID_TRIGGER
#endif
#if defined NO_CONTINUOUS_RAPID_TRIGGER
#   undef USE_CONTINUOUS_RAPID_TRIGGER
#endif
#if !defined USE_CONSTANT_RAPID_TRIGGER
#   define USE_CONSTANT_RAPID_TRIGGER
#endif
#if defined NO_CONSTANT_RAPID_TRIGGER
#   undef USE_CONSTANT_RAPID_TRIGGER
#endif

#if !defined USE_TRIGGER_HEIGHT && (defined USE_NONE || defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER)
#   define USE_TRIGGER_HEIGHT
#endif
#if !defined USE_RT_DISTANCE && (defined USE_RAPID_TRIGGER || defined USE_CONTINUOUS_RAPID_TRIGGER || defined USE_CONSTANT_RAPID_TRIGGER)
#   define USE_RT_DISTANCE
#endif

#if !defined NO_DYNAMIC_CALIBRATION && !defined DYNAMIC_CALIBRATION
#   define DYNAMIC_CALIBRATION
#endif

#if !defined NO_PRIORITY_MODE && !defined USE_PRIORITY_MODE
#   define USE_PRIORITY_MODE
#endif
#if !defined NO_PRIORITY_MODE && !defined NO_PRIORITY_INDICES
#   define PRIORITY_INDICES {[0 ... SWITCH_NUM-1] = 0}
#endif

#if defined SPLIT_KEYBOARD && !defined SPLIT_LAYER_SYNC
#   define SPLIT_LAYER_SYNC
#endif

#if PROFILE_SWITCH_MODE == 2
#undef PROFILE_SWITCH_MODE
#define PROFILE_SWITCH_MODE 0
#endif

#if !defined SPLIT_KEYBOARD
#ifdef TRIGGER_HEIGHT
#   define TOTAL_TRIGGER_HEIGHT TRIGGER_HEIGHT
#   define TOTAL_RELEASE_HEIGHT RELEASE_HEIGHT
#endif
#ifdef RT_PRESS_DISTANCE
#   define TOTAL_RT_PRESS_DISTANCE RT_PRESS_DISTANCE
#   define TOTAL_RT_RELEASE_DISTANCE RT_RELEASE_DISTANCE
#endif
#define TOTAL_KEY_MODES KEY_MODES
#endif

#ifdef JOYSTICK_ENABLE
#   undef JOYSTICK_AXIS_COUNT
#   define JOYSTICK_AXIS_COUNT 6
#   if JOYSTICK_BUTTON_COUNT < 16
#       undef JOYSTICK_BUTTON_COUNT
#       define JOSYTICK_BUTTON_COUNT 16
#   endif
#endif

//TODOD: Perhaps move these elsewhere?
extern uint16_t bottom_deadzone;
extern uint16_t smoothing;
