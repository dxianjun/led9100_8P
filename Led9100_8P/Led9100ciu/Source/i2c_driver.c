#include "i2c_driver.h"

#define EEPROM_WRITE_TIME 1
#define IIC_READ  0x01
#define IIC_WRITE 0x00

#define SetSCL() std_gpio_set_pin(I2C_GPIO_PORT, I2C_SCL_PIN)
#define ClrSCL() std_gpio_reset_pin(I2C_GPIO_PORT, I2C_SCL_PIN)
#define SetSDA() std_gpio_set_pin(I2C_GPIO_PORT, I2C_SDA_PIN)
#define ClrSDA() std_gpio_reset_pin(I2C_GPIO_PORT, I2C_SDA_PIN)
#define GetSDA() std_gpio_get_input_pin(I2C_GPIO_PORT, I2C_SDA_PIN)

static void I2C_delay(uint16_t cnt)
{
    while (cnt--) { __NOP(); }
}

static void Delay1ms_i2c(uint16_t cnt)
{
    while (cnt--) { I2C_delay(1000); std_iwdg_refresh();}
}

#define KtIICDelay1() I2C_delay(18)
#define KtIICDelays1() I2C_delay(6)

static void SDA_IO_SET(uint8_t input)
{
    std_gpio_init_t gpio = {0};
    gpio.pin = I2C_SDA_PIN;
    gpio.mode = input ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT;
    gpio.pull = input ? GPIO_PULLUP : GPIO_NOPULL;
    gpio.output_type = GPIO_OUTPUT_OPENDRAIN;
    std_gpio_init(I2C_GPIO_PORT, &gpio);
}

#define SetOutputAll() SDA_IO_SET(0)
#define SetInputSDA()  SDA_IO_SET(1)
#define SetOutputSDA() SDA_IO_SET(0)

void I2c_gpio_init(void)
{

}

static void I2C_Start(void)
{
    SetOutputAll();
    SetSDA();
    KtIICDelays1();
    SetSCL();
    KtIICDelays1();
    ClrSDA();
    KtIICDelays1();
    ClrSCL();
    KtIICDelays1();
}

static void I2C_Stop(void)
{
    SetOutputAll();
    ClrSDA();
    KtIICDelays1();
    SetSCL();
    KtIICDelays1();
    SetSDA();
}

static void I2C_SendAck(void)
{
    ClrSDA();
    KtIICDelays1();
    SetSCL();
    KtIICDelay1();
    ClrSCL();
    KtIICDelays1();
}

static void I2C_SendNoAck(void)
{
    SetSDA();
    KtIICDelays1();
    SetSCL();
    KtIICDelay1();
    ClrSCL();
    KtIICDelays1();
}

static bool I2C_ChkAck(void)
{
    bool ack;
    uint16_t i;

    ClrSCL();
    SetInputSDA();
    KtIICDelays1();
    SetSCL();
    SetSDA();
    KtIICDelays1();

    ack = false;
    for (i = 0; i < 250; i++)
    {
        ack = !GetSDA();
        if (ack) { break; }
    }

    KtIICDelays1();
    ClrSCL();
    KtIICDelays1();
    SetOutputSDA();
    return ack;
}

static bool I2C_WriteByte(uint8_t dat)
{
    uint8_t i = 8;
    SetOutputSDA();
    ClrSCL();
    KtIICDelays1();

    while (i--)
    {
        if (dat & 0x80) { SetSDA(); } else { ClrSDA(); }
        dat <<= 1;
        KtIICDelays1();
        SetSCL();
        KtIICDelay1();
        ClrSCL();
        KtIICDelays1();
		std_iwdg_refresh();
    }
    return I2C_ChkAck();
}

static uint8_t I2C_ReadByte(void)
{
    uint8_t i = 8;
    uint8_t dat = 0;

    SetInputSDA();
    SetSDA();
    KtIICDelays1();

    while (i--)
    {
        SetSCL();
        dat <<= 1;
        if (GetSDA()) { dat |= 0x01; }
        KtIICDelays1();
        ClrSCL();
        KtIICDelay1();
		std_iwdg_refresh();
    }
    SetOutputSDA();
    return dat;
}

