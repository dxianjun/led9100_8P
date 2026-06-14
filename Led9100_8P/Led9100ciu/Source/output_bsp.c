#include "output_bsp.h"

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

    /* PA10: BAT_EN (PP), discharge-state notify to charger board */
    output_init_config.pin = GPIO_PIN_10;
    output_init_config.mode = GPIO_MODE_OUTPUT;
    output_init_config.pull = GPIO_NOPULL;
    output_init_config.output_type = GPIO_OUTPUT_PUSHPULL;
    std_gpio_init(GPIOA, &output_init_config);
	std_gpio_set_pin(GPIOA, GPIO_PIN_10);		// BAT_EN上电后，就一直拉高，保持电池电量显示，直到手柄合上断电 davidd 20260609
    // std_gpio_reset_pin(GPIOA, GPIO_PIN_10);

    /* PB5: PWR_LOCK (OD), drive high when system is on */
    output_init_config.pin = GPIO_PIN_5;
    output_init_config.mode = GPIO_MODE_OUTPUT;
    output_init_config.pull = GPIO_NOPULL;
    output_init_config.output_type = GPIO_OUTPUT_OPENDRAIN;
    std_gpio_init(GPIOB, &output_init_config);
    std_gpio_reset_pin(GPIOB, GPIO_PIN_5);
	
    /* PA2: BT_WAKEUP (PP), falling edge wakes BT from standby; keep high in idle */
    output_init_config.pin = GPIO_PIN_2;
    output_init_config.mode = GPIO_MODE_OUTPUT;
    output_init_config.pull = GPIO_NOPULL;
    output_init_config.output_type = GPIO_OUTPUT_PUSHPULL;
    std_gpio_init(GPIOA, &output_init_config);
    std_gpio_set_pin(GPIOA, GPIO_PIN_2);
	
    /* PB11/PB13: wheel direction outputs (ODH/ODL) */
    output_init_config.pin = GPIO_PIN_11 | GPIO_PIN_13;
    output_init_config.mode = GPIO_MODE_OUTPUT;
    output_init_config.pull = GPIO_NOPULL;
    output_init_config.output_type = GPIO_OUTPUT_OPENDRAIN;
    std_gpio_init(GPIOB, &output_init_config);
    std_gpio_set_pin(GPIOB, GPIO_PIN_11);   /* left wheel forward */
    std_gpio_reset_pin(GPIOB, GPIO_PIN_13); /* right wheel forward */
}

void output_info_set(uint8_t active)
{
    if (active) { std_gpio_set_pin(GPIOB, GPIO_PIN_4); }
    else { std_gpio_reset_pin(GPIOB, GPIO_PIN_4); }
}

void output_bat_en_set(uint8_t active)
{
    if (active) { std_gpio_set_pin(GPIOA, GPIO_PIN_10); }
    else { std_gpio_reset_pin(GPIOA, GPIO_PIN_10); }
}

void output_pwr_lock_set(uint8_t active)
{
    if (active) { std_gpio_set_pin(GPIOB, GPIO_PIN_5); }
    else { std_gpio_reset_pin(GPIOB, GPIO_PIN_5); }
}

void output_bt_wakeup_set(uint8_t level)
{
    if (level) { std_gpio_set_pin(GPIOA, GPIO_PIN_2); }
    else { std_gpio_reset_pin(GPIOA, GPIO_PIN_2); }
}

void output_bt_wakeup_pulse(void)
{
    output_bt_wakeup_set(0);
    std_delayms(10);
    output_bt_wakeup_set(1);
    std_delayms(10);	//std_delayms
}

void output_left_wheel_dir_set(bsp_wheel_dir_t dir)
{
    if (dir == BSP_WHEEL_DIR_FORWARD) 
		{ 
		std_gpio_set_pin(GPIOB, GPIO_PIN_11); 
		}
    else 
		{ 
		std_gpio_reset_pin(GPIOB, GPIO_PIN_11); 
		}
}

void output_right_wheel_dir_set(bsp_wheel_dir_t dir)
{
    if (dir == BSP_WHEEL_DIR_FORWARD) 
		{ 
		std_gpio_reset_pin(GPIOB, GPIO_PIN_13); 
		}
    else 
		{ 
		std_gpio_set_pin(GPIOB, GPIO_PIN_13); 
		}
}

void output_wheel_dir_forward(void)
{
    output_left_wheel_dir_set(BSP_WHEEL_DIR_FORWARD);
    output_right_wheel_dir_set(BSP_WHEEL_DIR_FORWARD);
}

void output_wheel_dir_turn_left(void)
{
    output_left_wheel_dir_set(BSP_WHEEL_DIR_BACKWARD);
    output_right_wheel_dir_set(BSP_WHEEL_DIR_FORWARD);
}

void output_wheel_dir_turn_right(void)
{
    output_left_wheel_dir_set(BSP_WHEEL_DIR_FORWARD);
    output_right_wheel_dir_set(BSP_WHEEL_DIR_BACKWARD);
}

void output_wheel_pwm_set(uint16_t left_pwm, uint16_t right_pwm)
{
    std_tim_set_ccx_value(TIM4, TIM_CHANNEL_1, left_pwm);
    std_tim_set_ccx_value(TIM5, TIM_CHANNEL_1, right_pwm);
}

// 吸尘/吸水
void output_fan_pwm_set(uint16_t pwm)
{
    std_tim_set_ccx_value(TIM4, TIM_CHANNEL_4, pwm);
}

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




