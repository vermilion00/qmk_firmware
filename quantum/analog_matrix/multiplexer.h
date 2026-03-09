#pragma once

#include <stdint.h>

#if MUX_CHANNELS > 1
volatile void set_mux_channel(uint8_t channel);
#else
#   define set_mux_channel(channel)
#endif
