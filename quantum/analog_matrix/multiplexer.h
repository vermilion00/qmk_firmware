#pragma once

#include <stdint.h>

#ifdef MUX_PINS
volatile void set_mux_channel(uint8_t channel);
#else
#   define set_mux_channel(channel)
#endif
