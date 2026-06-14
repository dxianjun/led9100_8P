/************************************************************************************************/
/**
* @file               exti_bsp.h
* @author             MCU Ecosystem Development Team
* @brief              EXTI BSP头文件
*                           
*
**************************************************************************************************
* @attention
* Copyright (c) CEC Huada Electronic Design Co.,Ltd. All rights reserved.
*
**************************************************************************************************
*/

/* 避免头文件重复引用 */
#ifndef EXTI_BSP_H
#define EXTI_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------includes--------------------------------------------*/
#include "ciu32l051_std.h"

/*------------------------------------------variables-------------------------------------------*/
extern uint32_t g_exti_gpio_flag;

/*-------------------------------------------define---------------------------------------------*/

/* BUTTON_USER定义 */
#define BUTTON_USER_PORT          GPIOC
#define BUTTON_USER_PIN           GPIO_PIN_13

#define BUTTON_USER_EXTI_PORT     EXTI_GPIOC
#define BUTTON_USER_EXTI_LINE     EXTI_LINE_GPIO_PIN13
    

/*------------------------------------------functions-------------------------------------------*/

void exti_gpio_init(void);
void exti_init_config(void);

void EXTI4_15_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTI_BSP_H */

