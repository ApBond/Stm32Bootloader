#include "rcc.h"

int RccClockInit()
{
	//Enable HSE
	//Setting PLL
	//Enable PLL
	//Setting count wait cycles of FLASH
	//Setting AHB1,AHB2 prescaler
	//Switch to PLL	
	uint16_t timeDelay;
	RCC->CR|=RCC_CR_HSEON;//Enable HSE
	for(timeDelay=0;;timeDelay++)
	{
		if(RCC->CR&RCC_CR_HSERDY) break;
		if(timeDelay>0x1000)
		{
			RCC->CR&=~RCC_CR_HSEON;
			return 1;
		}
	}	
	RCC->CFGR|=RCC_CFGR_PLLMULL9;//PLL x9
	RCC->CFGR|=RCC_CFGR_PLLSRC_HSE;//PLL sourse:HSE
	RCC->CR|=RCC_CR_PLLON;//Enable PLL
	for(timeDelay=0;;timeDelay++)
	{
		if(RCC->CR&RCC_CR_PLLRDY) break;
		if(timeDelay>0x1000)
		{
			RCC->CR&=~RCC_CR_HSEON;
			RCC->CR&=~RCC_CR_PLLON;
			return 2;
		}
	}
	FLASH->ACR|=FLASH_ACR_LATENCY_2;
	RCC->CFGR|=RCC_CFGR_PPRE1_DIV2;//APB1 prescaler=2
	RCC->CFGR|=RCC_CFGR_SW_PLL;//Switch to PLL
	while((RCC->CFGR&RCC_CFGR_SWS)!=(0x02<<2)){}
	RCC->CR&=~RCC_CR_HSION;//Disable HSI
	return 0;
}
