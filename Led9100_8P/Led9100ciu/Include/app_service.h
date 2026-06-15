/************************************************************************************************/
/**
* @file               app_service.h
* @brief              LED9100 application service layer.
************************************************************************************************/

#ifndef APP_SERVICE_H
#define APP_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tim_bsp.h"

typedef struct
{
    unsigned int b0:1;
    unsigned int b1:1;
    unsigned int b2:1;
    unsigned int b3:1;
    unsigned int b4:1;
    unsigned int b5:1;
    unsigned int b6:1;
    unsigned int b7:1;
    unsigned int b8:1;
    unsigned int b9:1;
    unsigned int b10:1;
    unsigned int b11:1;
    unsigned int b12:1;
    unsigned int b13:1;
    unsigned int b14:1;
    unsigned int b15:1;
} bits16_t;

typedef union
{
    bits16_t bits;
    unsigned short val;
} bit16_field_t;

#define MODE_CCT        2U
#define MODE_DIRECT     3U
#define MODE_DEFAULT     MODE_DIRECT

extern bit16_field_t g_app_flag;
extern uint8_t uc_modecfg;

void sys_flg_init(void);
void app_init(void);
void app_task(void);
void pwm_update_isr(void);
void app_set_input_print(uint8_t enable);
void app_set_output_print(uint8_t enable);
void app_set_mode(uint8_t mode);
void app_set_manual_level(uint8_t ch,uint8_t level);
void user_serv(void);

#ifdef __cplusplus
}
#endif

#endif
