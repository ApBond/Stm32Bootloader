#include "main.h"

#define PROGRAMM_START_ADR 0x08002400
#define BOOT_KEY 0x861D8B36
#define BOOT_KEY_ADR 0x0801FC00
#define APP_SIZE_ADR 0x0801FC04
#define FRAME_SIZE 512

typedef enum STATE
{
	NOT_OPERATION,//Ожидание команды от клиента
	UPLOAD,//Загрузка программы
	COMPLETE,//Загрузка программы завершена, переход к основной программе
	TO_APPL//Переход к основное программе по команде клиента
}STATE;

uint32_t frameBuff[FRAME_SIZE/4];//Буфер для отправки данных по tcp
uint8_t tcpId;//id tcp сервера
STATE state=NOT_OPERATION;//Статус программы
uint32_t validAddr = PROGRAMM_START_ADR;//Текущий адрез загружаемого flash
uint64_t mainAppLen;//Размер основной программы
uint64_t recivedLen=0;//Кол-во отправленных данных

void jumpToMainApp()
{
	uint32_t app_jump_address;
	typedef void(*pFunction)(void);
	pFunction Jump_To_Application;
	//Сброс периферии
	RCC->APB1RSTR = 0xFFFFFFFF;
	RCC->APB1RSTR = 0x0; 
	RCC->APB2RSTR = 0xFFFFFFFF;
	RCC->APB2RSTR = 0x0; 
	RCC->APB1ENR = 0x0;
	RCC->APB2ENR = 0x0;
	RCC->AHBENR = 0x0;
	__disable_irq();
	app_jump_address = *( uint32_t*) (PROGRAMM_START_ADR + 4);//Адрес перехода к основной программе 
	Jump_To_Application = (pFunction)app_jump_address;
	__set_MSP(*(__IO uint32_t*) PROGRAMM_START_ADR);//Перенос стека
	Jump_To_Application();//Переход к основной программе
}

void toApplMode()
{
	uint32_t temp;
	if(flashRead(PROGRAMM_START_ADR)!=0xFFFFFFFF)//Проверка наличия программы
	{
		//Стирание boot ключа из flash
		flashUnlock();
		temp = flashRead(APP_SIZE_ADR);
		flashPageErase(BOOT_KEY_ADR);
		flashWriteData(APP_SIZE_ADR,temp);
		flashLock();
		tcpSendData(tcpId,(uint8_t*)"toappl",6);//Сообщение клиенту о переходе к основной программе
		state=TO_APPL;
	}
	else
	{
		tcpSendData(tcpId,(uint8_t*)"noappl",6);//Сообщение клиенту об остуствии основной программы
	}
}

int main(void)
{
	uint16_t i;
	uint32_t tempData;
	uint8_t messBuff[13]="loadstart";
	uint8_t* tcpData;
	uint16_t len;
	uint32_t bootKey;
	flashUnlock();
	bootKey = flashRead(BOOT_KEY_ADR);//Считыввание boot ключа
	flashLock();
	if(bootKey!=BOOT_KEY && flashRead(PROGRAMM_START_ADR)!=0xFFFFFFFF)//Проверка boot ключа и наличия основное программы
	{
		jumpToMainApp();
	}
	delayInit();
	netInit();
	delay_ms(100);
	tcpId = tcpCreate(80);//Создание tcp сервера
	tcpListen(tcpId);
	while(1)
	{
		netPool();//Проверка пакетов
		tcpData = tcpRead(tcpId,&len);//Получение данных от tcp сервера
		switch(state)
		{
			case NOT_OPERATION:
				if(tcpData!=0)
				{
					if(!memcmp(tcpData,"start",5))//Команда начала загрузки
					{
						memcpy(&mainAppLen,tcpData+7,len-7);//Извлекаем размер прошивки 
						for(i=0;i<(mainAppLen/1024+1);i++)//Стираем flash память
						{
							flashUnlock();
							flashPageErase(validAddr+i*1024);
							flashLock();
						}
						tcpSendData(tcpId,tcpData+5,len-5);//Возвращаем клиенту ключ
						state=UPLOAD;
					}
					else if(!memcmp(tcpData,"getstate",8))//Команда проверки состояние
					{
						tcpSendData(tcpId,(uint8_t*)"boot",4);//Режим загрузчика
					}
					else if(!memcmp(tcpData,"appl",4))//Команда перехода к основной программе
					{
						toApplMode();
					}
					else if(!memcmp(tcpData,"load",4))//Команда выгрузки прошивки клиента
					{
						if(flashRead(APP_SIZE_ADR)!=0xFFFFFFFF)//Проверка наличия прошивки
						{
							tempData = flashRead(APP_SIZE_ADR);//Определение размера прошивки
							memcpy(messBuff+9,(uint8_t*)&tempData,4);
							tcpSendData(tcpId,messBuff,13);//Сообщаем клиенту размер прошивки
							i=0;
							while(1)
							{
								frameBuff[i] = flashRead(validAddr);//Чтение 4х байт прошивки
								if(validAddr!=PROGRAMM_START_ADR+tempData)
								{
									i++;
									validAddr+=4;
									if(i*4==FRAME_SIZE)
									{
										tcpSendData(tcpId,(uint8_t*)frameBuff,FRAME_SIZE);//Отправляем пакет
										delay_ms(100);//Пауза между пакетами
										i=0;
									}
								}
								else
								{
									if(i!=0)
									{
										tcpSendData(tcpId,(uint8_t*)frameBuff,i*4);
									}
									validAddr=PROGRAMM_START_ADR;
									break;
								}
							}
						}
						else
						{
							tcpSendData(tcpId,(uint8_t*)"notapp",6);
						}
					}
				}
				break;
			case UPLOAD:
				if(tcpData!=0)
				{
					for(i=0;i<len;i+=4)
					{
						tempData = (tcpData[i+3]<<24)|(tcpData[i+2]<<16)|(tcpData[i+1]<<8)|tcpData[i];//Преобразование данных в 32х битный формат
						flashUnlock();
						flashWriteData(validAddr,tempData);///Запись прошивки в flash память
						flashLock();
						validAddr+=4;
					}
					recivedLen+=len;
					if(recivedLen==mainAppLen)
					{
						tcpSendData(tcpId,(uint8_t*)"ready",5);//Сообщение клиенту о завершении загрузки
						state=COMPLETE;
					}
				}
				break;
			case COMPLETE:
				if(getTcpStatus(tcpId)==TCP_CLOSED)//Проверка соединения
				{
					flashUnlock();
					flashPageErase(BOOT_KEY_ADR);//Удаление boot ключа
					flashLock();
					flashUnlock();
					flashWriteData(APP_SIZE_ADR,validAddr-PROGRAMM_START_ADR);//Запись во flash размера прошивки
					flashLock();
					jumpToMainApp();
				}
				else
				{
					tcpClose(tcpId);//Закрытие соединения
				}
				break;
			case TO_APPL:
				if(getTcpStatus(tcpId)==TCP_CLOSED)
				{
					jumpToMainApp();
				}
				else
				{
					tcpClose(tcpId);//Закрытие соединения
				}
				break;
		}	
	}
}


