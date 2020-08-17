#ifndef UDP_H
#define UDP_H

//--------------------------------------------------
#include "stm32f10x.h"
#include "enc28j60.h"
#include "uart.h"
#include "net.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
//--------------------------------------------------
typedef struct udpPacket {
    uint16_t portSrc;
    uint16_t portDst;
    uint16_t len;
    uint16_t cksum;
    uint8_t data[];
} udpPacketPtr;
//--------------------------------------------------
uint8_t udpRead(enc28j60FramePtr *frame, uint16_t len);
IP_STATUS udpSend(uint8_t* data,uint16_t len,uint8_t* ipDst,uint16_t portDst,uint16_t portSrc);
//--------------------------------------------------
#endif
