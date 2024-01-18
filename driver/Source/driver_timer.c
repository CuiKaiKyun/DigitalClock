#include "driver_timer.h"
#include "stddef.h"
#include "gd32f30x.h"
#include "chip_resource.h"

#define DRV_TIMERn     2

#define DRV_TIMER0                        TIMER0
#define DRV_TIMER5                        TIMER5

static uint32_t TIMER_PERIPH[DRV_TIMERn] = {DRV_TIMER0, DRV_TIMER5};
static rcu_periph_enum TIMER_CLK[DRV_TIMERn] = {RCU_TIMER0, RCU_TIMER5};
static IRQn_Type TIMER_IRQ[DRV_TIMERn] = {TIMER0_UP_IRQn, TIMER5_IRQn};
static rcu_clock_freq_enum TIMER_CLK_SRC[DRV_TIMERn] = {CK_APB2, CK_APB1};

int8_t TimerInit(TimerStruct *timer, TimerInitStruct *init)
{
    timer_parameter_struct timer_initpara;
    uint32_t clock_src_freq;
    uint32_t apb_clk_freq;

    if(timer == &Timer0)
    {
        timer->timer_id = 0;
    }
    else if(timer == &Timer5)
    {
        timer->timer_id = 1;
    }
    
    nvic_irq_enable(TIMER_IRQ[timer->timer_id], 0, 1);

    rcu_periph_clock_enable(TIMER_CLK[timer->timer_id]);

    timer_deinit(TIMER_PERIPH[timer->timer_id]);

    // 根据GD32F30x_用户手册 P79，若APB时钟小于AHB时钟，则定时器的时钟源为APB的两倍
    apb_clk_freq = rcu_clock_freq_get(TIMER_CLK_SRC[timer->timer_id]);
    if(apb_clk_freq < rcu_clock_freq_get(CK_AHB))
    {
        clock_src_freq = apb_clk_freq*2;
    }
    else
    {
        clock_src_freq = apb_clk_freq;
    }

    /* TIMER0 configuration */
    timer_initpara.prescaler         = clock_src_freq/1000000U - 1;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = init->update_time_us-1;
    // timer_initpara.clockdivision     = TIMER_CKDIV_DIV4;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER_PERIPH[timer->timer_id], &timer_initpara);
	
	timer_interrupt_enable(TIMER_PERIPH[timer->timer_id], TIMER_INT_UP);

    timer_update_event_enable(TIMER_PERIPH[timer->timer_id]);

    /* TIMER0 counter enable */
    timer_enable(TIMER_PERIPH[timer->timer_id]);

    return 0;
}

uint32_t GetTimerCNT(TimerStruct *timer)
{
    return timer_counter_read(TIMER_PERIPH[timer->timer_id]);
}

void DisableTimerUpdateInt(TimerStruct *timer)
{
    timer_interrupt_disable(TIMER_PERIPH[timer->timer_id], TIMER_INT_UP);
}

void EnableTimerUpdateInt(TimerStruct *timer)
{
    timer_interrupt_enable(TIMER_PERIPH[timer->timer_id], TIMER_INT_UP);
}

int8_t TimerUpdateCallbackRegister(TimerStruct *timer, TimerUpdateCpltFunc func)
{
    timer->timer_update_func = func;
    return 0;
}

void TimerUpdateCallback(TimerStruct *timer)
{
    if(timer->timer_update_func != NULL)
        timer->timer_update_func();
}
