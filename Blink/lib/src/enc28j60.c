#include "enc28j60.h"

static uint8_t Enc28j60Bank;
uint8_t macaddr[6]=MAC_ADDR;
static int gNextPacketPtr;

void spi1Init()
{
	//PA4-CS
	//PA5-SCK
	//PA6-MISO
	//PA7-MOSI
	RCC->APB2ENR|=RCC_APB2ENR_SPI1EN | RCC_APB2ENR_IOPAEN;//Enable clock spi1 and GPIOA
	RCC->AHBENR|=RCC_AHBENR_DMA1EN;//Enable DMA clock
	GPIOA->CRL|= GPIO_CRL_MODE4 | GPIO_CRL_MODE5 | GPIO_CRL_MODE7;//PA4,PA5,PA7 - Output mode max speen 50MHz
	GPIOA->CRL&= ~(GPIO_CRL_CNF4 | GPIO_CRL_CNF5 | GPIO_CRL_CNF6 | GPIO_CRL_CNF7);
	GPIOA->CRL|=  GPIO_CRL_CNF5_1 | GPIO_CRL_CNF6_1 | GPIO_CRL_CNF7_1;//PA5,PA7 - AF output push-pull;PA4 - output push-pull;PA6-Floating input
	SPI1->CR1|=SPI_CR1_MSTR;//Master mode
	SPI1->CR1|= (0x00 & SPI_CR1_BR_0);//fclk/4
	SPI1->CR1|=SPI_CR1_SSM;//Software slave select
	SPI1->CR1|=SPI_CR1_SSI;//NSS - high
	ENC_DESELECT;
	SPI1->CR1|=SPI_CR1_SPE;//SPI1 enable
}

uint8_t spiTranmittRecive(uint8_t data)
{
  while (!(SPI1->SR & SPI_SR_TXE));     
  SPI1->DR = data;                       
  while (!(SPI1->SR & SPI_SR_RXNE));     
  return (SPI1->DR);

}

void spiTransmit(uint8_t data)
{
	spiTranmittRecive(data);
}

uint8_t spiRecive()
{
  return spiTranmittRecive(0xFF);
}

void enc28j60WriteOp(uint8_t opcode,uint8_t addres, uint8_t data)
{
 ENC_SELECT;
 spiTransmit(opcode|(addres&ADDR_MASK));
 spiTransmit(data);
 ENC_DESELECT;
}

static uint8_t enc28j60ReadOp(uint8_t op,uint8_t addres)
{
 uint8_t result;
 ENC_SELECT;
 spiTransmit(op|(addres&ADDR_MASK));
 spiTransmit(0x00);
 //пропускаем ложный байт
  if(addres & 0x80) spiRecive();
 result=spiRecive();
 ENC_DESELECT;
 return result;
}

static void enc28j60ReadBuf(uint16_t len,uint8_t* data)
{
 ENC_SELECT;
 spiTransmit(ENC28J60_READ_BUF_MEM);
 while(len--){
  *data++=spiTranmittRecive(0x00);
 }
 ENC_DESELECT;
}

static void enc28j60WriteBuf(uint16_t len,uint8_t* data)

{
  ENC_SELECT;
  spiTransmit(ENC28J60_WRITE_BUF_MEM);
  while(len--)
  spiTransmit(*data++);
  ENC_DESELECT;

}

static void enc28j60SetBank(uint8_t addres)
{
 if ((addres&BANK_MASK)!=Enc28j60Bank)
 {
  enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR,ECON1,ECON1_BSEL1|ECON1_BSEL0);
  Enc28j60Bank = addres&BANK_MASK;
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET,ECON1,Enc28j60Bank>>5);
 }
}

static void enc28j60WriteRegByte(uint8_t addres,uint8_t data)
{
 enc28j60SetBank(addres);
 enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG,addres,data);
}

static uint8_t enc28j60ReadRegByte(uint8_t addres)
{
 enc28j60SetBank(addres);
 return enc28j60ReadOp(ENC28J60_READ_CTRL_REG,addres);
}

static void enc28j60WriteReg(uint8_t addres,uint16_t data)
{
 enc28j60WriteRegByte(addres, data);
 enc28j60WriteRegByte(addres+1, data>>8);
}

static void enc28j60WritePhy(uint8_t addres,uint16_t data)
{
  enc28j60WriteRegByte(MIREGADR, addres);
  enc28j60WriteReg(MIWR, data);
  while(enc28j60ReadRegByte(MISTAT)&MISTAT_BUSY) ;
}

