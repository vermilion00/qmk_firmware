// Copyright 2026 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

/*******************************************************************************
  88888888888 888      d8b                .d888 d8b 888               d8b
      888     888      Y8P               d88P"  Y8P 888               Y8P
      888     888                        888        888
      888     88888b.  888 .d8888b       888888 888 888  .d88b.       888 .d8888b
      888     888 "88b 888 88K           888    888 888 d8P  Y8b      888 88K
      888     888  888 888 "Y8888b.      888    888 888 88888888      888 "Y8888b.
      888     888  888 888      X88      888    888 888 Y8b.          888      X88
      888     888  888 888  88888P'      888    888 888  "Y8888       888  88888P'
                                                        888                 888
                                                        888                 888
                                                        888                 888
     .d88b.   .d88b.  88888b.   .d88b.  888d888 8888b.  888888 .d88b.   .d88888
    d88P"88b d8P  Y8b 888 "88b d8P  Y8b 888P"      "88b 888   d8P  Y8b d88" 888
    888  888 88888888 888  888 88888888 888    .d888888 888   88888888 888  888
    Y88b 888 Y8b.     888  888 Y8b.     888    888  888 Y88b. Y8b.     Y88b 888
     "Y88888  "Y8888  888  888  "Y8888  888    "Y888888  "Y888 "Y8888   "Y88888
         888
    Y8b d88P
     "Y88P"
*******************************************************************************/

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <keycodes.h>

#include "compiler_support.h"

#define COMMUNITY_MODULES_API_VERSION_BUILDER(ver_major,ver_minor,ver_patch) (((((uint32_t)(ver_major))&0xFF) << 24) | ((((uint32_t)(ver_minor))&0xFF) << 16) | (((uint32_t)(ver_patch))&0xFF))
#define COMMUNITY_MODULES_API_VERSION COMMUNITY_MODULES_API_VERSION_BUILDER(1,1,1)
#define ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(ver_major,ver_minor,ver_patch) STATIC_ASSERT(COMMUNITY_MODULES_API_VERSION_BUILDER(ver_major,ver_minor,ver_patch) <= COMMUNITY_MODULES_API_VERSION, "Community module requires a newer version of QMK modules API -- needs: " #ver_major "." #ver_minor "." #ver_patch ", current: 1.1.1.")

typedef struct keyrecord_t keyrecord_t; // forward declaration so we don't need to include quantum.h

enum {
    // From module: socd_cleaner
    SOCDON = QK_COMMUNITY_MODULE,
    SOCDOFF,
    SOCDTOG,

    // From module: pointing_device_accel
    CM_MOUSE_ACCEL_TOGGLE,
    MA_TOGG = CM_MOUSE_ACCEL_TOGGLE,
    CM_MOUSE_ACCEL_TAKEOFF,
    MA_TKOF = CM_MOUSE_ACCEL_TAKEOFF,
    CM_MOUSE_ACCEL_GROWTH_RATE,
    MA_GROW = CM_MOUSE_ACCEL_GROWTH_RATE,
    CM_MOUSE_ACCEL_OFFSET,
    MA_OFST = CM_MOUSE_ACCEL_OFFSET,
    CM_MOUSE_ACCEL_LIMIT,
    MA_LMT = CM_MOUSE_ACCEL_LIMIT,

    LAST_COMMUNITY_MODULE_KEY
};
STATIC_ASSERT((int)LAST_COMMUNITY_MODULE_KEY <= (int)(QK_COMMUNITY_MODULE_MAX+1), "Too many community module keycodes");

#if defined(OS_DETECTION_ENABLE)
#include <os_detection.h>
#endif  // defined(OS_DETECTION_ENABLE)

#if defined(POINTING_DEVICE_ENABLE)
#include <report.h>
#endif  // defined(POINTING_DEVICE_ENABLE)

#if defined(RGB_MATRIX_ENABLE)
#include <rgb_matrix.h>
#endif  // defined(RGB_MATRIX_ENABLE)

#if defined(RGB_MATRIX_ENABLE)
#include <rgb_matrix.h>
#endif  // defined(RGB_MATRIX_ENABLE)

#if defined(LED_MATRIX_ENABLE)
#include <led_matrix.h>
#endif  // defined(LED_MATRIX_ENABLE)

#if defined(LED_MATRIX_ENABLE)
#include <led_matrix.h>
#endif  // defined(LED_MATRIX_ENABLE)

