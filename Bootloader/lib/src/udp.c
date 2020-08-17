#include "udp.h"
extern uint8_t netBuf[ENC28J60_MAXFRAME];
char str2[80]={0};

IP_STATUS udpSend(uint8_t* data,uint16_t len,uint8_t* ipDst,uint16_t portDst,uint16_t portSrc)
{
	enc28j60FramePtr *frame = (void*)netBuf;
	ipPktPtr *ipPkt = (void*)frame->data;
	udpPacketPtr *udpPkt = (void*)ipPkt->data;
	memcpy(ipPkt->ipaddrDst,ipDst,4);
	udpPkt->portDst=be16toword(portDst);
	udpPkt->portSrc=be16toword(portSrc);
	udpPkt->len=be16toword(len+sizeof(udpPacketPtr));
	memcpy(udpPkt->data,data,len);
	udpPkt->cksum=0;
	return ipSend(frame,len+sizeof(udpPacketPtr),UDP);
}

uint8_t udpReply(enc28j60FramePtr *frame)
{
	uint8_t res=0;
	uint16_t port,len;
	ipPktPtr *ipPkt = (void*)frame->data;
	udpPacketPtr *udpPkt = (void*)ipPkt->data;
	/*/port=udpPkt->portDst;
	udpPkt->portDst=udpPkt->portSrc;
	udpPkt->portSrc=port;
	strcpy((char*)udpPkt->data,"UDP REPLY HELLO CLIENT\n\r");
	len=strlen("UDP REPLY HELLO CLIENT\n\r")+sizeof(udpPacketPtr);
	udpPkt->len=be16toword(len);
	udpPkt->cksum=0;
	udpPkt->cksum=udpChecksum((uint8_t*)udpPkt-8,len+8);
	ipReSend(frame,len+sizeof(ipPktPtr));*/
	arpAdd(ipPkt->ipaddrSrc,frame->addrSrc);
	udpSend((uint8_t*)"UDP REPLY\n\r",strlen("UDP REPLY\n\r"),ipPkt->ipaddrSrc,be16toword(udpPkt->portSrc),be16toword(udpPkt->portDst));
	return res;
}

uint8_t udpRead(enc28j60FramePtr *frame, uint16_t len)
{
	uint8_t res=0;
	ipPktPtr *ipPkt = (void*)frame->data;
	udpPacketPtr *udpPkt = (void*)ipPkt->data;
	//sprintf(str2,"%u-%u\r\n", be16toword(udpPkt->portSrc),be16toword(udpPkt->portDst));
	//uart1Printf(str2);
	uart1Printf((char*)udpPkt->data);
	uart1Printf("\n\r");
	udpReply(frame);
	return res;
}