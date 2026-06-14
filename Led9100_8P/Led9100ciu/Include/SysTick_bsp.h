/************************************************************************************************/
/**
* @file               SysTick_bsp.h
* @brief              SysTick and software timer BSP.
************************************************************************************************/

#ifndef SYSTICK_BSP_H
#define SYSTICK_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#define TTMR_CK_CH		0	
#define TTCPIN1         1
#define TTCPIN2         2
#define MAX1MS          3

#define TTMR_DLY        0
#define TTPWM_CH1       1
#define TTPWM_CH2       2
#define MAX10MS         3

extern volatile bit_field_t TimFlg;
extern unsigned short TimOut1mS[MAX1MS];
extern unsigned short TimOut10mS[MAX10MS];

#define Tim1ms_flg      TimFlg.bits.b0
#define Tim10ms_flg     TimFlg.bits.b1
#define Tim1s_flg       TimFlg.bits.b2
#define f_cal_ok        TimFlg.bits.b3

void SysTick_init(void);
void TimFlg_Hand(void);
void Delay_1ms(uint32_t nTime);
void delay_clk(uint32_t nclk);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif
