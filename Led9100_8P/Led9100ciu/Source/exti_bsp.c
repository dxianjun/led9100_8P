/************************************************************************************************/
/**
* @file               exti_bsp.c
* @author             MCU Ecosystem Development Team
* @brief              EXTI BSP驱动函数，实现EXTI功能配置。
*
*
**************************************************************************************************
* @attention
* Copyright (c) CEC Huada Electronic Design Co.,Ltd. All rights reserved.
*
**************************************************************************************************
*/

/*------------------------------------------includes--------------------------------------------*/
#include "exti_bsp.h"

/*------------------------------------------variables-------------------------------------------*/

uint32_t g_exti_gpio_flag = 0;

/*------------------------------------------functions-------------------------------------------*/


/**
* @brief  EXTI初始化
* @retval 无
*/
void exti_gpio_init(void)
{
    std_gpio_init_t button_init_config = {0};

    /* 使能BUTTON_USER对应的GPIO时钟 */
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOB);
	std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOC);
	std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOD);

    /* 配置BUTTON_USER的GPIO */
    button_init_config.pin = BUTTON_USER_PIN;
    button_init_config.mode = GPIO_MODE_INPUT;
    button_init_config.pull = GPIO_PULLUP;
    std_gpio_init(BUTTON_USER_PORT, &button_init_config);
}

void exti_init_config(void)
{
    std_exti_init_t exti_init_config = {0};

    /* 配置BUTTON_USER的EXTI */
    exti_init_config.mode = EXTI_MODE_INTERRUPT;
    exti_init_config.trigger = EXTI_TRIGGER_FALLING;	// EXTI_TRIGGER_RISING_FALLING;	

    exti_init_config.line_id = BUTTON_USER_EXTI_LINE;
    exti_init_config.gpio_id = BUTTON_USER_EXTI_PORT;
    std_exti_init(&exti_init_config);


    /* 配置中断优先级 */
    NVIC_SetPriority(EXTI4_15_IRQn, NVIC_PRIO_3); 	// 4-15, // 数值越大，优先级越低
    /* 使能中断 */
    NVIC_EnableIRQ(EXTI4_15_IRQn);

#if 0
    /* 配置 exit_0 的GPIO */
    button_init_config.pin = GPIO_PIN_0;
    button_init_config.mode = GPIO_MODE_INPUT;
    button_init_config.pull = GPIO_PULLUP;
    std_gpio_init(GPIOD, &button_init_config);
    
    /* 配置 exit_0 的EXTI */
    exti_init_config.mode = EXTI_MODE_INTERRUPT;
    exti_init_config.trigger = EXTI_TRIGGER_FALLING;

    exti_init_config.line_id = EXTI_LINE_GPIO_PIN0;
    exti_init_config.gpio_id = EXTI_GPIOD;
    std_exti_init(&exti_init_config);

    /* 配置中断优先级 */
    NVIC_SetPriority(EXTI0_1_IRQn, NVIC_PRIO_3); 	// 0-1
    /* 使能中断 */
    NVIC_EnableIRQ(EXTI0_1_IRQn);
#endif
}


/**
* @brief  EXTI4_15中断服务函数
* @retval 无
*/
void EXTI4_15_IRQHandler(void)
{
	/* 读取EXTI通道中断挂起状态 */
	if (std_exti_get_pending_status(EXTI_LINE_GPIO_PIN13))
		{
		/* 清除EXTI通道中断挂起状态 */
		std_exti_clear_pending(EXTI_LINE_GPIO_PIN13);
		g_exti_gpio_flag = 1;
		}
}

#if 0
void Exit_Hand(void)
{
	/* 每按一次按键触发EXTI中断，LED1切换一次亮灭 */
	if (g_exti_gpio_flag == 1)
		{
		LED1_TOGGLE();
		g_exti_gpio_flag = 0;
		}
}
#endif


