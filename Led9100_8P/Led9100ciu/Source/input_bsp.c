#include "input_bsp.h"

void input_gpio_init(void)
{
    std_gpio_init_t input_init_config = {0};

    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA);

    /* PA12: DC_IN (IPD), high means charging/DC present */
    input_init_config.pin = GPIO_PIN_12;
    input_init_config.mode = GPIO_MODE_INPUT;
    input_init_config.pull = GPIO_PULLDOWN;
    std_gpio_init(GPIOA, &input_init_config);

    /* PA4: BAT_DET (IPU), low means battery low */
    input_init_config.pin = GPIO_PIN_4;
    input_init_config.mode = GPIO_MODE_INPUT;
    input_init_config.pull = GPIO_PULLUP;
    std_gpio_init(GPIOA, &input_init_config);
}

uint8_t input_dc_in_is_active(void)
{
    return (uint8_t)std_gpio_get_input_pin(GPIOA, GPIO_PIN_12);
}

uint8_t input_bat_low_is_active(void)
{
    /* BAT_DET: low means battery low */
    return (uint8_t)(!std_gpio_get_input_pin(GPIOA, GPIO_PIN_4));
}



