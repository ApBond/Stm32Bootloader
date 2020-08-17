#include "tcp.h"

extern uint8_t netBuf[ENC28J60_MAXFRAME];
tcpState tcpStateConn[TCP_MAX_CONNECTION];

uint8_t tcpCreate(uint16_t portSrc)
{
	uint8_t i;
	for(i=0;i<TCP_MAX_CONNECTION;i++)
	{
		if(tcpStateConn[i].id==0)
		{
			tcpStateConn[i].srcPort=portSrc;
			tcpStateConn[i].state=TCP_CLOSED;
			tcpStateConn[i].id=rand()+1;
			tcpStateConn[i].poolStat=NOT_POOL;
			return tcpStateConn[i].id;
		}
	}
	return 0;
}

void tcpClose(uint8_t id)
{
	uint8_t i;
	enc28j60FramePtr* frame = (void*)netBuf;
	ipPktPtr *ipPkt = (void*)(frame->data);
	tcpPktPtr *tcpPkt = (void*)(ipPkt->data);
	for(i=0;i<TCP_MAX_CONNECTION;i++)
	{
		if(tcpStateConn[i].id==id  && tcpStateConn[i].state==TCP_ESTABLISHED)
		{
			tcpPkt->portDst = be16toword(tcpStateConn[i].dstPort);
			tcpPkt->portSrc = be16toword(tcpStateConn[i].srcPort);
			tcpPkt->numAsk = be32todword(tcpStateConn[i].numAsk);
			tcpPkt->btNumSeg = be32todword(tcpStateConn[i].btNumSeg);
			tcpPkt->fl = TCP_ACK | TCP_FIN;
			tcpPkt->sizeWnd = be16toword(TCP_MAX_PKT_SIZE);
			tcpPkt->urgPtr = 0;
			tcpPkt->lenHdr = sizeof(tcpPktPtr)<<2;
			tcpPkt->cs = 0;
			tcpPkt->cs=checksum((uint8_t*)tcpPkt-8, sizeof(tcpPktPtr)+8, TCP);
			memcpy(ipPkt->ipaddrDst,tcpStateConn[i].dstIp,4);
			ipSend(frame,sizeof(tcpPktPtr),TCP);
			tcpStateConn[i].state=TCP_FIN_WAIT;
		}
	}
}

void tcpListen(uint8_t id)
{
	uint8_t i;
	for(i=0;i<TCP_MAX_CONNECTION;i++)
	{
		if(tcpStateConn[i].id==id)
		{
			tcpStateConn[i].poolStat=POOL;
		}
	}
}

void tcpSendData(uint8_t id,uint8_t* data,uint16_t len)
{
	uint8_t i;
	enc28j60FramePtr* frame = (void*)netBuf;
	ipPktPtr *ipPkt = (void*)(frame->data);
	tcpPktPtr *tcpPkt = (void*)(ipPkt->data);
	for(i=0;i<TCP_MAX_CONNECTION;i++)
	{
		if(tcpStateConn[i].id==id && tcpStateConn[i].state==TCP_ESTABLISHED)
		{
			tcpPkt->portDst = be16toword(tcpStateConn[i].dstPort);
			tcpPkt->portSrc = be16toword(tcpStateConn[i].srcPort);
			tcpPkt->numAsk = be32todword(tcpStateConn[i].numAsk);
			tcpPkt->btNumSeg = be32todword(tcpStateConn[i].btNumSeg);
			tcpStateConn[i].btNumSeg += len;
			tcpPkt->fl = TCP_ACK | TCP_PSH;
			tcpPkt->sizeWnd = be16toword(TCP_MAX_PKT_SIZE);
			tcpPkt->urgPtr = 0;
			tcpPkt->lenHdr = sizeof(tcpPktPtr)<<2;
			memcpy(tcpPkt->data,data,len);
			len+=sizeof(tcpPktPtr);
			tcpPkt->cs = 0;
			tcpPkt->cs=checksum((uint8_t*)tcpPkt-8, len+8, TCP);
			memcpy(ipPkt->ipaddrDst,tcpStateConn[i].dstIp,4);
			ipSend(frame,len,TCP);
		}
	}
}

