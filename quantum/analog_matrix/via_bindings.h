#pragma once
#include "info_config.h"

#include <stdint.h>

#define SPLIT_VIA_MUT
#define VIA_MUTABLE

// If VIA is enabled, all modes should be included by default unless specifically turned off
#if !defined USE_NONE && !defined NO_NONE
#   define USE_NONE
#elif defined NO_NONE
#   undef USE_NONE
#endif
#if !defined USE_RAPID_TRIGGER && !defined NO_RAPID_TRIGGER
#   define USE_RAPID_TRIGGER
#elif defined NO_RAPID_TRIGGER
#   undef USE_RAPID_TRIGGER
#endif
#if !defined USE_CONTINUOUS_RAPID_TRIGGER && !defined NO_CONTINUOUS_RAPID_TRIGGER
#   define USE_CONTINUOUS_RAPID_TRIGGER
#elif defined NO_CONTINUOUS_RAPID_TRIGGER
#   undef USE_CONTINUOUS_RAPID_TRIGGER
#endif
#if !defined USE_CONSTANT_RAPID_TRIGGER && !defined NO_CONSTANT_RAPID_TRIGGER
#   define USE_CONSTANT_RAPID_TRIGGER
#elif defined NO_CONSTANT_RAPID_TRIGGER
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

#if PROFILE_SWITCH_MODE == 2
#undef PROFILE_SWITCH_MODE
#define PROFILE_SWITCH_MODE 0
#endif

extern uint16_t bottom_deadzone;
extern uint16_t smoothing;
