#ifndef _SD_H
#define _SD_H
#include "stm32f10x.h"
#include "uart.h"
#include "delay.h"

#define SD_SELECT GPIOB->BSRR|=GPIO_BSRR_BR12
#define SD_DESELECT GPIOB->BSRR|=GPIO_BSRR_BS12

//--------------------------------------------------
/* Card type flags (CardType) */
#define CT_MMC 0x01 /* MMC ver 3 */
#define CT_SD1 0x02 /* SD ver 1 */
#define CT_SD2 0x04 /* SD ver 2 */
#define CT_SDC (CT_SD1|CT_SD2) /* SD */
#define CT_BLOCK 0x08 /* Block addressing */
//--------------------------------------------------
// Definitions for MMC/SDC command
#define CMD0 (0x40+0) // GO_IDLE_STATE
#define CMD1 (0x40+1) // SEND_OP_COND (MMC)
#define ACMD41 (0xC0+41) // SEND_OP_COND (SDC)
#define CMD8 (0x40+8) // SEND_IF_COND
#define CMD9 (0x40+9) // SEND_CSD
#define CMD16 (0x40+16) // SET_BLOCKLEN
#define CMD17 (0x40+17) // READ_SINGLE_BLOCK
#define CMD24 (0x40+24) // WRITE_BLOCK
#define CMD55 (0x40+55) // APP_CMD
#define CMD58 (0x40+58) // READ_OCR
//--------------------------------------------------

typedef struct sd_info {  
	volatile uint8_t type;
} sd_info_ptr;
//--------------------------------------------------



void spi2Init(void);
void spi2Transmit(uint8_t data);
uint8_t spi2ReciveByte(void);
void spiRelease(void);
uint8_t spiWaitReady(void);
uint8_t sdReadBlock(uint8_t *buff, uint32_t lba);
uint8_t sdWriteBlock(uint8_t *buff,uint32_t lba);
uint8_t sdInit(void);

#endif
