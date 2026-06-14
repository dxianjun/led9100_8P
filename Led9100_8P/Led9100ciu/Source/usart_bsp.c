/************************************************************************************************/
/**
* @file               usart_bsp.c
* @brief              UART1 debug port implementation.
************************************************************************************************/

#include <stdio.h>
#include "usart_bsp.h"
#include "tim_bsp.h"

uint8_t UartRxBuf1[BUFF_SIZE] = {0};
uint8_t UartRxBuf2[BUFF_SIZE] = {0};

volatile uint32_t rx_ptr1 = 0;
volatile uint32_t rx_len1 = 0;
volatile uint8_t rx_irq_flag1 = 0;

#define UART1_RX_IDLE_MS        10U

void uart_gpio_init(void)
{
    std_gpio_init_t uart_gpio = {0};

    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA);

    /* UART1: PA3=TX, PA4=RX, PB4/PB5 reserved and not used. */
    uart_gpio.pin = GPIO_PIN_3 | GPIO_PIN_4;
    uart_gpio.mode = GPIO_MODE_ALTERNATE;
    uart_gpio.pull = GPIO_PULLUP;
    uart_gpio.output_type = GPIO_OUTPUT_PUSHPULL;
    uart_gpio.alternate = GPIO_AF1_UART1;
    std_gpio_init(GPIOA, &uart_gpio);
}

void uart_init(void)
{
    std_uart_init_t uart_init = {0};

    std_rcc_apb2_clk_enable(RCC_PERIPH_CLK_UART1);

    uart_init.baudrate = 115200;
    uart_init.direction = UART_DIRECTION_SEND_RECEIVE;
    uart_init.parity = UART_PARITY_NONE;
    uart_init.stopbits = UART_STOPBITS_1;
    uart_init.wordlength = UART_WORDLENGTH_8BITS;
    std_uart_init(UART1, &uart_init);
    std_uart_enable(UART1);
}

void usart_init_int(void)
{
    std_uart_cr1_interrupt_enable(UART1, UART_CR1_INTERRUPT_RXNE);
    NVIC_SetPriority(UART1_IRQn, NVIC_PRIO_1);
    NVIC_EnableIRQ(UART1_IRQn);
}

#if defined(__GNUC__)
int _write(int fd, char* ptr, int len)
{
    int i;
    (void)fd;

    for (i = 0; i < len; i++)
        {
        while (!std_uart_get_flag(UART1, UART_FLAG_TXE)) { WDG_ReloadCounter; }
        std_uart_tx_write_data(UART1, (uint32_t)ptr[i]);
        while (!std_uart_get_flag(UART1, UART_FLAG_TC)) { WDG_ReloadCounter; }
        }

    return len;
}
#else
int fputc(int ch, FILE *f)
{
    (void)f;

    while (!std_uart_get_flag(UART1, UART_FLAG_TXE)) { WDG_ReloadCounter; }
    std_uart_tx_write_data(UART1, (uint32_t)ch);
    while (!std_uart_get_flag(UART1, UART_FLAG_TC)) { WDG_ReloadCounter; }

    return ch;
}
#endif

void UART1_IRQHandler(void)
{
    uint8_t dat;

    if ((std_uart_get_cr1_interrupt_enable(UART1, UART_CR1_INTERRUPT_RXNE) ||
         std_uart_get_cr3_interrupt_err_enable(UART1)) &&
        std_uart_get_flag(UART1, UART_FLAG_ORE))
        {
        std_uart_clear_flag(UART1, UART_CLEAR_ORE);
        }

    if (std_uart_get_cr1_interrupt_enable(UART1, UART_CR1_INTERRUPT_RXNE))
        {
        if (std_uart_get_flag(UART1, UART_FLAG_RXNE))
            {
            dat = (uint8_t)std_uart_rx_read_data(UART1);
            UartRxBuf1[rx_ptr1] = dat;

            if (rx_ptr1 < (BUFF_SIZE - 1U))
                {
                rx_ptr1++;
                }

            TimOut1mS[TTCPIN1] = 0U;

            if ((dat == '$') || (dat == '\r') || (dat == '\n') || (rx_ptr1 >= (BUFF_SIZE - 1U)))
                {
                rx_len1 = rx_ptr1;
                rx_ptr1 = 0U;
                rx_irq_flag1 = 1U;
                }
            }
        }
}

void uart1_rx_idle_check(void)
{
    if ((rx_ptr1 > 0U) && (TimOut1mS[TTCPIN1] >= UART1_RX_IDLE_MS))
        {
        rx_len1 = rx_ptr1;
        rx_ptr1 = 0U;
        rx_irq_flag1 = 1U;
        TimOut1mS[TTCPIN1] = 0U;
        }
}
