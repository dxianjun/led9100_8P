#ifndef BSP_INPUT_H
#define BSP_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ciu32l051_std.h"

void input_gpio_init(void);
uint8_t input_dc_in_is_active(void);
uint8_t input_bat_low_is_active(void);

#ifdef __cplusplus
}
#endif

#endif



