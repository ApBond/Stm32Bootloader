#include "ssd1306.h"

typedef struct {
	uint16_t CurrentX;
	uint16_t CurrentY;
} SSD1306_t;

SSD1306_t ssd1306 = {0,0};


SSD1306_ListElem* printFirst = 0;
SSD1306_ListElem* printLast = 0;

void spi1Init()
{
	//PA5-SCK
	//PA7-MOSI
	RCC->APB2ENR|=RCC_APB2ENR_SPI1EN | RCC_APB2ENR_IOPAEN;//Enable clock spi1 and GPIOA
	RCC->AHBENR|=RCC_AHBENR_DMA1EN;//Enable DMA clock
	GPIOA->CRL|= GPIO_CRL_MODE5 | GPIO_CRL_MODE7;//PA4,PA5,PA7 - Output mode max speen 50MHz
	GPIOA->CRL&= ~(GPIO_CRL_CNF5 | GPIO_CRL_CNF7);
	GPIOA->CRL|=  GPIO_CRL_CNF5_1 | GPIO_CRL_CNF7_1;//PA5,PA7 - AF output push-pull;PA4 - output push-pull
	SPI1->CR1|=SPI_CR1_MSTR;//Master mode
	SPI1->CR1|= (0x00 & SPI_CR1_BR);//fclk/2
	SPI1->CR1|=SPI_CR1_SSM;//Software slave select
	SPI1->CR1|=SPI_CR1_SSI;//NSS - high
	CS_SET;
	SPI1->CR2|=SPI_CR2_TXDMAEN;//Enable DMA request
	SPI1->CR1|=SPI_CR1_SPE;//SPI1 enable
	//DMA settings
	DMA1_Channel3->CCR|=DMA_CCR1_PSIZE_0;
	DMA1_Channel3->CCR|=DMA_CCR1_DIR;//Read from memory
	DMA1_Channel3->CCR|=DMA_CCR1_MINC;//Memory increment enable
	DMA1_Channel3->CCR|=DMA_CCR1_PL;//Very high prioryty
	//DMA1_Channel3->CCR|=DMA_CCR1_TCIE;//Enable interrupt "End of transmitt"
	//DMA1->IFCR&=~DMA_IFCR_CTCIF3;//Reset flag
	//NVIC_EnableIRQ(DMA1_Channel3_IRQn);//Enable interrupt
	
}

void spiTransmit(uint8_t data)
{
	CS_RES;	
	SPI1->DR = data;
	while((SPI1->SR & SPI_SR_BSY))
	{};
	CS_SET;
}

void sendDMA(uint16_t size,uint8_t * data)
{
	DMA1_Channel3->CCR&=~(DMA_CCR1_EN);//Disable DMA
	DMA1_Channel3->CPAR=(uint32_t)(&SPI1->DR);//Adredd DR of SPI1
	DMA1_Channel3->CMAR=(uint32_t)data;//Data adress
	DMA1_Channel3->CNDTR=size;//Data size
	DMA1->IFCR&=~(DMA_IFCR_CGIF3);
	DMA1_Channel3->CCR|=DMA_CCR1_EN;//Channel enable
}

void ssd1306RunDisplayUPD()
{
	DMA1_Channel3->CCR&=~(DMA_CCR1_EN);//Disable DMA
	DMA1_Channel3->CPAR=(uint32_t)(&SPI1->DR);//Adredd DR of SPI1
	DMA1_Channel3->CMAR=(uint32_t)&displayBuff;//Data adress
	DMA1_Channel3->CNDTR=sizeof(displayBuff);//Data size
	DMA1->IFCR&=~(DMA_IFCR_CGIF3);
	CS_RES;
	DMA1_Channel3->CCR|=DMA_CCR1_CIRC;//Circular mode enable
	DMA1_Channel3->CCR|=DMA_CCR1_EN;
}

void ssd1306StopDispayUPD()
{
	CS_SET;
	DMA1_Channel3->CCR&=~(DMA_CCR1_EN);//Disable DMA
	DMA1_Channel3->CCR&=~DMA_CCR1_CIRC;//Circular mode disable
}

void ssd1306SendCommand(uint8_t command)
{
	COMMAND;
	spiTransmit(command);
	DATA;
}

void ssd1306SendData(uint8_t data)
{
	spiTransmit(data);
}

