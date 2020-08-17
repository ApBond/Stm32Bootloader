#include "sd.h"

sd_info_ptr sdinfo;
uint8_t str[30];

void spi2Init(void)
{
	//B15 - MOSI
	//B14 - MISO
	//B13 - SCK
	//B12 - CS
	RCC->APB2ENR|=RCC_APB2ENR_IOPBEN;//Enable clockGPIOB
	RCC->APB2ENR|=RCC_APB2ENR_ADC2EN;//AF clock
	RCC->APB1ENR|=RCC_APB1ENR_SPI2EN;//Enable SPI2 clock
	GPIOB->CRH|=  GPIO_CRH_MODE12 | GPIO_CRH_MODE13 | GPIO_CRH_MODE15;//PB15,PB13,PB12 - Output mode max speen 50MHz
	GPIOB->CRH&= ~(GPIO_CRH_CNF12 | GPIO_CRH_CNF13 | GPIO_CRH_CNF14 | GPIO_CRH_CNF15);
	GPIOB->CRH|=  GPIO_CRH_CNF13_1 | GPIO_CRH_CNF14_0 | GPIO_CRH_CNF15_1;//PB15,PB13 - AF output push-pull,PB13 - input with pullup
	//GPIOB->ODR|=GPIO_ODR_ODR14;//B14-Pull up
	SPI2->CR1|=SPI_CR1_MSTR;//Master mode
	SPI2->CR1|= SPI_CR1_BR_1;//fclk/8
	SPI2->CR1|=SPI_CR1_SSM;//Software slave select
	SPI2->CR1|=SPI_CR1_SSI;//NSS - high
	SD_DESELECT;
	SPI2->CR1|=SPI_CR1_SPE;//SPI1 enable
}

uint8_t SpiTranmittRecive(uint8_t data)
{
  while (!(SPI2->SR & SPI_SR_TXE));     
  SPI2->DR = data;                       
  while (!(SPI2->SR & SPI_SR_RXNE));     
  return (SPI2->DR);
}

void spi2Transmit(uint8_t data)
{
	SpiTranmittRecive(data);
}

uint8_t spi2ReciveByte()
{
  return SpiTranmittRecive(0xFF);
}

void spiRelease()
{
	spi2Transmit(0xFF);
}

uint8_t spiWaitReady(void)
{
  uint8_t res;
  uint16_t cnt;
  cnt=0;
  do 
	{
    res=spi2ReciveByte();
    cnt++;
  } while ( (res!=0xFF)&&(cnt<0xFFFF) );
  if (cnt>=0xFFFF) return 1;
  return res;
}

uint8_t sdSendCmd(uint8_t cmd,uint32_t arg)
{
	uint8_t n, res;
	if (cmd & 0x80)
	{
  cmd &= 0x7F;
  res = sdSendCmd(CMD55, 0);
  if (res > 1) return res;
	}
	SD_DESELECT;
	spi2ReciveByte();
	SD_SELECT;
	spi2ReciveByte();
	// Send a command packet
	spi2Transmit(cmd); // Start + Command index
	spi2Transmit((uint8_t)(arg >> 24)); // Argument[31..24]
	spi2Transmit((uint8_t)(arg >> 16)); // Argument[23..16]
	spi2Transmit((uint8_t)(arg >> 8)); // Argument[15..8]
	spi2Transmit((uint8_t)arg); // Argument[7..0]
	n = 0x01; // Dummy CRC + Stop
	if (cmd == CMD0) {n = 0x95;} // Valid CRC for CMD0(0)
	if (cmd == CMD8) {n = 0x87;} // Valid CRC for CMD8(0x1AA)
	spi2Transmit(n);
	// Receive a command response
  n = 10; // Wait for a valid response in timeout of 10 attempts
  do {
    res = spi2ReciveByte();
  } while ((res & 0x80) && --n);
	//ssd1306PrintDigit(res,&Font_11x18,1);
  return res;
}

uint8_t sdReadBlock(uint8_t *buff, uint32_t lba)
{
	uint8_t result;
  uint16_t cnt;
	result=sdSendCmd(CMD17, lba);
	if (result!=0x00) return 5;
	spiRelease();
	do
	{
    result=spi2ReciveByte();
    cnt++;
  } while ( (result!=0xFE)&&(cnt<0xFFFF) );
  if (cnt>=0xFFFF) return 5;
  for (cnt=0;cnt<512;cnt++) buff[cnt]=spi2ReciveByte();
	spiRelease();
	spiRelease();
  return 0;
}

uint8_t sdWriteBlock(uint8_t *buff,uint32_t lba)
{
	uint8_t result;
  uint16_t cnt;
  result=sdSendCmd(CMD24,lba);
  if (result!=0x00) return 6;
  spiRelease();
  spi2Transmit(0xFE);
  for (cnt=0;cnt<512;cnt++) spi2Transmit(buff[cnt]);
  spiRelease();
  spiRelease();
  result=spi2ReciveByte();
  if ((result&0x05)!=0x05) return 6;
  cnt=0;
  do 
	{
    result=spi2ReciveByte();
    cnt++;
  } while ( (result!=0xFF)&&(cnt<0xFFFF) );
  if (cnt>=0xFFFF) return 6;
  return 0;
}

uint8_t sdInit()
{
	uint8_t i,cmd;
	uint32_t tmr;
	uint8_t ocr[4];
	for(i=0;i<10;i++) 
		spiRelease();
	SD_SELECT;
  if (sdSendCmd(CMD0, 0) == 1) // Enter Idle state
  {
		uart1Printf("OK,\n\r");
		spiRelease();
		if (sdSendCmd(CMD8, 0x1AA) == 1) // SDv2
		{
			uart1Printf("Type1  ,");
			for (i = 0; i < 4; i++) 
			{
				ocr[i] = spi2ReciveByte();
				//ssd1306PrintDigit(ocr[i],&Font_7x10,1);
				//ssd1306Puts(',',&Font_7x10,1);
			}
			// Get trailing return value of R7 resp
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) // The card can work at vdd range of 2.7-3.6V
			{
				for (tmr = 12000; tmr && sdSendCmd(ACMD41, 1UL << 30); tmr--)
					; // Wait for leaving idle state (ACMD41 with HCS bit)
				if (tmr && sdSendCmd(CMD58, 0) == 0) // Check CCS bit in the OCR
				{ 
					uart1Printf("OCR:");
					for (i = 0; i < 4; i++) 
					{
						ocr[i] = spi2ReciveByte();
						sprintf(str,"%u\n\r");
						//ssd1306PrintDigit(ocr[i],&Font_7x10,1);
						uart1Printf(str);
					}
					sdinfo.type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; // SDv2 (HC or SC)
				}
			}
		}
		else //SDv1 or MMCv3
		{
			//ssd1306Printf("Type2,",&Font_7x10,1);
			if (sdSendCmd(ACMD41, 0) <= 1)
			{
				sdinfo.type = CT_SD1; cmd = ACMD41; // SDv1
			}
			else
			{
				sdinfo.type = CT_MMC; cmd = CMD1; // MMCv3
			}
			for (tmr = 25000; tmr && sdSendCmd(cmd, 0); tmr--) ; // Wait for leaving idle state
			if (!tmr || sdSendCmd(CMD16, 512) != 0) // Set R/W block length to 512
				sdinfo.type = 0;
		}
  }
  else
  {
    return 1;
  }
	//ssd1306Printf("CardType:",&Font_7x10,1);
	//ssd1306PrintDigit(sdinfo.type,&Font_7x10,1);
  return 0;
}