#if !defined(NO_ACTION_LAYER)
#include <action_layer.h>
#endif  // !defined(NO_ACTION_LAYER)

#if !defined(NO_ACTION_LAYER)
#include <action_layer.h>
#endif  // !defined(NO_ACTION_LAYER)

// From module: getreuer/socd_cleaner

void keyboard_pre_init_socd_cleaner_user(void);
void keyboard_pre_init_socd_cleaner_kb(void);
void keyboard_pre_init_socd_cleaner(void);

void keyboard_post_init_socd_cleaner_user(void);
void keyboard_post_init_socd_cleaner_kb(void);
void keyboard_post_init_socd_cleaner(void);

bool pre_process_record_socd_cleaner_user(uint16_t keycode, keyrecord_t *record);
bool pre_process_record_socd_cleaner_kb(uint16_t keycode, keyrecord_t *record);
bool pre_process_record_socd_cleaner(uint16_t keycode, keyrecord_t *record);

bool process_record_socd_cleaner_user(uint16_t keycode, keyrecord_t *record);
bool process_record_socd_cleaner_kb(uint16_t keycode, keyrecord_t *record);
bool process_record_socd_cleaner(uint16_t keycode, keyrecord_t *record);

void post_process_record_socd_cleaner_user(uint16_t keycode, keyrecord_t *record);
void post_process_record_socd_cleaner_kb(uint16_t keycode, keyrecord_t *record);
void post_process_record_socd_cleaner(uint16_t keycode, keyrecord_t *record);

void housekeeping_task_socd_cleaner_user(void);
void housekeeping_task_socd_cleaner_kb(void);
void housekeeping_task_socd_cleaner(void);

void suspend_power_down_socd_cleaner_user(void);
void suspend_power_down_socd_cleaner_kb(void);
void suspend_power_down_socd_cleaner(void);

void suspend_wakeup_init_socd_cleaner_user(void);
void suspend_wakeup_init_socd_cleaner_kb(void);
void suspend_wakeup_init_socd_cleaner(void);

bool shutdown_socd_cleaner_user(bool jump_to_bootloader);
bool shutdown_socd_cleaner_kb(bool jump_to_bootloader);
bool shutdown_socd_cleaner(bool jump_to_bootloader);

#if defined(OS_DETECTION_ENABLE)
bool process_detected_host_os_socd_cleaner_user(os_variant_t os);
bool process_detected_host_os_socd_cleaner_kb(os_variant_t os);
bool process_detected_host_os_socd_cleaner(os_variant_t os);
#endif  // defined(OS_DETECTION_ENABLE)

#if defined(POINTING_DEVICE_ENABLE)
void pointing_device_init_socd_cleaner_user(void);
void pointing_device_init_socd_cleaner_kb(void);
void pointing_device_init_socd_cleaner(void);
#endif  // defined(POINTING_DEVICE_ENABLE)

#if defined(POINTING_DEVICE_ENABLE)
report_mouse_t pointing_device_task_socd_cleaner_user(report_mouse_t mouse_report);
report_mouse_t pointing_device_task_socd_cleaner_kb(report_mouse_t mouse_report);
report_mouse_t pointing_device_task_socd_cleaner(report_mouse_t mouse_report);
#endif  // defined(POINTING_DEVICE_ENABLE)

#if defined(RGB_MATRIX_ENABLE)
bool rgb_matrix_indicators_socd_cleaner_user(void);
bool rgb_matrix_indicators_socd_cleaner_kb(void);
bool rgb_matrix_indicators_socd_cleaner(void);
#endif  // defined(RGB_MATRIX_ENABLE)

#if defined(RGB_MATRIX_ENABLE)
bool rgb_matrix_indicators_advanced_socd_cleaner_user(uint8_t led_min, uint8_t led_max);
bool rgb_matrix_indicators_advanced_socd_cleaner_kb(uint8_t led_min, uint8_t led_max);
bool rgb_matrix_indicators_advanced_socd_cleaner(uint8_t led_min, uint8_t led_max);
#endif  // defined(RGB_MATRIX_ENABLE)

#if defined(LED_MATRIX_ENABLE)
bool led_matrix_indicators_socd_cleaner_user(void);
bool led_matrix_indicators_socd_cleaner_kb(void);
bool led_matrix_indicators_socd_cleaner(void);
#endif  // defined(LED_MATRIX_ENABLE)

