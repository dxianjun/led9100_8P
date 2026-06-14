/************************************************************************************************/
/**
* @file               usart_bsp.h
* @brief              UART1 debug port for LED9100 8P.
************************************************************************************************/

#ifndef USART_BSP_H
#define USART_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#define BUFF_SIZE 64

extern uint8_t UartRxBuf1[BUFF_SIZE];
extern uint8_t UartRxBuf2[BUFF_SIZE];
extern volatile uint32_t rx_ptr1;
extern volatile uint32_t rx_len1;
extern volatile uint8_t rx_irq_flag1;

void uart_gpio_init(void);
void uart_init(void);
void usart_init_int(void);
void uart1_rx_idle_check(void);
void UART1_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
