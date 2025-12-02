#include "multiplexer.h"
#include <stdint.h>
#include "gpio.h"
#include "he_matrix.h"
#include "info_config.h"

#define MUX_MASK ((1 << MUX_PIN_NUM) - 1)
void set_mux_channel(uint8_t channel) {
#if defined MUX_PINS
#if defined MUX_PIN_OFFSET && defined CONTINUOUS_MUX_PORT
    CONTINUOUS_MUX_PORT->ODR = (CONTINUOUS_MUX_PORT->ODR & ~(MUX_MASK << MUX_PIN_OFFSET)) | (channel << MUX_PIN_OFFSET);
#else
    switch(channel){
        case 0:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 1:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
#   if MUX_PIN_NUM > 1
        case 2:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 3:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
#   if MUX_PIN_NUM > 2
        case 4:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 5:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 6:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 7:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
#   if MUX_PIN_NUM > 3
        case 8:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 9:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 10:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 11:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 12:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 13:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 14:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
        case 15:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #endif
            #endif
            #endif
            break;
#   endif //if MUX_PIN_NUM > 3
#   endif //if MUX_PIN_NUM > 2
#   endif //if MUX_PIN_NUM > 1
    }
#endif // else MUX_PIN_OFFSET && MUX_PORT
#endif // defined MUX_PINS
}