#if defined(LED_MATRIX_ENABLE)
bool led_matrix_indicators_advanced_socd_cleaner_user(uint8_t led_min, uint8_t led_max);
bool led_matrix_indicators_advanced_socd_cleaner_kb(uint8_t led_min, uint8_t led_max);
bool led_matrix_indicators_advanced_socd_cleaner(uint8_t led_min, uint8_t led_max);
#endif  // defined(LED_MATRIX_ENABLE)

#if !defined(NO_ACTION_LAYER)
layer_state_t default_layer_state_set_socd_cleaner_user(layer_state_t state);
layer_state_t default_layer_state_set_socd_cleaner_kb(layer_state_t state);
layer_state_t default_layer_state_set_socd_cleaner(layer_state_t state);
#endif  // !defined(NO_ACTION_LAYER)

#if !defined(NO_ACTION_LAYER)
layer_state_t layer_state_set_socd_cleaner_user(layer_state_t state);
layer_state_t layer_state_set_socd_cleaner_kb(layer_state_t state);
layer_state_t layer_state_set_socd_cleaner(layer_state_t state);
#endif  // !defined(NO_ACTION_LAYER)

// From module: drashna/pointing_device_accel

void keyboard_pre_init_pointing_device_accel_user(void);
void keyboard_pre_init_pointing_device_accel_kb(void);
void keyboard_pre_init_pointing_device_accel(void);

void keyboard_post_init_pointing_device_accel_user(void);
void keyboard_post_init_pointing_device_accel_kb(void);
void keyboard_post_init_pointing_device_accel(void);

bool pre_process_record_pointing_device_accel_user(uint16_t keycode, keyrecord_t *record);
bool pre_process_record_pointing_device_accel_kb(uint16_t keycode, keyrecord_t *record);
bool pre_process_record_pointing_device_accel(uint16_t keycode, keyrecord_t *record);

bool process_record_pointing_device_accel_user(uint16_t keycode, keyrecord_t *record);
bool process_record_pointing_device_accel_kb(uint16_t keycode, keyrecord_t *record);
bool process_record_pointing_device_accel(uint16_t keycode, keyrecord_t *record);

void post_process_record_pointing_device_accel_user(uint16_t keycode, keyrecord_t *record);
void post_process_record_pointing_device_accel_kb(uint16_t keycode, keyrecord_t *record);
void post_process_record_pointing_device_accel(uint16_t keycode, keyrecord_t *record);

void housekeeping_task_pointing_device_accel_user(void);
void housekeeping_task_pointing_device_accel_kb(void);
void housekeeping_task_pointing_device_accel(void);

void suspend_power_down_pointing_device_accel_user(void);
void suspend_power_down_pointing_device_accel_kb(void);
void suspend_power_down_pointing_device_accel(void);

void suspend_wakeup_init_pointing_device_accel_user(void);
void suspend_wakeup_init_pointing_device_accel_kb(void);
void suspend_wakeup_init_pointing_device_accel(void);

bool shutdown_pointing_device_accel_user(bool jump_to_bootloader);
bool shutdown_pointing_device_accel_kb(bool jump_to_bootloader);
bool shutdown_pointing_device_accel(bool jump_to_bootloader);

#if defined(OS_DETECTION_ENABLE)
bool process_detected_host_os_pointing_device_accel_user(os_variant_t os);
bool process_detected_host_os_pointing_device_accel_kb(os_variant_t os);
bool process_detected_host_os_pointing_device_accel(os_variant_t os);
#endif  // defined(OS_DETECTION_ENABLE)

#if defined(POINTING_DEVICE_ENABLE)
void pointing_device_init_pointing_device_accel_user(void);
void pointing_device_init_pointing_device_accel_kb(void);
void pointing_device_init_pointing_device_accel(void);
#endif  // defined(POINTING_DEVICE_ENABLE)

#if defined(POINTING_DEVICE_ENABLE)
report_mouse_t pointing_device_task_pointing_device_accel_user(report_mouse_t mouse_report);
report_mouse_t pointing_device_task_pointing_device_accel_kb(report_mouse_t mouse_report);
report_mouse_t pointing_device_task_pointing_device_accel(report_mouse_t mouse_report);
#endif  // defined(POINTING_DEVICE_ENABLE)

