#include "system_timer.h"
#include "driver.h"

#ifndef SYSTEM_TIMER_TIMER
    #error "please define SYSTEM_TIMER_TIMER first!"
#endif

#define SYSTEM_TIMER_PERIOD_US      50000

static void system_timer_update(void);

volatile uint32_t SystemTimerUpdateCnt = 0;	// 多线程访问变量

/**
* @brief  Initialize Timer
* @param  htim_x : HAL Handler of timer x.
* @param  src : Choose the src for delay_ms().
* @retval None
*/
int8_t SystemTimerInit(void)
{
    TimerInitStruct timer_init;

    timer_init.update_time_us = SYSTEM_TIMER_PERIOD_US;
    TimerInit(SYSTEM_TIMER_TIMER, &timer_init);
    TimerUpdateCallbackRegister(SYSTEM_TIMER_TIMER, &system_timer_update);

    return 0;
}


/**
* @brief  Get the system tick from timer.
* @param  None
* @retval current tick.
*/
uint64_t GetSystemTimer_us(void)
{
    uint32_t update_cnt;
    uint32_t update_cnt_confirm;
    uint32_t timer_cnt;
    // 先读一遍update_cnt,再读一遍定时器计数值，再读一遍update_cnt，如果两次cnt不一样，
    // 则表示中途进过一次定时器中断，timer_cnt归零过一次，不能确保此时计数值timer_cnt
    // 对应的是哪一个update_cnt，需要重新读取一次timer_cnt，并按照这个新的来计算当前
    // 系统时间。
    DisableTimerUpdateInt(SYSTEM_TIMER_TIMER);
    update_cnt = SystemTimerUpdateCnt;
    EnableTimerUpdateInt(SYSTEM_TIMER_TIMER);

    timer_cnt = GetTimerCNT(SYSTEM_TIMER_TIMER);

    DisableTimerUpdateInt(SYSTEM_TIMER_TIMER);
    update_cnt_confirm = SystemTimerUpdateCnt;
    EnableTimerUpdateInt(SYSTEM_TIMER_TIMER);

    if(update_cnt == update_cnt_confirm)
    {
        return update_cnt * SYSTEM_TIMER_PERIOD_US + timer_cnt;
    }
    else
    {
        // 如果两次获取到的溢出次数不一样，证明中间经历过一次溢出，
        // 不能确定定时器计数值对应的是溢出前获取的还是溢出
        // 后获取的，因此重新取一次，这次一定是对应溢出后的定时器计数值。
        uint32_t timer_cnt_confirm;
        timer_cnt_confirm = GetTimerCNT(SYSTEM_TIMER_TIMER);
        return update_cnt_confirm * SYSTEM_TIMER_PERIOD_US + timer_cnt_confirm;
    }
}

/**
* @brief  Get the system tick from timer.
* @param  None
* @retval current tick.
*/
uint64_t GetSystemTimer_ms(void)
{ 
    return GetSystemTimer_us() / 1000;
}

static void system_timer_update(void)
{
    SystemTimerUpdateCnt++;
}
