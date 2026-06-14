/************************************************************************************************/
/**
* @file               app_tcp.h
* @brief              UART command parser for LED9100 8P.
************************************************************************************************/

#ifndef APP_TCP_H
#define APP_TCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_service.h"
#include "usart_bsp.h"

void parse_cmd(uint8_t *buf, uint16_t len);
void tcp_hand(void);

#ifdef __cplusplus
}
#endif

#endif
