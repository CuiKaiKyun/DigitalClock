/**
 * @file rom_key_value_port.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "rom_key_value_port.h"
#include "rom_key_value_cfg.h"
#include "string.h"
#include "stdint.h"
#include "stm32f7xx_hal.h"

#define MIN_ERASE_SIZE          0x20000
#define MIN_WRITE_SIZE          4

int8_t flash_write(uint32_t phy_addr, void *data, uint32_t size)
{
    int8_t ret = 0;
    uint32_t wr_cnt;
    void *wr_pointer = data;
    uint32_t wr_phy_addr = phy_addr;
    if(size % MIN_WRITE_SIZE != 0)
    {
        return -1;
    }

    if(phy_addr + size > START_ADDR + USED_SPACE_SIZE + USED_SPACE_SIZE || \
        phy_addr < START_ADDR)
    {
        return -1;
    }

	HAL_FLASH_Unlock();
	// 循环写入数据
    for(wr_cnt = size/MIN_WRITE_SIZE; wr_cnt > 0; wr_cnt --)
    {
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, wr_phy_addr, *((uint32_t *)wr_pointer)) != HAL_OK)
        {
        	ret = -1;
        }
        wr_pointer += MIN_WRITE_SIZE;
        wr_phy_addr += MIN_WRITE_SIZE;
    }
	HAL_FLASH_Lock();

    return ret;
}

int8_t flash_read(uint32_t phy_addr, void *data, uint32_t size)
{
    void *pdata = (void *)phy_addr;

    if(data == NULL)
    {
        return -1;
    }

    if(phy_addr + size > START_ADDR + USED_SPACE_SIZE + USED_SPACE_SIZE || \
        phy_addr < START_ADDR)
    {
        return -1;
    }

    memcpy(data, pdata, size);

    return 0;
}

int8_t flash_erase(uint32_t phy_addr, uint32_t size)
{
    FLASH_EraseInitTypeDef flash_erase_struct;
    uint32_t sector_err;
    // 检查可以被整除
    if(size % MIN_ERASE_SIZE != 0)
    {
        return -1;
    }

    flash_erase_struct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    flash_erase_struct.TypeErase = FLASH_TYPEERASE_SECTORS;
    flash_erase_struct.NbSectors = 1;

	// TODO 改成可读性更高的写法
    if(phy_addr == START_ADDR)
        flash_erase_struct.Sector = FLASH_SECTOR_22;
    else if(phy_addr == START_ADDR + USED_SPACE_SIZE)
        flash_erase_struct.Sector = FLASH_SECTOR_23;

    HAL_FLASH_Unlock();
    if(HAL_FLASHEx_Erase(&flash_erase_struct, &sector_err) != HAL_OK)
    {
        return -1;
    }
    HAL_FLASH_Lock();

    return 0;
}