void ssd1306Init()
{
	//PA1-reset
	//PA2 - CS
	//PA3-data/command
	uint16_t i;
	GPIOA->CRL|= GPIO_CRL_MODE2 |GPIO_CRL_MODE1 | GPIO_CRL_MODE3;//PA1,PA2,PA3 - Output mode max speen 50MHz
	GPIOA->CRL&= ~(GPIO_CRL_CNF1 | GPIO_CRL_CNF2 | GPIO_CRL_CNF3);
	RESET_RES;
	for(i=0;i<BUFFER_SIZE;i++)
	{
		displayBuff[i]=0;
	}
	RESET_SET;
	DATA;
	ssd1306SendCommand(0xAE); //display off
	ssd1306SendCommand(0xD5); //Set Memory Addressing Mode
	ssd1306SendCommand(0x80); //00,Horizontal Addressing Mode;01,Vertical
	ssd1306SendCommand(0xA8); //Set Page Start Address for Page Addressing
	ssd1306SendCommand(0x3F); //Set COM Output Scan Direction
	ssd1306SendCommand(0xD3); //---set low column address
	ssd1306SendCommand(0x00); //---set high column address
	ssd1306SendCommand(0x40); //--set start line address
	ssd1306SendCommand(0x8D); //--set contrast control register
	ssd1306SendCommand(0x14);
	ssd1306SendCommand(0x20); //--set segment re-map 0 to 127
	ssd1306SendCommand(0x00); //--set normal display
	ssd1306SendCommand(0xA1); //--set multiplex ratio(1 to 64)
	ssd1306SendCommand(0xC8); //
	ssd1306SendCommand(0xDA); //0xa4,Output follows RAM
	ssd1306SendCommand(0x12); //-set display offset
	ssd1306SendCommand(0x81); //-not offset
	ssd1306SendCommand(0x8F); //--set display clock divide ratio/oscillator frequency
	ssd1306SendCommand(0xD9); //--set divide ratio
	ssd1306SendCommand(0xF1); //--set pre-charge period
	ssd1306SendCommand(0xDB); //
	ssd1306SendCommand(0x40); //--set com pins hardware configuration
	ssd1306SendCommand(0xA4);
	ssd1306SendCommand(0xA6); //--set vcomh
	ssd1306SendCommand(0xAF); //0x20,0.77xVcc
}

void ssd1306DrawPixel(uint16_t x, uint16_t y,uint8_t color){
	if(x<SSD1306_WIDTH && y <SSD1306_HEIGHT && x>=0 && y>=0)
	{
		if(color==1)
		{
			displayBuff[x+(y/8)*SSD1306_WIDTH]|=(1<<(y%8));
		}
		else
		{
			displayBuff[x+(y/8)*SSD1306_WIDTH]&=~(1<<(y%8));
		}
	}
}

uint8_t getPixelStatus(uint16_t x, uint16_t y)
{
	uint16_t temp1=displayBuff[x+(y/8)*SSD1306_WIDTH];
	uint16_t temp = (displayBuff[x+(y/8)*SSD1306_WIDTH]>>(y%8))&0x01;
	return (displayBuff[x+(y/8)*SSD1306_WIDTH]>>(y%8))&0x01;
}

uint8_t ssd1306SetCursos(uint16_t x,uint16_t y){
	if(x<=SSD1306_WIDTH && y<=SSD1306_HEIGHT)
	{
		ssd1306.CurrentX=x;
		ssd1306.CurrentY=y;
		return 1;
	}
	else
	{
		return 0;
	}
}

char ssd1306Puts(char ch, FontDef_t* Font, uint8_t color) {
	uint32_t i, b, j;
	if(SSD1306_WIDTH <= (ssd1306.CurrentX + Font->FontWidth))
	{
		if((ssd1306.CurrentY + Font->FontHeight)>SSD1306_HEIGHT)
		{
			return 0;
		}
		else
		{
			ssd1306.CurrentY+=Font->FontHeight;
			ssd1306.CurrentX=0;
		}
	}
	if (SSD1306_HEIGHT <= (ssd1306.CurrentY + Font->FontHeight))
	{
		return 0;
	}
	for (i = 0; i < Font->FontHeight; i++) {
		b = Font->data[(ch - 32) * Font->FontHeight + i];
		for (j = 0; j < Font->FontWidth; j++) {
			if ((b << j) & 0x8000) {
				if(color==1)
				{
					ssd1306DrawPixel(ssd1306.CurrentX + j, (ssd1306.CurrentY + i), 1);
				}
				else
				{
					ssd1306DrawPixel(ssd1306.CurrentX + j, (ssd1306.CurrentY + i), 0);
				}
			} else {
				if(color==1)
				{
					ssd1306DrawPixel(ssd1306.CurrentX + j, (ssd1306.CurrentY + i), 0);
				}
				else
				{
					ssd1306DrawPixel(ssd1306.CurrentX + j, (ssd1306.CurrentY + i), 1);
				}
			}
		}
	}
	ssd1306.CurrentX += Font->FontWidth;
	return ch;
}

