/**
 * @file rom_key_value.h
 * @author qiquan.cui 
 * @brief 此文件用于保存和读取掉电不丢失的关键数据
 * @version 0.1
 * @date 2023-08-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "stdint.h"

#define MAX_NAME_SIZE       23
#define MAX_VALUE_SIZE      8

#define SET_KEY_VALUE(name, data)   set_key_value(name, &data, sizeof(data))
#define GET_KEY_VALUE(name, data)   get_key_value(name, &data, sizeof(data))

#ifdef __cplusplus
extern "C" {
#endif

int8_t rom_key_value_init(void);

int8_t set_key_value(const char *name, void *data, uint16_t len);

int8_t get_key_value(const char *name, void *data, uint16_t len);

int8_t delete_key_value(const char *name);

#ifdef __cplusplus
}
#endif



