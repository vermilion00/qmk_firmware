#include "multiplexer.h"
#include <stdint.h>
#include "gpio.h"
#include "analog_matrix.h"
#include "info_config.h"

#define MUX_MASK ((1 << MUX_PIN_NUM) - 1)

//TODO: Change the port to point to the output register directly
// STM32 needs GPIOx->ODR, AT32 needs GPIOx_ODT, AVR needs PORTx(?)
void set_mux_channel(uint8_t channel) {
#if defined MUX_PINS
#if defined MUX_PIN_OFFSET && defined CONTINUOUS_MUX_PORT
#if defined USE_BSRR && defined AT32F415
    // Set action takes priority
    CONTINUOUS_MUX_PORT->SCR.W = (MUX_MASK << MUX_PIN_OFFSET << 16) | (channel << MUX_PIN_OFFSET);
#elif defined USE_BSRR
    // Set action takes priority
    CONTINUOUS_MUX_PORT->BSRR.W = (MUX_MASK << MUX_PIN_OFFSET << 16) | (channel << MUX_PIN_OFFSET);
// #endif
#else
    CONTINUOUS_MUX_PORT->ODR = (CONTINUOUS_MUX_PORT->ODR & ~(MUX_MASK << MUX_PIN_OFFSET)) | (channel << MUX_PIN_OFFSET);
#endif // defined USE_BSRR

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
