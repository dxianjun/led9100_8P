/************************************************************************************************/
/**
* @file               tim_bsp.h
* @brief              CIU32F003 timer and PWM BSP for LED9100 8P.
************************************************************************************************/

#ifndef TIM_BSP_H
#define TIM_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "SysTick_bsp.h"

#define PWM_SCALE       12000U
#define ARR_16K         3000U
#define ARR_2K        	24000U

#define FIX_16K			0

#define CHIP_9100	0
#define CHIP_6800	1
#define CHIP_9101	2


#define	CHIP_VER	CHIP_9101

#if (CHIP_VER==CHIP_9101)
#define WAKE_UP_CHX		1	
#else
#define WAKE_UP_CHX		0
#endif


#if (CHIP_VER==CHIP_9100)	// 9100
#define MIN_PULSE	30U		// VerA定义为11，9+37=46,    实际输出的脉宽是 340ns(被死区砍成的380ns)
#define DUTY_DELT	76U		// 加了死区后，在输出占空时，做一下补偿38(DT_SET)； (47-38)/1500*62.5us=  333ns
#define DT_SET		76U		// VerA 38-1.583us
#elif (CHIP_VER==CHIP_9101)		// 9101
#define MIN_PULSE	4U		// VerA定义为11，9+37=46,    实际输出的脉宽是 340ns(被死区砍成的380ns)
#define DUTY_DELT	20U		// 加了死区后，在输出占空时，做一下补偿38(DT_SET)； (47-38)/1500*62.5us=  333ns
#define DT_SET		22U		// VerA 38-1.583us
#else		// 6800
// DT_SET只和CKD(TIM_ClockDivision)有关, 因此不变
// 输入最小限定在3/30 = 0.1 = 0.1%, 只要低于0.1%，强制输出最小脉冲；因此特意将DUTY_DELT由11改为9；
#define MIN_PULSE	4U		// VerB定义为3,  3+9=12, 实际输出的脉宽是41.7ns， 
#define DUTY_DELT	20U		// 加了死区后，在输出占空时，做一下补偿(应该是11, 调整到了9)；(12-11)/3000*125us=41.7ns
#define DT_SET		22U		// VerB 11- 458.3ns, 11/24=0.4583 = 458.3ns; 最小可以到440ns, 这个颗芯片做不了这么细
#endif



extern uint8_t uc_sel_ch;
extern uint16_t tim1_period_ticks;

void tim1_gpio_init(void);
void tim1_output_init(void);
void tim1_output_start(void);
void tim1_apply_output(uint16_t period_ticks, uint16_t pulse1_ticks, uint16_t pulse2_ticks);
//void pwm_output_apply(uint16_t out1, uint16_t out2, uint16_t period);


void tim3_gpio_init(void);
void tim3_input_init(void);
void bsp_tim3_capture_start(void);	
void Capture_switch(uint8_t uc_sel_ch);



void input_timeout_check(void);
uint8_t input_get(uint8_t channel, uint16_t *duty);

void TIM1_BRK_UP_TRG_COM_IRQHandler(void);
void TIM3_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
