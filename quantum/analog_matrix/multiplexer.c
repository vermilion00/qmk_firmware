#include "multiplexer.h"
#include <stdint.h>
#include "gpio.h"
#include "analog_matrix.h"
#include "hardware/regs/addressmap.h"
#include "info_config.h"

#define MUX_MASK ((1 << MUX_PIN_NUM) - 1)

#ifdef QMK_MCU_RP2040
#define GPIO_OUT_SET 0xD0000014
#define GPIO_OUT_CLEAR 0xD0000018
#endif

//TODO: Change the port to point to the output register directly
// STM32 needs GPIOx->ODR, AT32 needs GPIOx_ODT(?), AVR needs PORTx(?)
#if defined MUX_PINS
void set_mux_channel(uint8_t channel) {
#if defined MUX_PIN_OFFSET && defined CONTINUOUS_MUX_PORT
#if defined USE_BSRR
#if defined AT32F415
    // Set action takes priority
    CONTINUOUS_MUX_PORT->SCR.W = (MUX_MASK << MUX_PIN_OFFSET << 16) | (channel << MUX_PIN_OFFSET);
#elif defined QMK_MCU_RP2040
    //TODO: Check how this works. Is it similar to the BS and BR registers? Can I do this in one call like the BSRR?
    // GPIO_OUT_CLEAR = (MUX_MASK << MUX_PIN_OFFSET);
    // GPIO_OUT_SET = (channel << MUX_PIN_OFFSET);
    *((volatile uint64_t *)GPIO_OUT_SET) = ((uint64_t)MUX_MASK << (MUX_PIN_OFFSET + 16)) | (channel << MUX_PIN_OFFSET);
#else
    // Set action takes priority
    CONTINUOUS_MUX_PORT->BSRR.W = (MUX_MASK << MUX_PIN_OFFSET << 16) | (channel << MUX_PIN_OFFSET);
#endif // defined AT32F415
#else
    CONTINUOUS_MUX_PORT->ODR = (CONTINUOUS_MUX_PORT->ODR & ~(MUX_MASK << MUX_PIN_OFFSET)) | (channel << MUX_PIN_OFFSET);
#endif // defined USE_BSRR

#else // if defined MUX_PIN_OFFSET && defined CONTINUOUS_MUX_PORT
    switch(channel){
        case 0:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_low(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
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
            #if MUX_PIN_NUM > 4
            gpio_write_pin_(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

#   if MUX_PIN_NUM > 4

        case 16:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 17:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 18:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 19:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 20:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 21:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 22:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 23:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_low(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 24:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 25:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 26:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 27:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_low(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 28:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 29:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_low(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 30:
            gpio_write_pin_low(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

        case 31:
            gpio_write_pin_high(mux_pins[0]);
            #if MUX_PIN_NUM > 1
            gpio_write_pin_high(mux_pins[1]);
            #if MUX_PIN_NUM > 2
            gpio_write_pin_high(mux_pins[2]);
            #if MUX_PIN_NUM > 3
            gpio_write_pin_high(mux_pins[3]);
            #if MUX_PIN_NUM > 4
            gpio_write_pin_high(mux_pins[4]);
            #endif
            #endif
            #endif
            #endif
            break;

#   endif //if MUX_PIN_NUM > 4
#   endif //if MUX_PIN_NUM > 3
#   endif //if MUX_PIN_NUM > 2
#   endif //if MUX_PIN_NUM > 1
    }
#endif // if defined MUX_PIN_OFFSET && defined CONTINUOUS_MUX_PORT else
}
#else // defined MUX_PINS
#   define set_mux_channel(channel)
#endif // defined MUX_PINS else
