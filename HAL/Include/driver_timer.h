#pragma once

#include "stdint.h"

typedef void (*TimerTimeOutFunc)(void);

typedef struct __TimerInitStruct
{
	uint32_t update_time_us;

}TimerInitStruct;

typedef struct __TimerStruct
{
    TimerInitStruct Init;
	
	TimerTimeOutFunc time_out_call_back;
}TimerStruct;

int8_t TimerInit(TimerStruct *timer, TimerInitStruct *init);
