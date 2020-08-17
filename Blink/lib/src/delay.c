#include "delay.h"

int delayInit()
{
	return SysTick_Config(SYS_CORE_FREQUENSY/1000);
}

void delay_ms(__IO uint32_t nTime)
{
	TimingDelay=nTime;
	while(TimingDelay!=0){}
}

void timingDelayDecrement(void)
{
	if(TimingDelay!=0) TimingDelay--;
}

void SysTick_Handler(void)
{
	timingDelayDecrement();
}
