/************************************************************************************************/
/**
* @file               main.c
* @brief              LED9100 8P PWM converter firmware.
************************************************************************************************/

#include <string.h>
#include "main.h"

#if (CHIP_VER==CHIP_9100)
#define HARDWAREVERSION						"LED9100_8P_V1.01"
#elif (CHIP_VER==CHIP_9101)
#define HARDWAREVERSION						"LED9101_8P_V1.01"
#else
#define HARDWAREVERSION						"LED6800_8P_V1.01"
#endif

uint8_t FirmVers[20] = "V1.00_26.01.01";
static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

static void build_FirmVers(void)
{
    int month;
    int day;
    int year;
    uint8_t idx;
    char s_month[5];
    char s_date[12];

    sprintf(s_date, __DATE__);
    sscanf(s_date, "%s %d %d", s_month, &day, &year);

    month = (int)((strstr(month_names, s_month) - month_names) / 3) + 1;
    year = year - 2000;

    idx = 6;
    FirmVers[idx] = (uint8_t)(year / 10 + '0');
    FirmVers[idx + 1] = (uint8_t)(year % 10 + '0');
    FirmVers[idx + 3] = (uint8_t)(month / 10 + '0');
    FirmVers[idx + 4] = (uint8_t)(month % 10 + '0');
    FirmVers[idx + 6] = (uint8_t)(day / 10 + '0');
    FirmVers[idx + 7] = (uint8_t)(day % 10 + '0');

    printf("Soft_Vers: %s\r\n", FirmVers);
}


uint8_t uc_rstflg;
uint8_t rcc_get_rstflg(void)
{
	return ((uint8_t)((RCC->CSR2 & RCC_RESET_FLAG_ALL)>>24));
}

int main(void)
{
	system_clock_config();
	SysTick_init();

	uart_gpio_init();
	uart_init();
	usart_init_int();

	printf("\r\n*********%s********** \r\n", HARDWAREVERSION);
	printf("build(" __DATE__ " - " __TIME__ ") \r\n");
	build_FirmVers();

	// 复位标志
	uc_rstflg = rcc_get_rstflg();
	if (uc_rstflg)
		{
		/** 
		* @brief  获取复位标志
		* @param  reset_flag 指定要获取的复位标志
		*             @arg RCC_RESET_FLAG_LOCKUP
		*             @arg RCC_RESET_FLAG_OBL   
		*             @arg RCC_RESET_FLAG_NRST  
		*             @arg RCC_RESET_FLAG_PMU   
		*             @arg RCC_RESET_FLAG_SW    
		*             @arg RCC_RESET_FLAG_IWDG  
		*             @arg RCC_RESET_FLAG_WWDG  
		*             @arg RCC_RESET_FLAG_LPM   
		*             @arg RCC_RESET_FLAG_ALL   
		* @retval bool 返回逻辑表达式的判断结果
		*             @arg true：表示指定的复位标志置位
		*             @arg false：表示指定的复位标志未置位
		*/
		printf("Rst flg: %02x\n", uc_rstflg);
		
		if (std_rcc_get_reset_flag(RCC_RESET_FLAG_LOCKUP))
			{
			printf("LOCKUP reset\n");	// 
			}
		
		#if 0	// 003没有这个标记
		if (std_rcc_get_reset_flag(RCC_RESET_FLAG_OBL))
			{
			printf("OBL reset\n");		// 选项字加载复位
			}
		#endif
		
		if (std_rcc_get_reset_flag(RCC_RESET_FLAG_NRST))
			{
			printf("NRST reset\n");		// 复位脚复位
			}
		
		if (std_rcc_get_reset_flag(RCC_RESET_FLAG_PMU))
			{
			printf("PMU reset\n");		// 上电复位
			}

		if (std_rcc_get_reset_flag(RCC_RESET_FLAG_SW))
			{
			printf("SW reset\n");		// 软件复位, 执行了NVIC_SystemReset();    // 系统复位, 烧录后就是这个
			}
		
		if (std_rcc_get_reset_flag(RCC_RESET_FLAG_IWDG))
			{
			printf("IWDG reset\n");		// 看门狗复位
			}
		
		#if 0	// 003没有这个标记
		if (std_rcc_get_reset_flag(RCC_RESET_FLAG_WWDG))
			{
			printf("WWDG reset\n");		// 窗口看门狗复位
			}
		#endif
		
		if (std_rcc_get_reset_flag(RCC_RESET_FLAG_LPM))
			{
			printf("LPM reset\n");		// 低功耗复位标志，非法模式复位
			}
		
		std_rcc_clear_reset_flags();
		}
	
	app_init();
	tim3_gpio_init();

	uc_sel_ch=1U;
	tim3_input_init();
	tim3_nvic_init();
	bsp_tim3_capture_start();
	
	tim1_gpio_init();
	tim1_output_init();
	tim1_output_start();
	#if (STATUS_LED_ENABLE == 1)
	led_init();
	#endif
	// iwdg_init();

	while (1)
		{
		WDG_ReloadCounter;
		TimFlg_Hand();
		user_serv();
		}
}