uint8_t* tcpRead(uint8_t id,uint16_t* len)
{
	uint8_t i;
	for(i=0;i<TCP_MAX_CONNECTION;i++)
	{
		if(tcpStateConn[i].id==id && tcpStateConn[i].resStat==RECIVED)
		{
			tcpStateConn[i].resStat=NOT_RECIVE;
			*len=tcpStateConn[i].tcpDataLen;
			return tcpStateConn[i].data;
		}
	}
	return 0;
}

void tcpFilter(uint8_t connectNumber,enc28j60FramePtr *frame)
{
	ipPktPtr *ipPkt = (void*)(frame->data);
	tcpPktPtr *tcpPkt = (void*)(ipPkt->data);
	static uint32_t numSeg=0;
	uint16_t len = be16toword(ipPkt->len)-20-(tcpPkt->lenHdr>>2);
	if(tcpPkt->fl == (TCP_ACK|TCP_RST))
	{
		tcpStateConn[connectNumber].state=TCP_CLOSED;
		return;
	}
	switch(tcpStateConn[connectNumber].state)
	{
		case TCP_CLOSED:
			if(tcpPkt->fl == TCP_SYN)
			{
				tcpStateConn[connectNumber].dstPort=be16toword(tcpPkt->portSrc);
				memcpy(tcpStateConn[connectNumber].dstIp,ipPkt->ipaddrSrc,4);
				tcpPkt->portDst = be16toword(tcpStateConn[connectNumber].dstPort);
				tcpPkt->portSrc = be16toword(tcpStateConn[connectNumber].srcPort);
				tcpPkt->numAsk = be32todword(be32todword(tcpPkt->btNumSeg) + 1);
				tcpPkt->btNumSeg = be32todword(rand());
				tcpStateConn[connectNumber].btNumSeg=be32todword(tcpPkt->btNumSeg);
				tcpStateConn[connectNumber].numAsk=be32todword(tcpPkt->numAsk);
				tcpPkt->fl = TCP_SYN | TCP_ACK;
				tcpPkt->sizeWnd = be16toword(TCP_MAX_PKT_SIZE);
				tcpPkt->urgPtr = 0;
				len = sizeof(tcpPktPtr)+4;
				tcpPkt->lenHdr = len << 2;
				tcpPkt->data[0]=2;//Maximum Segment Size (2)
				tcpPkt->data[1]=4;//Length
				tcpPkt->data[2]=0x05;
				tcpPkt->data[3]=0x82;
				tcpPkt->cs = 0;
				tcpPkt->cs=checksum((uint8_t*)tcpPkt-8, len+8, TCP);
				arpAdd(tcpStateConn[connectNumber].dstIp,frame->addrSrc);
				memcpy(ipPkt->ipaddrDst,tcpStateConn[connectNumber].dstIp,4);
				ipSend(frame,len,TCP);
				tcpStateConn[connectNumber].state=TCP_SYN_RECEIVED;
				tcpStateConn[connectNumber].btNumSeg++;
			}
			break;
		case TCP_SYN_RECEIVED:
			if(tcpPkt->fl == TCP_ACK)
			{
				tcpStateConn[connectNumber].state=TCP_ESTABLISHED;
			}
			break;
		case TCP_FIN_WAIT:
			if(tcpPkt->fl == (TCP_FIN|TCP_ACK))
			{
				tcpPkt->portDst = be16toword(tcpStateConn[connectNumber].dstPort);
				tcpPkt->portSrc = be16toword(tcpStateConn[connectNumber].srcPort);
				numSeg = tcpPkt->numAsk;
				tcpPkt->numAsk = be32todword(be32todword(tcpPkt->btNumSeg) + 1);
				tcpPkt->btNumSeg = numSeg;
				tcpPkt->fl = TCP_ACK;
				tcpPkt->sizeWnd = be16toword(TCP_MAX_PKT_SIZE);
				tcpPkt->urgPtr = 0;
				len = sizeof(tcpPktPtr);
				tcpPkt->lenHdr = len << 2;
				tcpPkt->cs = 0;
				tcpPkt->cs=checksum((uint8_t*)tcpPkt-8, len+8, TCP);
				memcpy(ipPkt->ipaddrDst,tcpStateConn[connectNumber].dstIp,4);
				ipSend(frame,len,TCP);
				tcpStateConn[connectNumber].state=TCP_CLOSED;
			}
			break;
		case TCP_ESTABLISHED:
			if(tcpPkt->fl == (TCP_PSH|TCP_ACK))
			{
				if(len>0)
				{
					tcpStateConn[connectNumber].resStat=RECIVED;
					tcpStateConn[connectNumber].tcpDataLen = len;
					tcpStateConn[connectNumber].data = tcpPkt->data;
				}
				tcpPkt->portDst = be16toword(tcpStateConn[connectNumber].dstPort);
				tcpPkt->portSrc = be16toword(tcpStateConn[connectNumber].srcPort);
				tcpStateConn[connectNumber].numAsk+=len;
				tcpPkt->numAsk = be32todword(tcpStateConn[connectNumber].numAsk);
				tcpPkt->btNumSeg = be32todword(tcpStateConn[connectNumber].btNumSeg);
				tcpPkt->fl = TCP_ACK;
				tcpPkt->sizeWnd = be16toword(TCP_MAX_PKT_SIZE);
				tcpPkt->urgPtr = 0;
				len = sizeof(tcpPktPtr);
				tcpPkt->lenHdr = len << 2;
				tcpPkt->cs = 0;
				tcpPkt->cs=checksum((uint8_t*)tcpPkt-8, len+8, TCP);
				memcpy(ipPkt->ipaddrDst,tcpStateConn[connectNumber].dstIp,4);
				ipSend(frame,len,TCP);
			}
			else if(tcpPkt->fl == (TCP_FIN|TCP_ACK))
			{
				tcpPkt->portDst = be16toword(tcpStateConn[connectNumber].dstPort);
				tcpPkt->portSrc = be16toword(tcpStateConn[connectNumber].srcPort);
				numSeg = tcpPkt->numAsk;
				tcpPkt->numAsk = be32todword(be32todword(tcpPkt->btNumSeg) + 1);
				tcpPkt->btNumSeg = numSeg;
				tcpPkt->fl = TCP_ACK;
				tcpPkt->sizeWnd = be16toword(TCP_MAX_PKT_SIZE);
				tcpPkt->urgPtr = 0;
				len = sizeof(tcpPktPtr);
				tcpPkt->lenHdr = len << 2;
				tcpPkt->cs = 0;
				tcpPkt->cs=checksum((uint8_t*)tcpPkt-8, len+8, TCP);
				memcpy(ipPkt->ipaddrDst,tcpStateConn[connectNumber].dstIp,4);
				ipSend(frame,len,TCP);
				tcpPkt->fl = TCP_FIN|TCP_ACK;
				len = sizeof(tcpPktPtr);
				tcpPkt->cs = 0;
				tcpPkt->cs=checksum((uint8_t*)tcpPkt-8, len+8,TCP);
				ipSend(frame,len,TCP);
				tcpStateConn[connectNumber].state=TCP_CLOSED;
			}
			break;
	}
}

void tcpPool(enc28j60FramePtr *frame)
{
	uint8_t i;
	ipPktPtr *ipPkt = (void*)(frame->data);
	tcpPktPtr *tcpPkt = (void*)(ipPkt->data);
	for(i=0;i<TCP_MAX_CONNECTION;i++)
	{
		if(tcpStateConn[i].poolStat==POOL\
		&& tcpStateConn[i].srcPort==be16toword(tcpPkt->portDst))
		{
			tcpFilter(i,frame);
		}
	}
}

TCP_STAT_CODE getTcpStatus(uint8_t id)
{
	uint8_t i;
	for(i=0;i<TCP_MAX_CONNECTION;i++)
	{
		if(tcpStateConn[i].id==id)
		{
			return tcpStateConn[i].state;
		}
	}
}