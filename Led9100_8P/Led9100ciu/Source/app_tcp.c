#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_tcp.h"

void parse_cmd(uint8_t *buf, uint16_t len)
{
    uint16_t value;
    char *cmd;

    if ((buf == NULL) || (len == 0U))
        {
        return;
        }

    buf[(len < BUFF_SIZE) ? len : (BUFF_SIZE - 1U)] = '\0';
    cmd = (char *)buf;

    if (strstr(cmd, "inpEn=1") != NULL)
        {
        app_set_input_print(1U);
        printf("inpEn=1\r\n");
        }
    else if (strstr(cmd, "inpEn=0") != NULL)
        {
        app_set_input_print(0U);
        printf("inpEn=0\r\n");
        }
    else if (strstr(cmd, "outEn=1") != NULL)
        {
        app_set_output_print(1U);
        printf("outEn=1\r\n");
        }
    else if (strstr(cmd, "outEn=0") != NULL)
        {
        app_set_output_print(0U);
        printf("outEn=0\r\n");
        }
    else if (strstr(cmd, "mode=2") != NULL)
        {
        app_set_mode(MODE_CCT);
        printf("mode=2\r\n");
        }
    else if (strstr(cmd, "mode=3") != NULL)
        {
        app_set_mode(MODE_DIRECT);
        printf("mode=3\r\n");
        }
    else if (strstr(cmd, "pwmLv1=") != NULL)
        {
        value = (uint16_t)strtoul(strstr(cmd, "pwmLv1=") + 7, NULL, 10);
        if (value > 100U)
            {
            value = 100U;
            }
        app_set_manual_level(1, (uint8_t)value);
        printf("pwmLv1=%u\r\n", value);
        }
	else if (strstr(cmd, "pwmLv2=") != NULL)
        {
        value = (uint16_t)strtoul(strstr(cmd, "pwmLv2=") + 7, NULL, 10);
        if (value > 100U)
            {
            value = 100U;
            }
        app_set_manual_level(2, (uint8_t)value);
        printf("pwmLv2=%u\r\n", value);
        }
    else
        {
        printf("cmd: inpEn=0/1,outEn=0/1,mode=2/3,pwmLv=0..100\r\n");
        }
}

void tcp_hand(void)
{
    if (rx_irq_flag1)
        {
        rx_irq_flag1 = 0U;
        parse_cmd(UartRxBuf1, (uint16_t)rx_len1);
        memset(UartRxBuf1, 0, BUFF_SIZE);
        rx_len1 = 0U;
        }
}