void enc28j60Init()
{
	//PA3-Reset
	GPIOA->CRL|= GPIO_CRL_MODE3;
	GPIOA->CRL&= ~(GPIO_CRL_CNF3);
	ENC_RES_HIGH;
	enc28j60WriteOp(ENC28J60_SOFT_RESET,0,ENC28J60_SOFT_RESET);
	delay_ms(5);
	//проверим, что всё перезагрузилось
	while(!enc28j60ReadOp(ENC28J60_READ_CTRL_REG,ESTAT)&ESTAT_CLKRDY);
	//Настройка буферов
	enc28j60WriteReg(ERXST,RXSTART_INIT);
	enc28j60WriteReg(ERXRDPT,RXSTART_INIT);
	enc28j60WriteReg(ERXND,RXSTOP_INIT);
	enc28j60WriteReg(ETXST,TXSTART_INIT);
	enc28j60WriteReg(ETXND,TXSTOP_INIT);
	enc28j60WriteReg(ERXFCON,enc28j60ReadRegByte(ERXFCON)|ERXFCON_BCEN);//Включить broadcast
	//настраиваем канальный уровень
	enc28j60WriteRegByte(MACON1,MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	enc28j60WriteRegByte(MACON2,0x00);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET,MACON3,MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	enc28j60WriteReg(MAIPG,0x0C12);
	enc28j60WriteRegByte(MABBIPG,0x12);//промежуток между фреймами
	enc28j60WriteReg(MAMXFL,MAX_FRAMELEN);//максимальный размер фрейма
	enc28j60WriteRegByte(MAADR5,macaddr[0]);//Set MAC addres
	enc28j60WriteRegByte(MAADR4,macaddr[1]);
	enc28j60WriteRegByte(MAADR3,macaddr[2]);
	enc28j60WriteRegByte(MAADR2,macaddr[3]);
	enc28j60WriteRegByte(MAADR1,macaddr[4]);
	enc28j60WriteRegByte(MAADR0,macaddr[5]);
	//настраиваем физический уровень
	enc28j60WritePhy(PHCON2,PHCON2_HDLDIS);//отключаем loopback
	enc28j60WritePhy(PHLCON,PHLCON_LACFG2| //светодиоды
	PHLCON_LBCFG2|PHLCON_LBCFG1|PHLCON_LBCFG0|
	PHLCON_LFRQ0|PHLCON_STRCH);
	enc28j60SetBank(ECON1);
	//enc28j60WriteOp(ENC28J60_BIT_FIELD_SET,EIE,EIE_INTIE|EIE_PKTIE);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_RXEN);//разрешаем приём пакетов
  //Включим делитель частоты генератора 2, то есть частота будет 12,5 MHz
  enc28j60WriteRegByte(ECOCON,0x02);
  delay_ms(10);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_RXEN);//разрешаем приём пакетов
}

uint16_t enc28j60PacketReceive(uint8_t *buf,uint16_t buflen)
{
	struct
	{
		uint16_t nextPacket;
		uint16_t byteCount;
		uint16_t status;
	}header;
	uint16_t len=0;
	if(enc28j60ReadRegByte(EPKTCNT)>0)
	{
		enc28j60WriteReg(ERDPT,gNextPacketPtr);
		//считаем заголовок
		enc28j60ReadBuf(sizeof header,(uint8_t*)&header);
		gNextPacketPtr=header.nextPacket;
		len=header.byteCount-4;//remove the CRC count
		if(len>buflen) len=buflen;
		if((header.status&0x80)==0) len=0;
		else enc28j60ReadBuf(len, buf);
		buf[len]=0;
		if(gNextPacketPtr-1>RXSTOP_INIT)
			enc28j60WriteReg(ERXRDPT,RXSTOP_INIT);
		else
			enc28j60WriteReg(ERXRDPT,gNextPacketPtr-1);
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET,ECON2,ECON2_PKTDEC);
	}
	return len;
}

void enc28j60PacketSend(uint8_t *buf,uint16_t buflen)
{
  while(enc28j60ReadOp(ENC28J60_READ_CTRL_REG,ECON1)&ECON1_TXRTS)
  {
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_TXRST);
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR,ECON1,ECON1_TXRST);
  }
	enc28j60WriteReg(EWRPT,TXSTART_INIT);
  enc28j60WriteReg(ETXND,TXSTART_INIT+buflen);
  enc28j60WriteBuf(1,(uint8_t*)"x00");
  enc28j60WriteBuf(buflen,buf);
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_TXRTS);
	delay_ms(1);
}