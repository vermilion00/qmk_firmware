#include <stdint.h>
#include <stdbool.h>
#include "he_config.h"
#include "matrix_ref.h"

typedef enum {
    up = 0,
    down
} key_dir_t;

typedef struct Switch {
    bool pressed;
    uint16_t trigger_height;
    #ifdef RAPID_TRIGGER
    uint16_t lowest_value;
    uint16_t rapid_distance;
    #endif
    uint16_t min;
    uint16_t max;
    key_dir_t direction;
} Switch;

volatile Switch matrix[ROWS][COLUMNS];

/* Get the min and max values of each switch */
void calibrate_switches(void);
/* Read calibration data from eeprom */
void read_eeprom(void);
