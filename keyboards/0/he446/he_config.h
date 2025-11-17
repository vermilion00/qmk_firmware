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
-Allow enabling rapid trigger on specific keys only
*/

//Define this automatically at compile time
// #define SWITCH_NUM 4

//TODO: When these defines only have one value, use it for every switch
// #define TRIGGER_HEIGHT { 1.0, 2.0, 3.0, 0.5 }

// #define RELEASE_HEIGHT { 1.0, 2.0, 3.0, 0.5 }

// #define RT_PRESS_DISTANCE  { 0.5, 0.5, 0.2, 2.0 }

// #define RT_RELEASE_DISTANCE  { 0.5, 0.5, 0.2, 2.0 }

// #define TRAVEL_DISTANCE 3.8

//1,2,3
// #define SMOOTHING_LEVEL 1
//1,2,3,4,5
// #define DEADZONE_LEVEL 3

// #define HE_PINS { A4, A0 }
// #define HE_PINS_RIGHT {}
// #define HE_PIN_NUM 2
// #define MUX_PIN_OFFSET 12
// #define MUX_PORT GPIOB
// #define DIRECT_ADC_PINS
// #define MUX_PINS { B12 }
// #define MUX_PINS_RIGHT {}
// #define MUX_PIN_NUM 1
// #define ADC_RESOLUTION 12

// If the ADC value increases as you press a switch further down, define this
// #define INVERT_ADC

// Translates from mux combination to matrix position
// Rows are adc channels, cols are mux channels
// Use a 0 to indicate no connection
// #define MUX_TO_NUM { {1,2} }

// Translates from he matrix to qmk matrix
// [ROW][COL]
// Follows QMK layout macro style, so split keyboard right half is set as a secondary set of rows beneath the left side
// #define NUM_TO_MATRIX { {0,0}, {0,1}, {1,0}, {1,1} }

/* Rapid trigger only activates below the trigger height and deactivates
   above it */
// #define RAPID_TRIGGER

/* Rapid trigger is always active, without an additional
   trigger height check */
// #define CONSTANT_RAPID_TRIGGER

/* Rapid trigger is activates below the trigger height, but remains
   active until the key is fully released */
// #define CONTINUOUS_RAPID_TRIGGER

//How are the sensors powered? Is every sensor powered all the time?
//Do they need to be powered by a pin before being read?
//#define CONSTANT_POWER

//TODO: Add a call to a user-defined function to allow them to control this
/* Make an array equal to 1 << MUX_PIN_NUM, every time the mux pins are changed,
   set and reset the appropriate pins so that the sensors about to be scanned are powered */
//#define POWER_BEFORE_SCAN
// #define POWER_PINS { B0, B1, B2 }
//#define POWER_DELAY_US 1
//#define CUSTOM_POWER_BEFORE_SCAN

// How many microseconds to wait before reading the adc
// TODO: Implement delay in the code somehow (QMK has avr delay files, prob smth for stm32 too)
//#define SCAN_DELAY_US 1

// Skip EEPROM Reading/Writing
//TODO: Implement EEPROM
#define NO_EEPROM

// TESTING STUFF

#define DYNAMIC_CALIBRATION

#define INITIAL_TOP_VALUE 580
#define INITIAL_BOTTOM_VALUE 310

#define TOP_VALUES {[0 ... SWITCH_NUM] =  630 }
#define BOTTOM_VALUES {[0 ... SWITCH_NUM] = 280 }
