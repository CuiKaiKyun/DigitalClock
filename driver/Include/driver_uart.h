#pragma once

#include "stdint.h"

typedef struct __UartInitStruct
{
    uint32_t baudrate;
    enum{
        WordLen_8Bit = 0,
    }word_length;
    enum{
        StopBit_1Bit = 0,   // 1bit
        StopBit_2Bit,    // 2bit
        StopBit_05Bit,  // 0.5bit
        StopBit_15Bit,  // 1.5bit
    }stop_bit;
    enum{
        ParityNone = 0,
        ParityOdd,
        ParityEven,
    }parity;

}UartInitStruct;

typedef void (*UartSendCpltFunc)(void);
typedef void (*UartRecvIdleFunc)(uint16_t data_lenth);

typedef struct __UartStruct
{
    UartInitStruct Init;
	
    struct 
    {
        uint8_t send_start;
    }send_info;
	
    struct 
    {
        uint8_t receive_start;
        uint16_t dma_total_len;
    }receive_info;
	UartSendCpltFunc send_cplt_call_back;
    UartRecvIdleFunc recv_idle_call_back;
}UartStruct;

int8_t UartInit(UartStruct *Uart, UartInitStruct *Init);

int8_t UartSendDMA(UartStruct *Uart, const uint8_t *data, uint16_t data_len);

int8_t UartReceiveToIdleDMA(UartStruct *Uart, uint8_t *data, uint16_t data_len);

int8_t UartSendCallbackRegister(UartStruct *Uart, UartSendCpltFunc func);

int8_t UartRecvCallbackRegister(UartStruct *Uart, UartRecvIdleFunc func);

void UartSendCompleteCallback(UartStruct *Uart);

void UartReceiveIdleCallback(UartStruct *Uart);

