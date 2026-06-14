/************************************************************************************************/
/**
* @file               ciu32f003_std_flash.c
* @author             MCU Ecosystem Development Team
* @brief              FLASH STD库驱动。
*                     实现FLASH擦除、编程API。
*
*
**************************************************************************************************
* @attention
* Copyright (c) CEC Huada Electronic Design Co.,Ltd. All rights reserved.
*
**************************************************************************************************
*/

/************************************************************************************************/
/**
* @addtogroup CIU32F003_STD_Driver
* @{
*/

/**
* @addtogroup FLASH 
* @{
*
*/
/************************************************************************************************/


/*------------------------------------------includes--------------------------------------------*/
#include "ciu32f003_std.h"

#ifdef STD_FLASH_PERIPHERAL_USED

/*-------------------------------------------functions------------------------------------------*/

/************************************************************************************************/
/**
* @addtogroup FLASH_External_Functions 
* @{
*
*/
/************************************************************************************************/ 

/**
* @brief  Flash擦除与Option byte区擦除
* @param  mode 擦除模式
*             @arg FLASH_MODE_PAGE_ERASE
*             @arg FLASH_MODE_MASS_ERASE
* @param  address 擦除访问地址
* @note   user flash区擦除时，需先调std_flash_unlock()，解锁flash
* @note   Option Byte区擦除时，需先调用std_flash_opt_unlock()，解锁选项字节
* @retval std_status_t API执行结果
*/
std_status_t std_flash_erase(uint32_t mode, uint32_t address)
{
    std_status_t status = STD_OK;
    
    /* 设置擦除模式 */
    std_flash_set_operate_mode(mode);
    
    /* 执行擦除 */
    *(uint32_t *)address = 0xFFFFFFFF;
    
    /* 等待擦除完成，查询异常标志位 */
    while (std_flash_get_flag(FLASH_FLAG_BSY));
    if ((FLASH->SR & FLASH_FLAG_WRPERR) != 0x00000000U)
    {
        status = STD_ERR;
    }
    
    /* 清除所有标志 */
    std_flash_clear_flag(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR);
    
    /* 退出擦除模式 */
    std_flash_set_operate_mode(FLASH_MODE_IDLE);
    
    return (status);
}


/**
* @brief  User Flash区与Option Byte区字编程
* @param  address   编程地址
* @param  prog_data 编程数据（4字节）
* @note   user flash区编程时，需先调std_flash_unlock()，解锁flash
* @note   Option Byte区字编程时，需先调用std_flash_opt_unlock()，解锁选项字节
* @retval std_status_t API执行结果
*/
std_status_t std_flash_word_program(uint32_t address, uint32_t prog_data)
{
    std_status_t status = STD_OK;
    
    /* 进入编程模式 */
    std_flash_set_operate_mode(FLASH_MODE_PROGRAM);

    /* 向目标地址写入数据 */
    *(uint32_t *)address = prog_data;
    
    /* 等待编程完成，查询异常标志位 */
    while (std_flash_get_flag(FLASH_FLAG_BSY));
    if ((FLASH->SR & FLASH_FLAG_WRPERR) != 0x00000000U)
    {
        status = STD_ERR;
    }
    
    if (status == STD_OK)
    {
        /* 检查编程数据是否正确 */
        if(*((__IO uint32_t *)address) != prog_data)
        {
            status = STD_ERR;
        }
    }
    
    /* 清除所有标志 */
    std_flash_clear_flag(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR);
    
    /* 退出编程模式 */
    std_flash_set_operate_mode(FLASH_MODE_IDLE);
    
    return (status);
}


/** 
* @} 
*/

#endif /* STD_FLASH_PERIPHERAL_USED */

/** 
* @} 
*/

/** 
* @} 
*/
