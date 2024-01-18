#pragma once

#include "stdint.h"

typedef void (*I2cWriteCallback)(void);
typedef void (*I2cReadCallback)(void);

typedef struct __I2cInitStruct
{
    uint8_t     local_addr;
    uint32_t    speed;
}I2cInitStruct;

typedef struct __I2cStruct
{
    // I2cInitStruct Init;
    uint8_t i2c_id;

    uint8_t inited;
    
    enum {
        I2C_SEND_ADDRESS_FIRST = 0,
        I2C_CLEAR_ADDRESS_FLAG_FIRST,
        I2C_TRANSMIT_WRITE_READ_ADD,
        I2C_TRANSMIT_DATA,
        I2C_STOP,
    }i2c_process;
    
    struct 
    {
        uint8_t writing;
        uint8_t *pdata;
        uint16_t cur_data_num;
        uint16_t total_data_len;
        uint8_t slave_addr;
    }write_info;
    
    struct 
    {
        uint8_t reading;
        uint8_t *pdata;
        uint16_t cur_data_num;
        uint16_t total_data_len;
        uint8_t slave_addr;
    }read_info;
    I2cWriteCallback write_call_back;
    I2cReadCallback read_call_back;
}I2cStruct;

int8_t I2cInit(I2cStruct *i2c, I2cInitStruct *init);

int8_t I2cWrite(I2cStruct *i2c, uint8_t dev_addr, uint8_t *data, uint16_t data_len);

int8_t I2cRead(I2cStruct *i2c, uint8_t dev_addr, uint8_t *data, uint16_t data_len);

void I2cEventCallback(I2cStruct *i2c);

void I2cErrorCallback(I2cStruct *i2c);
