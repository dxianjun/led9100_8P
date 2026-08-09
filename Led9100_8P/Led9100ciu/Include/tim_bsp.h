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

// 输入占空比统一量化为12000份。
#define PWM_SCALE       12000U
// TIM1计数时钟为48MHz：3000 ticks对应16kHz，16000 ticks对应3kHz，24000 ticks对应2kHz。
#define ARR_16K         3000U
#define ARR_2K         	24000U
#define ARR_3K			16000U

// DUTY_MAX_NO_ADJ：0表示限制最大逻辑占空比；1表示取消高端限制，允许逻辑脉宽达到整个周期。
#define DUTY_MAX_NO_ADJ         0

#if ((DUTY_MAX_NO_ADJ != 0) && (DUTY_MAX_NO_ADJ != 1))
#error "DUTY_MAX_NO_ADJ must be 0 or 1"
#endif

#if (DUTY_MAX_NO_ADJ == 1)
// 取消高端限制时，输入亮度和逻辑脉宽均允许达到100%。
#define CONST_INPUT_MAX         PWM_SCALE
#else
// 启用高端限制时，只允许选择90%或95%的最大逻辑输出。
#define MAX_OUTPUT_PERCENT      90U

#if (MAX_OUTPUT_PERCENT == 90U)
#define CONST_INPUT_MAX         10800U
#elif (MAX_OUTPUT_PERCENT == 95U)
#define CONST_INPUT_MAX         11400U
#else
#error "MAX_OUTPUT_PERCENT must be 90 or 95"
#endif
#endif

// 定频模式选择：FIX_16K、FIX_3K、FIX_2K分别选择16kHz、3kHz、2kHz。
// 三者均为0时，根据亮度曲线在3000~24000 ticks之间动态变频。
#define FIX_16K                 0
#define FIX_2K                  0
#define FIX_3K                  1

#if (((FIX_16K != 0) && (FIX_16K != 1)) || \
     ((FIX_3K != 0) && (FIX_3K != 1)) || \
     ((FIX_2K != 0) && (FIX_2K != 1)))
#error "FIX_16K, FIX_3K and FIX_2K must be 0 or 1"
#endif

#if ((FIX_16K + FIX_3K + FIX_2K) > 1)
#error "Only one fixed PWM frequency may be enabled"
#endif

#if (FIX_16K == 1)
#define FIXED_PERIOD_TICKS ARR_16K
#define MAX_BRIGHT_PERIOD_TICKS	ARR_16K		// 最大亮度时的周期ARR_16K
#elif (FIX_3K == 1)
#define FIXED_PERIOD_TICKS ARR_3K
#define MAX_BRIGHT_PERIOD_TICKS	ARR_3K		// 最大亮度时的周期ARR_3K
#elif (FIX_2K == 1)
#define FIXED_PERIOD_TICKS ARR_2K
#define MAX_BRIGHT_PERIOD_TICKS	ARR_2K		// 最大亮度时的周期ARR_2K
#else
#define FIXED_PERIOD_TICKS ARR_2K
#define MAX_BRIGHT_PERIOD_TICKS	ARR_16K		// 最大亮度时的周期ARR_16K
#endif

#if (DUTY_MAX_NO_ADJ == 0)
// 最大占空比保护随当前定频周期缩放；动态变频模式保持原16kHz基准保护值。
#define DUTY_ADJ_OUT            ((MAX_BRIGHT_PERIOD_TICKS * (100U - MAX_OUTPUT_PERCENT)) / 100U)
#endif

// 1：当前硬件为TSSOP20调试板；0：当前硬件为8脚版本。
#define TSSOP20                 0
#if (TSSOP20 == 1)
// CH1/CH2仅用于TSSOP20调试波形；CH3/CH4始终作为正式PWM输出。
#define DEBUG_PWM_OUTPUT        1
// TSSOP20调试板具备状态LED。
#define STATUS_LED_ENABLE       0
// TSSOP20使用PA3/PA4作为UART1，不占用SWD引脚。
#define UART1_USE_SWD_PINS      0
#else
// 8脚硬件不具备CH1/CH2调试输出和状态LED能力，UART1复用SWD引脚。
#define DEBUG_PWM_OUTPUT        0
#define STATUS_LED_ENABLE       0
#define UART1_USE_SWD_PINS      1
#endif

#if ((UART1_USE_SWD_PINS != 0) && (UART1_USE_SWD_PINS != 1))
#error "UART1_USE_SWD_PINS must be 0 or 1"
#endif

#define CHIP_9100   0
#define CHIP_6800   1
#define CHIP_9101   2

#define CHIP_VER    CHIP_9100

// LED9101需要在输入由全低唤醒时先输出唤醒脉冲。
#if (CHIP_VER == CHIP_9101)
#define WAKE_UP_CHX              1
#else
#define WAKE_UP_CHX              0
#endif

// MIN_PULSE：非零逻辑脉宽的最小tick数。
// DUTY_DELT：写CCR时增加的死区补偿tick数，使实际高脉宽等于逻辑脉宽。
// DT_SET：TIM1硬件死区寄存器设置值。
#if (CHIP_VER == CHIP_9100)
#define MIN_PULSE   20U		// davidd 20260721, 最小脉冲改回416.7ns
#define DUTY_DELT   76U
#define DT_SET      76U
#elif (CHIP_VER == CHIP_9101)
#define MIN_PULSE   2U
#define DUTY_DELT   22U
#define DT_SET      22U
#else   // CHIP_6800
#define MIN_PULSE   2U
#define DUTY_DELT   22U
#define DT_SET      22U
#endif

extern volatile uint8_t uc_sel_ch;
extern uint16_t tim1_period_ticks;

void tim1_gpio_init(void);
void tim1_output_init(void);
void tim1_output_start(void);
void tim1_apply_output(uint16_t period_ticks, uint16_t pulse1_ticks, uint16_t pulse2_ticks);
//void pwm_output_apply(uint16_t out1, uint16_t out2, uint16_t period);

void tim3_gpio_init(void);
void tim3_input_init(void);
void tim3_nvic_init(void);

void bsp_tim3_capture_start(void);
void Capture_switch(uint8_t channel);

void input_service_process(void);
uint8_t input_get(uint8_t channel, uint16_t *duty);

void TIM1_BRK_UP_TRG_COM_IRQHandler(void);
void TIM3_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