void ssd1306Printf(char* str,FontDef_t* Font,uint8_t color){
	while(*str)
	{
		if(*str=='\n')
		{
			ssd1306SetCursos(0,ssd1306.CurrentY+Font->FontHeight);
		}
		else
		{
			if(*str>0x1F)
				ssd1306Puts(*str,Font,color);
		}
		str++;
	}
}

void ssd1306PrintDigit(uint32_t dig,FontDef_t* Font,uint8_t color)
{
	uint8_t i,temp,j;
	char buff[10]={0,0,0,0,0,0,0,0,0,0};
	for (i=0; i<10; i++)
  {
		buff[i] = '0' + dig % 10;
		dig /= 10;
		if(dig==0)
		{
			break;
		}
  }
	for(j=0;j<i;j++)
	{
		temp=buff[i];
		buff[i]=buff[j];
		buff[j]=temp;
		i--;
	}
	ssd1306Printf(buff,Font,color);
}

void ssd1306NextLine(FontDef_t* Font)
{
	ssd1306SetCursos(0,ssd1306.CurrentY+Font->FontHeight);
}

void ssd1306DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t c) {
	int16_t dx, dy, sx, sy, err, e2, i, tmp; 
	/* Check for overflow */
	if (x0 >= SSD1306_WIDTH) {
		x0 = SSD1306_WIDTH - 1;
	}
	if (x1 >= SSD1306_WIDTH) {
		x1 = SSD1306_WIDTH - 1;
	}
	if (y0 >= SSD1306_HEIGHT) {
		y0 = SSD1306_HEIGHT - 1;
	}
	if (y1 >= SSD1306_HEIGHT) {
		y1 = SSD1306_HEIGHT - 1;
	}	
	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1); 
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1); 
	sx = (x0 < x1) ? 1 : -1; 
	sy = (y0 < y1) ? 1 : -1; 
	err = ((dx > dy) ? dx : -dy) / 2; 
	if (dx == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}	
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}	
		/* Vertical line */
		for (i = y0; i <= y1; i++) {
			ssd1306DrawPixel(x0, i, c);
		}		
		/* Return from function */
		return;
	}	
	if (dy == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		/* Horizontal line */
		for (i = x0; i <= x1; i++) {
			ssd1306DrawPixel(i, y0, c);
		}
		
		/* Return from function */
		return;
	}
	while (1) {
		ssd1306DrawPixel(x0, y0, c);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err; 
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		} 
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		} 
	}
}

void ssd1306DrawRect(uint16_t x,uint16_t y,uint8_t width,uint8_t height,uint8_t color)
{
	uint8_t i,j;
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			ssd1306DrawPixel(x+j,y+i,color);
		}
	}
}

void ssd1306DrawCircle(uint16_t x0,uint16_t y0,uint8_t r,uint8_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;
  ssd1306DrawPixel(x0, y0 + r, color);
  ssd1306DrawPixel(x0, y0 - r, color);
  ssd1306DrawPixel(x0 + r, y0, color);
  ssd1306DrawPixel(x0 - r, y0, color);
  while (x < y) {
		if (f >= 0) {
			y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    ssd1306DrawPixel(x0 + x, y0 + y, color);
    ssd1306DrawPixel(x0 - x, y0 + y, color);
    ssd1306DrawPixel(x0 + x, y0 - y, color);
    ssd1306DrawPixel(x0 - x, y0 - y, color);
    ssd1306DrawPixel(x0 + y, y0 + x, color);
    ssd1306DrawPixel(x0 - y, y0 + x, color);
    ssd1306DrawPixel(x0 + y, y0 - x, color);
    ssd1306DrawPixel(x0 - y, y0 - x, color);
	}
}

void ssd1306DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;
  ssd1306DrawPixel(x0, y0 + r, c);
  ssd1306DrawPixel(x0, y0 - r, c);
  ssd1306DrawPixel(x0 + r, y0, c);
  ssd1306DrawPixel(x0 - r, y0, c);
  ssd1306DrawLine(x0 - r, y0, x0 + r, y0, c);
  while (x < y) {
      if (f >= 0) 
				{
          y--;
          ddF_y += 2;
          f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        ssd1306DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
        ssd1306DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);
        ssd1306DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
        ssd1306DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
	}
}

