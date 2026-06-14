/************************************************************************************************/
/**
* @file               common.h
* @brief              CIU32F003 common definitions.
************************************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "ciu32f003_std.h"

typedef struct {
    unsigned char  b0 : 1;
    unsigned char  b1 : 1;
    unsigned char  b2 : 1;
    unsigned char  b3 : 1;
    unsigned char  b4 : 1;
    unsigned char  b5 : 1;
    unsigned char  b6 : 1;
    unsigned char  b7 : 1;
} bits_t;

typedef union
{
    bits_t        bits;
    unsigned char val;
} bit_field_t;

#define WDG_ReloadCounter std_iwdg_refresh()

void system_clock_config(void);
void iwdg_init(void);

#ifdef __cplusplus
}
#endif

#endif

