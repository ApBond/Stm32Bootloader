#ifndef ARP_H
#define ARP_H

//--------------------------------------------------
#include "stm32f10x.h"
#include "enc28j60.h"
#include "uart.h"
#include "net.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
//--------------------------------------------------
#define ARP_ETH be16toword(0x0001)
#define ARP_IP be16toword(0x0800)
#define ARP_REQUEST be16toword(1)
#define ARP_REPLY be16toword(2)
#define ARP_TABLE_SIZE 5
//--------------------------------------------------
typedef struct arpMsg{
  uint16_t netTp;
  uint16_t protoTp;
  uint8_t macaddrLen;
  uint8_t ipaddrLen;
  uint16_t op;
  uint8_t macaddrSrc[6];
  uint8_t ipaddrSrc[4];
  uint8_t macaddrDst[6];
  uint8_t ipaddrDst[4];
} arpMsgPtr;

typedef struct ArpTable
{
	uint8_t ipAdr[4];
	uint8_t macaddr[6];
}ArpTable;
//--------------------------------------------------
void arpSend(enc28j60FramePtr *frame);
uint8_t arpRead(enc28j60FramePtr *frame, uint16_t len);
uint8_t* arpResolve(uint8_t* ipAdr);
uint8_t arpFilter(enc28j60FramePtr *frame, uint16_t len);
void arpAdd(uint8_t* ipAdr,uint8_t* macAdr);
//--------------------------------------------------
#endif
