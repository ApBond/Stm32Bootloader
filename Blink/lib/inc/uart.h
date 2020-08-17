#ifndef UART_H
#define UART_H

#include "stm32f10x.h"

void uart1Init(uint32_t coreFreq, uint32_t baudRate);
void uart1Transmitt(uint8_t data);
uint8_t uart1Recive(void);
void uart1Printf(char* str);
void uart1ReciveString(uint8_t* buff);


#endif


