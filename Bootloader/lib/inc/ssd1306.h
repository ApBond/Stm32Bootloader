#ifndef __SSD1306
#define __SSD1306

#include "stm32f10x.h"
#include "fonts.h"
#include "stdlib.h"

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define BUFFER_SIZE 1024

#define CS_SET GPIOA->BSRR|=GPIO_BSRR_BS2
#define CS_RES GPIOA->BSRR|=GPIO_BSRR_BR2
#define RESET_SET GPIOA->BSRR|=GPIO_BSRR_BS1
#define RESET_RES GPIOA->BSRR|=GPIO_BSRR_BR1
#define DATA GPIOA->BSRR|=GPIO_BSRR_BS3
#define COMMAND GPIOA->BSRR|=GPIO_BSRR_BR3

typedef enum HEAD_STAT
{
	ON,
	OFF
}HEAD_STAT;

typedef enum SCROLL_STAT
{
	UP,
	DOWN
}SCROLL_STAT;

typedef struct SSD1306_List
{
	struct SSD1306_ListElem *first;
	struct SSD1306_ListElem *currentElement;
	struct SSD1306_ListElem *printFirst;
	struct SSD1306_ListElem *ptintLast;
	uint8_t size;
	char *top;
	HEAD_STAT headStat;
	FontDef_t* font;
}SSD1306_List;

typedef struct SSD1306_ListElem
{
	struct SSD1306_ListElem *prev;
	struct SSD1306_ListElem *next;
	char *name;
	uint8_t id;
}SSD1306_ListElem;

static uint8_t displayBuff[BUFFER_SIZE];

void spi1Init(void);
void spiTransmit(uint8_t data);
void sendDMA(uint16_t size,uint8_t * data);
void ssd1306RunDisplayUPD(void);
void ssd1306StopDispayUPD(void);
void ssd1306SendCommand(uint8_t command);
void ssd1306SendData(uint8_t data);
void ssd1306Init(void);
void ssd1306DrawPixel(uint16_t x, uint16_t y,uint8_t color);
uint8_t getPixelStatus(uint16_t x, uint16_t y);
uint8_t ssd1306SetCursos(uint16_t x,uint16_t y);
char ssd1306Puts(char ch, FontDef_t* Font, uint8_t color);
void ssd1306Printf(char* str,FontDef_t* Font,uint8_t color);
void ssd1306PrintDigit(uint32_t dig,FontDef_t* Font,uint8_t color);
void ssd1306NextLine(FontDef_t* Font);
void ssd1306DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t c);
void ssd1306DrawRect(uint16_t x,uint16_t y,uint8_t width,uint8_t height,uint8_t color);
void ssd1306DrawCircle(uint16_t x0,uint16_t y0,uint8_t r,uint8_t color);
void ssd1306DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c);
void ssd1306ClearDisplay(uint8_t color);
void ssd1306ListAddElement(SSD1306_List *list,char* name,uint8_t id);
void ssd1306ListSetPosition(SSD1306_List *list,uint8_t pos);
void ssd1306PrintList(SSD1306_List *list);
void ssd1306ListInit(SSD1306_List *list,FontDef_t* Font, HEAD_STAT head);
void ssd1306ScrollList(SSD1306_List *list,SCROLL_STAT stat);
#endif
