#include "main.h"

#define PROGRAMM_START_ADR 0x08002400
#define BOOT_KEY 0x861D8B36
#define BOOT_KEY_ADR 0x0801FC00

typedef enum
{
	APP,
	BOOT
}STATE;

int main(void)
{
	
	uint16_t i;
	uint32_t tempData;
	uint8_t tcpId;
	uint8_t* tcpData;
	uint16_t len;
	STATE state = APP;
	
	__disable_irq();
	SCB->VTOR=PROGRAMM_START_ADR;
	__enable_irq();
	RCC->APB2ENR|=RCC_APB2ENR_IOPCEN;
	GPIOC->CRH|=GPIO_CRH_MODE13;
	GPIOC->CRH&=~GPIO_CRH_CNF13;
	GPIOC->BSRR|=GPIO_BSRR_BR13;
	
	delayInit();
	netInit();
	delay_ms(100);
	tcpId = tcpCreate(80);
	tcpListen(tcpId);
	while(1)
	{
		netPool();
		tcpData = tcpRead(tcpId,&len);
		switch(state)
		{
			case APP:
				if(tcpData!=0)
				{
					if(!memcmp(tcpData,"getstate",8))
					{
						tcpSendData(tcpId,(uint8_t*)"appl",4);
					}
					else if(!memcmp(tcpData,"boot",4))
					{
						flashUnlock();
						flashWriteData(BOOT_KEY_ADR,BOOT_KEY);
						flashLock();
						tcpSendData(tcpId,(uint8_t*)"toboot",6);
						tcpClose(tcpId);
						state=BOOT;
					}
				}
				break;
			case BOOT:
				if(getTcpStatus(tcpId)==TCP_CLOSED)
				{
					NVIC_SystemReset();
				}
				break;
		}
	}
}


