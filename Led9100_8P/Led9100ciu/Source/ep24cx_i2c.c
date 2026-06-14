#include <string.h>

#include "ep24cx_i2c.h"
#include "i2c_driver.h"
#include "tim_bsp.h"

#define ADDR_EP24Cxx    0xA0
#define BYTE_PAGE       32
#define DLY_WRITE       10

#define LOBYTE(v1) ((uint8_t)((v1) & 0xFF))
#define HIBYTE(v1) ((uint8_t)((v1) >> 8))
#define MAKEWORD(v1,v2) ((((uint16_t)(v1)) << 8) + (uint16_t)(v2))

SysData gSysData;

static uint8_t EpromData[DATA_EPROM_CNT] = {0};
static uint16_t ui_sys_flg = 0;
static uint16_t ui_custcode = 0;

bool ReadEpData(uint16_t raddr, uint8_t *rdata, uint16_t uNum)
{
    uint16_t page, uTail, uOffset, i;
    bool bRet;
    uint8_t *Data = rdata;

    uOffset = raddr % BYTE_PAGE;
    page = (uNum + uOffset) / BYTE_PAGE;
    uTail = (page > 0) ? (uNum + uOffset - page * BYTE_PAGE) : uNum;

    if (page == 0)
    {
        return I2C_PolledMasterRead(ADDR_EP24Cxx, (uint8_t)raddr, Data, (uint8_t)uTail);
    }

    bRet = I2C_PolledMasterRead(ADDR_EP24Cxx, (uint8_t)raddr, Data, (uint8_t)(BYTE_PAGE - uOffset));
    if (!bRet) { return false; }

    Delay_1ms(DLY_WRITE);
    raddr = (uint16_t)(raddr + BYTE_PAGE - uOffset);
    Data = Data + BYTE_PAGE - uOffset;

    for (i = 0; i < page - 1; i++)
    {
        bRet = I2C_PolledMasterRead(ADDR_EP24Cxx, (uint8_t)(raddr + i * BYTE_PAGE), Data + i * BYTE_PAGE, (uint8_t)BYTE_PAGE);
        if (!bRet) { return false; }
        Delay_1ms(DLY_WRITE);
    }

    if (uTail > 0)
    {
        return I2C_PolledMasterRead(ADDR_EP24Cxx, (uint8_t)(raddr + (page - 1) * BYTE_PAGE), Data + (page - 1) * BYTE_PAGE, (uint8_t)uTail);
    }

    return true;
}

bool WriteEpData(uint16_t waddr, uint8_t *wdata, uint16_t uNum)
{
    uint16_t page, uTail, i, uOffset;
    bool bRet;
    uint8_t *Data = wdata;

    uOffset = waddr % BYTE_PAGE;
    page = (uNum + uOffset) / BYTE_PAGE;
    uTail = (page > 0) ? (uNum + uOffset - page * BYTE_PAGE) : uNum;

    if (page == 0)
    {
        return I2C_PolledMasterWrite(ADDR_EP24Cxx, (uint8_t)waddr, Data, (uint8_t)uTail);
    }

    bRet = I2C_PolledMasterWrite(ADDR_EP24Cxx, (uint8_t)waddr, Data, (uint8_t)(BYTE_PAGE - uOffset));
    if (!bRet) { return false; }

    waddr = (uint16_t)(waddr + BYTE_PAGE - uOffset);
    Data = Data + BYTE_PAGE - uOffset;
    Delay_1ms(DLY_WRITE);

    for (i = 0; i < page - 1; i++)
    {
        bRet = I2C_PolledMasterWrite(ADDR_EP24Cxx, (uint8_t)(waddr + i * BYTE_PAGE), Data + i * BYTE_PAGE, (uint8_t)BYTE_PAGE);
        if (!bRet) { return false; }
        Delay_1ms(DLY_WRITE);
    }

    if (uTail > 0)
    {
        return I2C_PolledMasterWrite(ADDR_EP24Cxx, (uint8_t)(waddr + (page - 1) * BYTE_PAGE), Data + (page - 1) * BYTE_PAGE, (uint8_t)uTail);
    }

    return true;
}

void Save_CustomCode(void)
{
    if (gSysData.m_Custcode == ui_custcode) { return; }

    EpromData[0] = LOBYTE(ui_custcode);
    EpromData[1] = HIBYTE(ui_custcode);
    WriteEpData(SYSDATA_START + SYS_ADDR_OFFSET, EpromData, 2);
}

void clr_Custcode(void)
{
    ui_custcode = 0;
    if (gSysData.m_Custcode == 0) { return; }

    EpromData[0] = 0;
    EpromData[1] = 0;
    WriteEpData(SYSDATA_START + SYS_ADDR_OFFSET, EpromData, 2);
}

static void InitEprom_SysData(void)
{
    memset(EpromData, 0, DATA_EPROM_CNT);
    EpromData[0] = LOBYTE(SIGNAL);
    EpromData[1] = HIBYTE(SIGNAL);
    WriteEpData(SYSDATA_START, EpromData, DATA_EPROM_CNT);
}

static uint16_t GetEpromSign(void)
{
    uint16_t uEpromSign = 0;
    ReadEpData(SIGNADDR, (uint8_t *)&uEpromSign, 2);
    return uEpromSign;
}

void Init_SysData(void)
{
    uint16_t sign = GetEpromSign();

    if (sign == SIGNAL)
    {
        memset(EpromData, 0, DATA_SYS_CNT);
        ReadEpData(SYSDATA_START + SYS_DATA_OFFSET, EpromData, DATA_SYS_CNT);

        gSysData.m_flg = MAKEWORD(EpromData[1], EpromData[0]);
        gSysData.m_Custcode = MAKEWORD(EpromData[3], EpromData[2]);
    }
    else
    {
        InitEprom_SysData();
        memset(&gSysData, 0, sizeof(gSysData));
    }

    ui_sys_flg = gSysData.m_flg;
    (void)ui_sys_flg;
}


