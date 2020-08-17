#ifndef __ADC
#define __ADC
#include "stm32f10x.h"

int AdcInit(void);
uint16_t AdcRead(uint8_t channal);
void AdcCycleRead(uint8_t ch1,uint8_t ch2);

#endif
