/************************************************************************************************/
/**
* @file               output_bsp.c
* @brief              CIU32F003 output for LED9100 8P.
************************************************************************************************/


#include "main.h"


#if 0
void output_gpio_init(void)
{
    std_gpio_init_t output_init_config = {0};

    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA);
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOB);

    /* PB4: Info_out (OD), low-battery notification output */
    output_init_config.pin = GPIO_PIN_4;
    output_init_config.mode = GPIO_MODE_OUTPUT;
    output_init_config.pull = GPIO_NOPULL;
    output_init_config.output_type = GPIO_OUTPUT_PUSHPULL;			// GPIO_OUTPUT_OPENDRAIN
    std_gpio_init(GPIOB, &output_init_config);
    std_gpio_reset_pin(GPIOB, GPIO_PIN_4);

    /* PB5: PWR_LOCK (OD), drive high when system is on */
    output_init_config.pin = GPIO_PIN_5;
    output_init_config.mode = GPIO_MODE_OUTPUT;
    output_init_config.pull = GPIO_NOPULL;
    output_init_config.output_type = GPIO_OUTPUT_OPENDRAIN;
    std_gpio_init(GPIOB, &output_init_config);
    std_gpio_reset_pin(GPIOB, GPIO_PIN_5);
	
}
#endif

void led_init(void)
{
    std_gpio_init_t led_gpio_init = {0};

    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOB);

    led_gpio_init.pin = LED1_PIN;
    led_gpio_init.mode = GPIO_MODE_OUTPUT;
    led_gpio_init.pull = GPIO_PULLUP;
    led_gpio_init.output_type = GPIO_OUTPUT_PUSHPULL;
    std_gpio_init(LED1_GPIO_PORT, &led_gpio_init);
}




