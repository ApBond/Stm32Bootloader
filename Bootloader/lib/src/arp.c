#include "arp.h"
extern uint8_t macaddr[6];
extern uint8_t ipaddr[4];
extern uint8_t netBuf[ENC28J60_MAXFRAME];
extern uint8_t netMask[4];
uint8_t broadcastMac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t arpRequestMac[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
ArpTable arpTbl[ARP_TABLE_SIZE];
uint8_t arpPointer = 0;

uint8_t* arpTableSearch(uint8_t* ipAdr)
{
	uint8_t i;
	for(i=0;i<ARP_TABLE_SIZE;i++)
	{
		if(!memcmp(arpTbl[i].ipAdr,ipAdr,4))
			return arpTbl[i].macaddr;
	}
	return 0;
}

void arpAdd(uint8_t* ipAdr,uint8_t* macAdr)
{
	if(!(arpTableSearch(ipAdr)))
	{
		memcpy(arpTbl[arpPointer].ipAdr,ipAdr,4);
		memcpy(arpTbl[arpPointer].macaddr,macAdr,6);
		arpPointer++;
		(arpPointer==ARP_TABLE_SIZE) ? (arpPointer=0):0;						
	}
}

uint8_t* arpResolve(uint8_t* ipAdr)
{
	enc28j60FramePtr *frame = (void*)netBuf;
	arpMsgPtr *arpMsg = (void*)frame->data;
	uint8_t* mac;
	mac=arpTableSearch(ipAdr);
	if(mac)
		return mac;
	arpMsg->protoTp=ARP_IP;
	arpMsg->netTp=ARP_ETH;
	arpMsg->op=ARP_REQUEST;
	arpMsg->ipaddrLen=4;
	arpMsg->macaddrLen=6;
	memcpy(arpMsg->ipaddrDst,ipAdr,4);
	memcpy(arpMsg->ipaddrSrc,ipaddr,4);
	memcpy(arpMsg->macaddrSrc,macaddr,6);
	memcpy(arpMsg->macaddrDst,arpRequestMac,6);
	frame->type=ETH_ARP;
	memcpy(frame->addrDest,broadcastMac,6);
	memcpy(frame->addrSrc,macaddr,6);
	ethSend(frame,sizeof(arpMsgPtr));
}

uint8_t arpFilter(enc28j60FramePtr *frame, uint16_t len)
{
	uint8_t res=0;
	arpMsgPtr *arpMsg=(void*)(frame->data);
	if(len>=sizeof(arpMsgPtr))
	{
		if((arpMsg->netTp==ARP_ETH)&&(arpMsg->protoTp==ARP_IP)&&(!memcmp(arpMsg->ipaddrDst,ipaddr,4)))
		{
			switch(arpMsg->op)
			{
				case ARP_REQUEST:
					arpMsg->op = ARP_REPLY;
					arpAdd(arpMsg->ipaddrSrc,arpMsg->macaddrSrc);
					memcpy(arpMsg->macaddrDst,arpMsg->macaddrSrc,6);
					memcpy(arpMsg->macaddrSrc,macaddr,6);
					memcpy(arpMsg->ipaddrDst,arpMsg->ipaddrSrc,4);
					memcpy(arpMsg->ipaddrSrc,ipaddr,4);
				  memcpy(frame->addrDest,frame->addrSrc,6);
					memcpy(frame->addrSrc,macaddr,6);
					ethSend(frame,sizeof(arpMsgPtr));
				case ARP_REPLY:
					arpAdd(arpMsg->ipaddrSrc,arpMsg->macaddrSrc);
					break;
			}
		}
	}
}
