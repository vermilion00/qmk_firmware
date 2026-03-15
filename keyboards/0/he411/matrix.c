#include "_wait.h"
#include "analog_matrix.h"
#include "print.h"

uint8_t analog_matrix_scan(void) {
    // calibrate_switches(false);
    uint16_t val = adc_read(pinToMux(A0));
    printf("%u\n", val);
    printf("%u, %u\n", key_config[0].top_value, key_config[0].bottom_value);
    // printf("%u, %u\n", key_config[1].top_value, key_config[1].bottom_value);
    // printf("%u, %u\n", key_config[2].top_value, key_config[2].bottom_value);
    // printf("%u, %u\n", key_config[3].top_value, key_config[3].bottom_value);
    // printf("%u, %u\n", key_config[4].top_value, key_config[4].bottom_value);
    wait_ms(200);
    return true;
}
