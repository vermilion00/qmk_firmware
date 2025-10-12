#include <stdint.h>
#include <stdbool.h>

typedef struct Switch {
    bool pressed;
    uint16_t trigger_height;
    #ifdef RAPID_TRIGGER
    uint16_t lowest_value;
    uint16_t rapid_distance;
    #endif
    uint16_t min;
    uint16_t max;
} Switch;
