#include "stdint.h"

enum ClockSource
{
	IRC = 0,
	HXTAL,
};

int8_t RCC_Init(void);
int8_t SetSystemClock(uint32_t freq);
int8_t GetSystemClock(uint32_t *freq);
int8_t SetSystemClockSource(enum ClockSource);
int8_t RCC_Deinit(void);



