/************************************************************************************************/
/**
* @file               main.c
* @brief              LED9100 8P PWM converter firmware.
************************************************************************************************/

#include <string.h>
#include "main.h"

#define HARDWAREVERSION "LED9100_8P_V1.00"

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

	app_init();
	tim3_gpio_init();

	uc_sel_ch=1U;
	tim3_input_init();
	bsp_tim3_capture_start();

	
	tim1_gpio_init();
	tim1_output_init();
	tim1_output_start();

	iwdg_init();

    while (1)
        {
        WDG_ReloadCounter;
        TimFlg_Hand();
        user_serv();
        }
}

