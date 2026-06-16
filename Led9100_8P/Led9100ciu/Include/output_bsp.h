#ifndef BSP_OUTPUT_H
#define BSP_OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#define LED1_GPIO_PORT            GPIOB
#define LED1_PIN                  GPIO_PIN_1
#define LED1_TOGGLE()             std_gpio_toggle_pin(LED1_GPIO_PORT, LED1_PIN)

void led_init(void);

void output_gpio_init(void);


#ifdef __cplusplus
}
#endif

#endif




