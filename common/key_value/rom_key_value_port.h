/**
 * @file rom_key_value_port.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "stdint.h"
#include "../sys_support.h" // TODO 优化包含关系

int8_t flash_init(void);

int8_t flash_write(uint32_t phy_addr, void *data, uint32_t size);

int8_t flash_read(uint32_t phy_addr, void *data, uint32_t size);

int8_t flash_erase(uint32_t phy_addr, uint32_t size);

