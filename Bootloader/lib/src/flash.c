#include "flash.h"

void flashUnlock()
{
	FLASH->KEYR = FLASH_KEY_1;
	FLASH->KEYR = FLASH_KEY_2;
}

void flashLock()
{
	FLASH->CR|=FLASH_CR_LOCK;
}

void flashWriteData(uint32_t adr, uint32_t data)
{
	
	while((FLASH->SR & FLASH_SR_BSY)!=0);
	FLASH->CR|=FLASH_CR_PG;
	*(__IO uint16_t*) adr = (uint16_t)data;
	while((FLASH->SR & FLASH_SR_BSY)!=0);
	adr+=2;
	data>>=16;
	*(__IO uint16_t*) adr = (uint16_t)data;
	while((FLASH->SR & FLASH_SR_BSY)!=0);
	FLASH->CR&=~FLASH_CR_PG;
}

void flashPageErase(uint32_t adr)
{
	FLASH->CR|=FLASH_CR_PER;
	FLASH->AR = (uint32_t)adr;
	FLASH->CR|=FLASH_CR_STRT;
	while((FLASH->SR & FLASH_SR_BSY)!=0);
	FLASH->CR&=~FLASH_CR_PER;
}

uint32_t flashRead(uint32_t address)
{
	return (*(__IO uint32_t*) address);
}