

#include "gd32f30x.h"


int main(void)
{
	
	while(1)
	{
		GPIO_BOP(GPIOC) = GPIO_PIN_0;
	}
}
