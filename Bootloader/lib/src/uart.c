#include "uart.h"

void uart1Init(uint32_t coreFreq, uint32_t baudRate)
{
	//PA10 - RX
	//PA9 - TX
	RCC->APB2ENR|=RCC_APB2ENR_USART1EN;//Тактирование usart1
	RCC->APB2ENR|=RCC_APB2ENR_IOPAEN;//Тактирование порта А
	GPIOA->CRH &= (~GPIO_CRH_CNF9_0);
	//PA9 - настройка на выход альтернативной функции
	GPIOA->CRH |= (GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9);
	//PA10 - вход с подтягивающим резистором
	GPIOA->CRH &= (~GPIO_CRH_CNF10_0);
	GPIOA->CRH |= GPIO_CRH_CNF10_1;
	GPIOA->CRH &= (~(GPIO_CRH_MODE10));
	GPIOA->BSRR |= GPIO_ODR_ODR10;
	USART1->CR1 = 0;//Сбрасываем конфигурацию
	USART1->CR1|=USART_CR1_RE|USART_CR1_TE;//Включить приём и передачу
	USART1->BRR|=coreFreq/baudRate;//Скорость работы uart
	USART1->CR1|=USART_CR1_UE;//Включить uart
}

void uart1Transmitt(uint8_t data)
{
	while (!(USART1->SR & USART_SR_TXE)){}
	USART1->DR = data;
}

uint8_t uart1Recive()
{
	while(!(USART1->SR & USART_SR_RXNE)){}
	return USART1->DR;
}

void uart1Printf(char* str)
{
	while(*str)
	{
		uart1Transmitt(*str);
		str++;
	}
}

void uart1ReciveString(uint8_t* buff)
{
	uint16_t i=0;
	do
	{
		i++;
		buff[i-1]=uart1Recive();
	}while(buff[i-1]!=0x0D);
	buff[i]='\n';
	buff[i+1]=0;
}