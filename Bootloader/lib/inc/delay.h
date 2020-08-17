#include "stm32f10x.h"

#ifndef __DELAY
#define __DELAY
#define SYS_CORE_FREQUENSY 72000000

static __IO uint32_t TimingDelay;

int delayInit(void);
void delay_ms(__IO uint32_t nTime);
void timingDelayDecrement(void);



#endif