void ssd1306ClearDisplay(uint8_t color)
{
	uint16_t i;
	ssd1306.CurrentX=0;
	ssd1306.CurrentY=0;
	for(i=0;i<SSD1306_HEIGHT*SSD1306_WIDTH/8;i++)
	{
		if(color==1)
			displayBuff[i]=0xFF;
		else
			displayBuff[i]=0;
	}
	return;
}

void ssd1306ListAddElement(SSD1306_List *list,char* name,uint8_t id)
{
	SSD1306_ListElem* temp = (SSD1306_ListElem*)malloc(sizeof(SSD1306_ListElem));
	temp->name = name;
	temp->id=id;
	if(list->size==0)
	{
		temp->next=0;
		temp->prev=0;
		list->first=temp;
		list->currentElement=temp;
	}
	else
	{
		temp->next=list->currentElement->next;
		temp->prev=list->currentElement;
		list->currentElement->next=temp;
	}
	list->currentElement=temp;
	list->size++;
}

void ssd1306ListSetPosition(SSD1306_List *list,uint8_t pos)
{
	if(list->size>=pos)
	{
		uint8_t i=0;
		SSD1306_ListElem* temp=list->first;
		while(i!=pos)
		{
			temp=temp->next;
			i++;
		}
		list->currentElement=temp;
	}
}

void ssd1306PrintList(SSD1306_List *list)
{
	SSD1306_ListElem* temp;
	ssd1306.CurrentX=0;
	ssd1306.CurrentY=0;
	if(list->headStat==ON)
	{
		ssd1306Printf(list->top,list->font,1);
		ssd1306DrawLine(0,ssd1306.CurrentY+list->font->FontHeight,SSD1306_WIDTH,ssd1306.CurrentY+list->font->FontHeight,1);
		ssd1306SetCursos(0,ssd1306.CurrentY+list->font->FontHeight+3);
	}
	temp = list->printFirst;
	while(1)
	{
		if(temp==list->currentElement)
		{
			ssd1306DrawRect(0,ssd1306.CurrentY,SSD1306_WIDTH,list->font->FontHeight,1);
			ssd1306Printf(temp->name,list->font,0);
		}
		else
		{
			ssd1306DrawRect(0,ssd1306.CurrentY,SSD1306_WIDTH,list->font->FontHeight,0);
			ssd1306Printf(temp->name,list->font,1);
		}
		ssd1306SetCursos(0,ssd1306.CurrentY+list->font->FontHeight);
		if(temp==list->ptintLast) break;
		temp=temp->next;
	}
}

void ssd1306ListInit(SSD1306_List *list,FontDef_t* Font, HEAD_STAT head)
{
	uint16_t i,strCount=(SSD1306_HEIGHT/Font->FontHeight)-1;
	SSD1306_ListElem* temp;
	list->currentElement=list->first;
	list->font = Font;
	list->headStat = head;
	if(head==ON)
	{
		strCount--;
	}
	temp=list->first;
	list->printFirst=temp;
	for(i=0;i<strCount;i++)
	{
		temp=temp->next;
		if(i>list->size-3) break;
	}
	list->ptintLast=temp;
	ssd1306PrintList(list);
}

void ssd1306ScrollList(SSD1306_List *list,SCROLL_STAT stat)
{
	if(stat==UP)
	{
		if(list->currentElement!=list->first)
		{
			list->currentElement = list->currentElement->prev;
			if(list->printFirst!=list->first && list->currentElement->next==list->printFirst)
			{
				list->printFirst=list->printFirst->prev;
				list->ptintLast=list->ptintLast->prev;
			}
			ssd1306PrintList(list);
		}
	}
	else if(stat==DOWN)
	{
		if(list->currentElement->next!=0)
		{
			list->currentElement = list->currentElement->next;
			if(list->ptintLast->next!=0 && list->currentElement->prev==list->ptintLast)
			{
				list->ptintLast=list->ptintLast->next;
				list->printFirst=list->printFirst->next;
			}
			ssd1306PrintList(list);
		}
	}
}