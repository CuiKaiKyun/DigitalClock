#include "driver_rcc.h"
#include "gd32f30x.h"

// default freq
uint32_t system_clock = 120000000;

// default source
enum ClockSource clock_source = HXTAL;

int8_t RCC_Init(void)
{
	return 0;
}

int8_t SetSystemClock(uint32_t freq)
{
	if(clock_source == HXTAL)
	{
		if(freq % (HXTAL_VALUE/2) == 0)
		{
			 uint32_t timeout = 0U;
			uint32_t stab_flag = 0U;

			/* select IRC8M as system clock source, deinitialize the RCU */
			rcu_system_clock_source_config(RCU_CKSYSSRC_IRC8M);
			rcu_deinit();
			
			/* enable HXTAL */
			  RCU_CTL |= RCU_CTL_HXTALEN;

			/* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
			do{
				timeout++;
				stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
			}while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

			/* if fail */
			if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)){
				while(1){
				}
			}

			/* HXTAL is stable */
			/* AHB = SYSCLK */
			RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
			/* APB2 = AHB */
			RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
			/* APB1 = AHB */
			RCU_CFG0 |= RCU_APB1_CKAHB_DIV1;

			/* PLL = HXTAL / 25 * 36 = 36 MHz */
			RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF_4| RCU_CFG0_PLLMF_5);
			RCU_CFG0 |= (RCU_PLLSRC_HXTAL_IRC48M | RCU_PLL_MUL24 | RCU_CFG0_PREDV0);


			/* enable PLL and PLL1 */
			RCU_CTL |= RCU_CTL_PLLEN;

			/* wait until PLL is stable */
			while(0U == (RCU_CTL & RCU_CTL_PLLSTB)){
			}

			/* select PLL as system clock */
			RCU_CFG0 &= ~RCU_CFG0_SCS;
			RCU_CFG0 |= RCU_CKSYSSRC_PLL;

			/* wait until PLL is selected as system clock */
			while(0U == (RCU_CFG0 & RCU_SCSS_PLL)){
			}
			
			return 0;
		}
		else
		{return -1;}
		
	}
	else
	{
		if(freq % (HXTAL_VALUE/2) == 0)
		{
			uint8_t pll = freq/HXTAL_VALUE*2;
			
			rcu_pll_config(RCU_PLLSRC_IRC8M_DIV2, pll);
			system_clock = freq;
			
			return 0;
		}
		else
		{return -1;}
		
	}
}

int8_t GetSystemClock(uint32_t *freq)
{
	*freq = rcu_clock_freq_get(CK_SYS);
	
	return 0;
}

