#pragma once

#include "stdint.h"

typedef void (*TimerUpdateCpltFunc)(void);

typedef struct __TimerInitStruct
{
	uint32_t update_time_us;

}TimerInitStruct;

typedef struct __TimerStruct
{
    TimerInitStruct Init;

	uint8_t timer_id;
	
	TimerUpdateCpltFunc timer_update_func;
}TimerStruct;

// typedef enum __TimerInterruptType
// {
// 	UPDATE_INT = 0
// }TimerInterruptType;

int8_t TimerInit(TimerStruct *timer, TimerInitStruct *init);

int8_t TimerUpdateCallbackRegister(TimerStruct *timer, TimerUpdateCpltFunc func);

uint32_t GetTimerCNT(TimerStruct *timer);

void DisableTimerUpdateInt(TimerStruct *timer);

void EnableTimerUpdateInt(TimerStruct *timer);

void TimerUpdateCallback(TimerStruct *timer);

