#include "stm32f10x.h"

void main()
{
	__disable_irq();
	SCB->VTOR=0x08002000;
	__enable_irq();
	RCC->APB2ENR|=RCC_APB2ENR_IOPCEN;
	GPIOC->CRH|=GPIO_CRH_MODE13;
	GPIOC->CRH&=~GPIO_CRH_CNF13;
	GPIOC->BSRR|=GPIO_BSRR_BR13;
	while(1)
	{
		
	}
}