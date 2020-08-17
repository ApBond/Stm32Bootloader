#ifndef TCP_H
#define TCP_H

//--------------------------------------------------
#include "stm32f10x.h"
#include "enc28j60.h"
#include "uart.h"
#include "net.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
//--------------------------------------------------
//флаги TCP
#define TCP_CWR 0x80
#define TCP_ECE 0x40
#define TCP_URG 0x20
#define TCP_ACK 0x10
#define TCP_PSH 0x08
#define TCP_RST 0x04
#define TCP_SYN 0x02
#define TCP_FIN 0x01
//--------------------------------------------------
//операции TCP
typedef enum TCP_OPERATION
{
	TCP_OP_SYNACK,
	TCP_OP_ACK_OF_FIN,
	TCP_OP_ACK_OF_RST,
	TCP_OP_ACK_OF_DATA 
}TCP_OPERATION;

typedef enum TCP_STAT_CODE
{
	TCP_CLOSED,
  TCP_SYN_SENT,
  TCP_SYN_RECEIVED,
  TCP_ESTABLISHED,
  TCP_FIN_WAIT
}TCP_STAT_CODE;

typedef enum TCP_POOL
{
	POOL,
	NOT_POOL
}TCP_POOL;

typedef enum TCP_RECIVE_STAT
{
	RECIVED,
	NOT_RECIVE
}TCP_RECIVE_STAT;

typedef struct tcpState
{
	uint8_t id;
	TCP_STAT_CODE state;
	uint16_t srcPort;
	uint16_t dstPort;
	uint8_t dstIp[4];
	TCP_POOL poolStat;
	uint32_t btNumSeg;
	uint32_t numAsk;
	TCP_RECIVE_STAT resStat;
	uint16_t tcpDataLen;
	uint8_t* data;
}tcpState;
//--------------------------------------------------
#define LOCAL_PORT_TCP 80
#define TCP_MAX_CONNECTION 2
#define TCP_MAX_PKT_SIZE 512
#define TCP_BUFFER_SIZE 1024
//--------------------------------------------------
typedef struct tcpPkt {
  uint16_t portSrc;//порт отправителя
  uint16_t portDst;//порт получателя
  uint32_t btNumSeg;//порядковый номер байта в потоке данных (указатель на первый байт в сегменте данных)
  uint32_t numAsk;//номер подтверждения (первый байт в сегменте + количество байтов в сегменте + 1 или номер следующего ожидаемого байта)
  uint8_t lenHdr;//длина заголовка
  uint8_t fl;//флаги TCP
  uint16_t sizeWnd;//размер окна
  uint16_t cs;//контрольная сумма заголовка
  uint16_t urgPtr;//указатель на срочные данные
  uint8_t data[];//данные
} tcpPktPtr;
//--------------------------------------------------
uint8_t tcpCreate(uint16_t portSrc);
void tcpListen(uint8_t id);
void tcpPool(enc28j60FramePtr *frame);
uint8_t* tcpRead(uint8_t id,uint16_t* len);
void tcpSendData(uint8_t id,uint8_t* data,uint16_t len);
void tcpClose(uint8_t id);
TCP_STAT_CODE getTcpStatus(uint8_t id);
//--------------------------------------------------
#endif