#if defined(RGB_MATRIX_ENABLE)
bool rgb_matrix_indicators_pointing_device_accel_user(void);
bool rgb_matrix_indicators_pointing_device_accel_kb(void);
bool rgb_matrix_indicators_pointing_device_accel(void);
#endif  // defined(RGB_MATRIX_ENABLE)

#if defined(RGB_MATRIX_ENABLE)
bool rgb_matrix_indicators_advanced_pointing_device_accel_user(uint8_t led_min, uint8_t led_max);
bool rgb_matrix_indicators_advanced_pointing_device_accel_kb(uint8_t led_min, uint8_t led_max);
bool rgb_matrix_indicators_advanced_pointing_device_accel(uint8_t led_min, uint8_t led_max);
#endif  // defined(RGB_MATRIX_ENABLE)

#if defined(LED_MATRIX_ENABLE)
bool led_matrix_indicators_pointing_device_accel_user(void);
bool led_matrix_indicators_pointing_device_accel_kb(void);
bool led_matrix_indicators_pointing_device_accel(void);
#endif  // defined(LED_MATRIX_ENABLE)

#if defined(LED_MATRIX_ENABLE)
bool led_matrix_indicators_advanced_pointing_device_accel_user(uint8_t led_min, uint8_t led_max);
bool led_matrix_indicators_advanced_pointing_device_accel_kb(uint8_t led_min, uint8_t led_max);
bool led_matrix_indicators_advanced_pointing_device_accel(uint8_t led_min, uint8_t led_max);
#endif  // defined(LED_MATRIX_ENABLE)

#if !defined(NO_ACTION_LAYER)
layer_state_t default_layer_state_set_pointing_device_accel_user(layer_state_t state);
layer_state_t default_layer_state_set_pointing_device_accel_kb(layer_state_t state);
layer_state_t default_layer_state_set_pointing_device_accel(layer_state_t state);
#endif  // !defined(NO_ACTION_LAYER)

#if !defined(NO_ACTION_LAYER)
layer_state_t layer_state_set_pointing_device_accel_user(layer_state_t state);
layer_state_t layer_state_set_pointing_device_accel_kb(layer_state_t state);
layer_state_t layer_state_set_pointing_device_accel(layer_state_t state);
#endif  // !defined(NO_ACTION_LAYER)

// Core wrapper

void keyboard_pre_init_modules(void);

void keyboard_post_init_modules(void);

bool pre_process_record_modules(uint16_t keycode, keyrecord_t *record);

bool process_record_modules(uint16_t keycode, keyrecord_t *record);

void post_process_record_modules(uint16_t keycode, keyrecord_t *record);

void housekeeping_task_modules(void);

void suspend_power_down_modules(void);

void suspend_wakeup_init_modules(void);

bool shutdown_modules(bool jump_to_bootloader);

#if defined(OS_DETECTION_ENABLE)
bool process_detected_host_os_modules(os_variant_t os);
#endif  // defined(OS_DETECTION_ENABLE)

#if defined(POINTING_DEVICE_ENABLE)
void pointing_device_init_modules(void);
#endif  // defined(POINTING_DEVICE_ENABLE)

#if defined(POINTING_DEVICE_ENABLE)
report_mouse_t pointing_device_task_modules(report_mouse_t mouse_report);
#endif  // defined(POINTING_DEVICE_ENABLE)

#if defined(RGB_MATRIX_ENABLE)
bool rgb_matrix_indicators_modules(void);
#endif  // defined(RGB_MATRIX_ENABLE)

#if defined(RGB_MATRIX_ENABLE)
bool rgb_matrix_indicators_advanced_modules(uint8_t led_min, uint8_t led_max);
#endif  // defined(RGB_MATRIX_ENABLE)

#if defined(LED_MATRIX_ENABLE)
bool led_matrix_indicators_modules(void);
#endif  // defined(LED_MATRIX_ENABLE)

#if defined(LED_MATRIX_ENABLE)
bool led_matrix_indicators_advanced_modules(uint8_t led_min, uint8_t led_max);
#endif  // defined(LED_MATRIX_ENABLE)

#if !defined(NO_ACTION_LAYER)
layer_state_t default_layer_state_set_modules(layer_state_t state);
#endif  // !defined(NO_ACTION_LAYER)

#if !defined(NO_ACTION_LAYER)
layer_state_t layer_state_set_modules(layer_state_t state);
#endif  // !defined(NO_ACTION_LAYER)
