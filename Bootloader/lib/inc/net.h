#ifndef NET_H
#define NET_H

//--------------------------------------------------
#include "stm32f10x.h"
#include "enc28j60.h"
#include "uart.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
//--------------------------------------------------
typedef struct enc28j60Frame{
  uint8_t addrDest[6];
  uint8_t addrSrc[6];
  uint16_t type;
  uint8_t data[];
} enc28j60FramePtr;
//--------------------------------------------------
#define IP_ADDR {192,168,0,15}
#define be16toword(a) ((((a)>>8)&0xff)|(((a)<<8)&0xff00))
#define be32todword(a) ((((a)>>24)&0xff)|(((a)>>8)&0xff00)|(((a)<<8)&0xff0000)|(((a)<<24)&0xff000000))
#define ETH_ARP be16toword(0x0806)
#define ETH_IP be16toword(0x0800)
//Ip protocol
#define IP_ICMP 1
#define IP_TCP 6
#define IP_UDP 17

typedef struct ipPkt{
	uint8_t verlen;//версия протокола и длина заголовка
	uint8_t ts;//тип севриса
	uint16_t len;//длина
	uint16_t id;//идентификатор пакета
	uint16_t flFrgOf;//флаги и смещение фрагмента
	uint8_t ttl;//время жизни
	uint8_t prt;//тип протокола
	uint16_t cs;//контрольная сумма заголовка
	uint8_t ipaddrSrc[4];//IP-адрес отправителя
	uint8_t ipaddrDst[4];//IP-адрес получателя
	uint8_t data[];//данные
} ipPktPtr;

//--------------------------------------------------
//ICMP protocol
#define ICMP_REQ 8
#define ICMP_REPLY 0

typedef struct icmpPkt{
	uint8_t msgTp;//тип севриса
	uint8_t msgCd;//код сообщения
	uint16_t cs;//контрольная сумма заголовка
	uint16_t id;//идентификатор пакета
	uint16_t num;//номер пакета
	uint8_t data[];//данные
} icmpPktPtr;
//--------------------------------------------------
typedef enum IP_STATUS
{
	SEND_ERROR,
	SEND_COMPL
}IP_STATUS;

typedef enum IP_TYPE
{
	UDP=17,
	TCP=6,
	ICMP=1,
	IP=0
}IP_TYPE;
//--------------------------------------------------
void netInit(void);
void netPool(void);
uint8_t ipReSend(enc28j60FramePtr *frame, uint16_t len);
IP_STATUS ipSend(enc28j60FramePtr *frame, uint16_t len,IP_TYPE protocol);
void ethReSend(enc28j60FramePtr *frame, uint16_t len);
void ethSend(enc28j60FramePtr *frame, uint16_t len);
uint16_t checksum(uint8_t *ptr, uint16_t len,IP_TYPE protocol);

#include "arp.h"
#include "tcp.h"
#endif