static bool I2C_WriteBytes(uint8_t* buf, uint8_t len)
{
    while (len--)
    {
        if (!I2C_WriteByte(*(buf++))) { return false; }
    }
    return true;
}

static bool I2C_ReadBytes(uint8_t* buf, uint8_t len)
{
    while (len--)
    {
        *(buf++) = I2C_ReadByte();
        if (len == 0) { I2C_SendNoAck(); }
        else { I2C_SendAck(); }
    }
    return true;
}

static bool I2C_SendAddr(uint8_t icAddr, uint16_t wAddr, uint8_t ucRW)
{
    I2C_Start();
    if (!I2C_WriteByte(icAddr))
    {
        Delay1ms_i2c(EEPROM_WRITE_TIME);
        I2C_Start();
        if (!I2C_WriteByte(icAddr))
        {
            I2C_Stop();
            return false;
        }
    }

    if (!I2C_WriteByte((uint8_t)wAddr))
    {
        I2C_Stop();
        return false;
    }

    if (ucRW == IIC_READ)
    {
        I2C_Start();
        if (!I2C_WriteByte(icAddr | IIC_READ))
        {
            I2C_Stop();
            return false;
        }
    }
    return true;
}

void i2c_gpio_init(void)
{
    std_gpio_init_t i2c_gpio = {0};

    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOB);

    i2c_gpio.pin = I2C_SCL_PIN | I2C_SDA_PIN;
    i2c_gpio.mode = GPIO_MODE_OUTPUT;
    i2c_gpio.pull = GPIO_NOPULL;
    i2c_gpio.output_type = GPIO_OUTPUT_OPENDRAIN;
    std_gpio_init(I2C_GPIO_PORT, &i2c_gpio);

    SetSCL();
    SetSDA();
}

void I2C_Configuration(void)
{
    i2c_gpio_init();
}

bool I2C_PolledMasterWrite(uint8_t device_addr, uint8_t regADD, uint8_t* write_data, uint8_t num_bytes)
{
    bool ack = I2C_SendAddr(device_addr, regADD, IIC_WRITE);
    ack = ack && I2C_WriteBytes(write_data, num_bytes);
    I2C_Stop();
    return ack;
}

bool I2C_PolledMasterRead(uint8_t device_addr, uint8_t regADD, uint8_t* read_data, uint8_t num_bytes)
{
    bool ack;
    device_addr &= 0xFE;
    ack = I2C_SendAddr(device_addr, regADD, IIC_READ);
    ack = ack && I2C_ReadBytes(read_data, num_bytes);
    I2C_Stop();
    return ack;
}

bool I2C_PolledMasterNWrite(uint8_t device_addr, uint8_t* write_data, uint8_t num_bytes)
{
    bool ack = true;

    I2C_Start();
    if (!I2C_WriteByte(device_addr))
    {
        Delay1ms_i2c(EEPROM_WRITE_TIME);
        I2C_Start();
        if (!I2C_WriteByte(device_addr))
        {
            I2C_Stop();
            return false;
        }
    }

    ack = ack && I2C_WriteBytes(write_data, num_bytes);
    I2C_Stop();
    return ack;
}

bool I2C_PolledMasterNRead(uint8_t device_addr, uint8_t* read_data)
{
    bool ack = true;
    device_addr &= 0xFE;

    I2C_Start();
    if (!I2C_WriteByte(device_addr))
    {
        Delay1ms_i2c(EEPROM_WRITE_TIME);
        I2C_Start();
        if (!I2C_WriteByte(device_addr))
        {
            I2C_Stop();
            return false;
        }
    }

    I2C_Start();
    if (!I2C_WriteByte(device_addr | IIC_READ))
    {
        I2C_Stop();
        return false;
    }

    ack = ack && I2C_ReadBytes(read_data, 1);
    I2C_Stop();
    return ack;
}

bool I2C_WriteData(unsigned char Cmd, unsigned char write_data)
{
    bool ack;
    I2C_Start();
    if (!I2C_WriteByte(Cmd))
    {
        I2C_Stop();
        return false;
    }

    ack = I2C_WriteByte(write_data);
    I2C_Stop();
    return ack;
}


