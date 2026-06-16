/************************************************************************************************/
/**
* @file               common.c
* @brief              CIU32F003 common initialization.
************************************************************************************************/

#include "common.h"

void system_clock_config(void)
{
    std_flash_set_latency(FLASH_LATENCY_1CLK);

    std_rcc_rch_enable();
    while (std_rcc_get_rch_ready() != RCC_CSR1_RCHRDY) {}

    std_rcc_set_sysclk_source(RCC_SYSCLK_SRC_RCH);
    while (std_rcc_get_sysclk_source() != RCC_SYSCLK_SRC_STATUS_RCH) {}

    std_rcc_set_ahbdiv(RCC_HCLK_DIV1);
    std_rcc_set_apbdiv(RCC_PCLK_DIV1);

    SystemCoreClock = RCH_VALUE;
}

void iwdg_init(void)
{
    std_rcc_rcl_enable();
    while (std_rcc_get_rcl_ready() != 1U) {}

    std_iwdg_write_access_enable();
    std_iwdg_set_overflow_period(IWDG_OVERFLOW_PERIOD_16384);
    std_iwdg_start();
}

