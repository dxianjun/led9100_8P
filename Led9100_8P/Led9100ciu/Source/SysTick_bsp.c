/************************************************************************************************/
/**
* @file               SysTick_bsp.c
* @brief              SysTick and software timer BSP.
************************************************************************************************/

#include "SysTick_bsp.h"
#include "usart_bsp.h"
#include "output_bsp.h"

volatile bit_field_t TimFlg = {0};
unsigned short TimOut1mS[MAX1MS] = {0};
unsigned short TimOut10mS[MAX10MS] = {0};

static volatile uint32_t l_systick_125us = 0;
static volatile uint32_t l_systick_ms = 0;
static volatile uint32_t l_systick_s = 0;

static volatile uint32_t l_delay_ms = 0;

void SysTick_init(void)
{
    SysTick_Config(SystemCoreClock / 8000U);
}

void TimFlg_Hand(void)
{
    uint8_t i;

	if (Tim0_1ms_flg)
		{
		Tim0_1ms_flg = 0;
		}

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

void Delay_125us(__IO uint32_t nTime)
{
	uint32_t  nTimeH=0U;
	
		// 每100*125us清一次看门狗
		nTimeH = nTime/100;
		while (nTimeH--)
			{
			WDG_ReloadCounter;
			l_systick_125us = 0U;
			while (l_systick_125us < 100U);
			}
	
		// 余数和100ms以内的处理
		WDG_ReloadCounter;
		nTimeH = nTime%100;
		
		l_systick_125us = 0U;
		while (l_systick_125us < nTimeH);
}


/**
  * @brief   ms延时程序,1ms为一个单位
  * @param  
  *		@arg nTime: Delay_ms( 1 ) 则实现的延时为 1 * 1ms = 1ms
  * @retval  无
  */

void Delay_1ms(__IO uint32_t nTime)
{
	uint32_t  nTimeH=0U;
	// uint32_t  l_Try;
	
	// 每100ms清一次看门狗
	nTimeH = nTime/100;
	while (nTimeH--)
		{
		WDG_ReloadCounter;
		l_delay_ms = 0U;
		while (l_delay_ms < 100U);
		}

	// 余数和100ms以内的处理
	WDG_ReloadCounter;
	nTimeH = nTime%100;
	
	l_delay_ms = 0U;
	while (l_delay_ms < nTimeH);
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
	l_systick_125us++;
	Tim0_1ms_flg = 1U;
	LED1_TOGGLE();
	
	if(l_systick_125us % 8 ==0)
		{
		Tim1ms_flg=1U;
		
		// tick 
		l_systick_ms++;
    	l_delay_ms++;
		
		if(l_systick_125us % 80 ==0)
			{
			Tim10ms_flg = 1U;
			
			if(l_systick_125us % 8000 == 0)
				{
				Tim1s_flg = 1U;
				l_systick_s++;
				
				if (l_systick_125us >= 40000)
					{
					l_systick_125us = 0;
					}
				}
			}
		}
}
