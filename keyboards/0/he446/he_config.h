#pragma once

/*
-Pins are set to input by default -> check if QMK sets unused pins to output, if not diy
-Output is only 3.3v, maybe not enough to change mux channel?
-LED on B2 is active high
-Button on C13 is active high
-Set a default EEPROM configuration when enabling HE, and allow overwriting with a custom config
-Check side and adjust pins if _RIGHT is defined
-ADC Value changes faster at the top then at the bottom, adjust with a lut
    -Allow defining own lut
-Support for switch presets containing travel distance and lut
-If no switch data is in eeprom, immediately go into calibration mode, save data to eeprom when no changes have been made for many cycles, force reboot
*/

//Define this automatically at compile time
#define SWITCH_NUM 4

#define TRIGGER_HEIGHT { 1.0, 2.0, 3.0, 0.5 }

#define RELEASE_HEIGHT { 1.0, 2.0, 3.0, 0.5 }

#define RT_PRESS_DISTANCE  { 0.5, 0.5, 0.2, 2.0 }

#define RT_RELEASE_DISTANCE  { 0.5, 0.5, 0.2, 2.0 }

//
#define TRAVEL_DISTANCE 3.8

//1,2,3
#define SMOOTHING_LEVEL 1
//1,2,3,4,5
#define DEADZONE_LEVEL 3

#define HE_PINS { A4 }
// #define HE_PINS_RIGHT {}
// #define HE_PIN_NUM 2
// #define MUX_PINS_CONTINUOUS
#define MUX_PIN_OFFSET 12
#define MUX_PORT GPIOB
#define MUX_PINS { B12 }
// #define MUX_PINS_RIGHT {}
#define MUX_PIN_NUM 2
#define ADC_RESOLUTION 12

// Translates from mux combination to matrix position
// Rows are adc channels, cols are mux channels
// Use a 0 to indicate no connection
#define MUX_TO_MATRIX {{1,2}}

/* Rapid trigger only activates below the trigger height and deactivates
   above it */
// #define RAPID_TRIGGER

/* Rapid trigger is always active, without an additional
   trigger height check */
// #define CONSTANT_RAPID_TRIGGER

/* Rapid trigger is activates below the trigger height, but remains
   active until the key is fully released */
#define CONTINUOUS_RAPID_TRIGGER

//How are the sensors powered? Is every sensor powered all the time?
//Do they need to be powered by a pin before being read?
//#define CONSTANT_POWER
//TODO: Add a call to a user-defined function to allow them to control this
/* Make an array equal to the 1 << MUX_PIN_NUM, every time the mux pins are changed,
   set and reset the appropriate pins so that the sensors about to be scanned are powered */
//#define POWER_BEFORE_SCAN
#define POWER_PINS { B0, B1, B2 }
//#define POWER_DELAY_US 1
//#define CUSTOM_POWER_BEFORE_SCAN

// How many microseconds to wait before reading the adc
//#define SCAN_DELAY_US 1

// Skip EEPROM Reading/Writing
#define NO_EEPROM

// TESTING STUFF

#define DYNAMIC_CALIBRATION

#define INITIAL_TOP_VALUE 580
#define INITIAL_BOTTOM_VALUE 310

#define TOP_VALUES { 630, 630, 630, 630 }
#define BOTTOM_VALUES { 280, 280, 280, 280 }
