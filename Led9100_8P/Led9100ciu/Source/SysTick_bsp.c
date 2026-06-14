/************************************************************************************************/
/**
* @file               SysTick_bsp.c
* @brief              SysTick and software timer BSP.
************************************************************************************************/

#include "SysTick_bsp.h"
#include "usart_bsp.h"

volatile bit_field_t TimFlg = {0};
unsigned short TimOut1mS[MAX1MS] = {0};
unsigned short TimOut10mS[MAX10MS] = {0};

static volatile uint32_t l_systick_ms = 0;
static volatile uint32_t l_delay_ms = 0;

void SysTick_init(void)
{
    SysTick_Config(SystemCoreClock / 1000U);
}

void TimFlg_Hand(void)
{
    uint8_t i;

    if (Tim1ms_flg)
        {
        Tim1ms_flg = 0U;
        for (i = 0U; i < MAX1MS; i++)
            {
            if (TimOut1mS[i] < 0xFFFFU)
                {
                TimOut1mS[i]++;
                }
            }
        uart1_rx_idle_check();
        }

    if (Tim10ms_flg)
        {
        Tim10ms_flg = 0U;
        for (i = 0U; i < MAX10MS; i++)
            {
            if (TimOut10mS[i] < 0xFFFFU)
                {
                TimOut10mS[i]++;
                }
            }
        }
}

void Delay_1ms(uint32_t nTime)
{
    l_delay_ms = 0U;
    while (l_delay_ms < nTime)
        {
        WDG_ReloadCounter;
        }
}

void delay_clk(uint32_t nclk)
{
    while (nclk--)
        {
        __NOP();
        }
}

void SysTick_Handler(void)
{
    l_systick_ms++;
    l_delay_ms++;
    Tim1ms_flg = 1U;

    if ((l_systick_ms % 10U) == 0U)
        {
        Tim10ms_flg = 1U;
        }

    if ((l_systick_ms % 1000U) == 0U)
        {
        Tim1s_flg = 1U;
        }
}
