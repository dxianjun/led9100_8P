/************************************************************************************************/
/**
* @file               rtc_bsp.h
* @author             MCU Ecosystem Development Team
* @brief              RTC BSP头文件
*                           
*
**************************************************************************************************
* @attention
* Copyright (c) CEC Huada Electronic Design Co.,Ltd. All rights reserved.
*
**************************************************************************************************
*/

/* 避免头文件重复引用 */
#ifndef RTC_BSP_H
#define RTC_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------includes--------------------------------------------*/
#include "ciu32l051_std.h"

/*--------------------------------------------variables-----------------------------------------*/
extern __IO uint8_t g_timestamp_flag;

/*------------------------------------------functions-------------------------------------------*/
void rtc_clock_config(void);

void RTC_TAMP_IRQHandler(void);
void bsp_rtc_datetime_config(void);
void bsp_rtc_tamp_in_config(void);
void bsp_rtc_get_timestamp(uint8_t *timestamp_date, uint8_t *timestamp_time);

#ifdef __cplusplus
}
#endif

#endif /* RTC_BSP_H */

