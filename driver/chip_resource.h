#include "driver_uart.h"
#include "driver_timer.h"
#include "driver_i2c.h"

#define TERMINAL_UART        (&Uart0)
#define SYSTEM_TIMER_TIMER   (&Timer5)

extern UartStruct Uart0;
extern UartStruct Uart1;

extern I2cStruct  I2c0;
extern I2cStruct  I2c1;

extern TimerStruct Timer0;
extern TimerStruct Timer5;

