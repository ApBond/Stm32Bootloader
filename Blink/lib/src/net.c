#include "net.h"

uint8_t netBuf[ENC28J60_MAXFRAME];
extern uint8_t macaddr[6];
uint8_t ipaddr[4]=IP_ADDR;
uint8_t gateIp[4]={192,168,0,1};
uint8_t netMask[4] = {255,255,255,0};
char str1[60]={0};

void netInit()
{
	spi1Init();
	enc28j60Init();
}

uint16_t checksum(uint8_t *ptr, uint16_t len,IP_TYPE protocol)
{
	uint32_t sum=0;
	if(protocol==TCP)
	{
		sum+=IP_TCP;
		sum+=len-8;
	}
	while(len>1)
	{
		sum += (uint16_t)(((uint32_t)*ptr<<8)|*(ptr+1));
		ptr+=2;
		len-=2;
	}
	if(len) sum+=((uint32_t)*ptr)<<8;
	while (sum>>16) sum=(uint16_t)sum+(sum>>16);
	return ~be16toword((uint16_t)sum);
}

void ethReSend(enc28j60FramePtr *frame, uint16_t len)
{
  memcpy(frame->addrDest,frame->addrSrc,6);
  memcpy(frame->addrSrc,macaddr,6);
  enc28j60PacketSend((void*)frame,len + sizeof(enc28j60FramePtr));
}

void ethSend(enc28j60FramePtr *frame, uint16_t len)
{
	enc28j60PacketSend((void*)frame,len + sizeof(enc28j60FramePtr));
}

uint8_t ipReSend(enc28j60FramePtr *frame, uint16_t len)
{
	uint8_t res=0;
	ipPktPtr *ipPkt = (void*)frame->data;
	//Заполним заголовок пакета IP
	ipPkt->len=be16toword(len);
	ipPkt->flFrgOf=0;
	ipPkt->ttl=128;
	ipPkt->cs = 0;
	memcpy(ipPkt->ipaddrDst,ipPkt->ipaddrSrc,4);
	memcpy(ipPkt->ipaddrSrc,ipaddr,4);
	ipPkt->cs = checksum((void*)ipPkt,sizeof(ipPktPtr),IP);
	ethReSend(frame,len);
	return res;
}

IP_STATUS ipSend(enc28j60FramePtr *frame, uint16_t len,IP_TYPE protocol)
{
	ipPktPtr *ipPkt = (void*)frame->data;
	uint8_t* mac;
	uint8_t i;
	for(i=0;i<4;i++)
	{
		if(((ipPkt->ipaddrDst[i]^ipaddr[i])&netMask[i])!=0)
		{
			memcpy(ipPkt->ipaddrDst,gateIp,4);
			break;
		}
	}
	mac=arpResolve(ipPkt->ipaddrDst);
	if(mac!=0)
	{
		memcpy(frame->addrDest,mac,6);
		memcpy(frame->addrSrc,macaddr,6);
		memcpy(ipPkt->ipaddrSrc,ipaddr,4);
		ipPkt->verlen=0x45;
		ipPkt->ts=0;
		ipPkt->len=be16toword(len+sizeof(ipPktPtr));
		ipPkt->flFrgOf=0;
		ipPkt->ttl=0x08;
		ipPkt->prt=protocol;
		ipPkt->id=0;
		ipPkt->cs = 0;
		ipPkt->cs = checksum((void*)ipPkt,sizeof(ipPktPtr),IP);
		ethSend(frame,len+sizeof(ipPktPtr));
		return SEND_COMPL;
	}
	else
	{
		return SEND_ERROR;
	}
}

uint8_t icmpRead(enc28j60FramePtr *frame, uint16_t len)
{
	uint8_t res=0;
	ipPktPtr *ipPkt = (void*)frame->data;
	icmpPktPtr *icmpPkt = (void*)ipPkt->data;
	if ((len>=sizeof(icmpPktPtr))&&(icmpPkt->msgTp==ICMP_REQ))
	{
		icmpPkt->msgTp=ICMP_REPLY;
		icmpPkt->cs=0;
		icmpPkt->cs=checksum((void*)icmpPkt,len,ICMP);
		ipReSend(frame,len+sizeof(ipPktPtr));
		
	}
  return res;
}

uint8_t ipRead(enc28j60FramePtr *frame, uint16_t len)
{
	uint8_t res=0;
	ipPktPtr *ipPkt = (void*)(frame->data);
	if((ipPkt->verlen==0x45)&&(!memcmp(ipPkt->ipaddrDst,ipaddr,4)))
  {
		len = be16toword(ipPkt->len) - sizeof(ipPktPtr);
		if (ipPkt->prt==IP_ICMP)
		{
			icmpRead(frame,len);
		}
		else if (ipPkt->prt==IP_TCP)
		{
			//tcpRead(frame,len);
			tcpPool(frame);
		}
  }
  return res;
}


void ethRead(enc28j60FramePtr *frame, uint16_t len)
{
	if (len>=sizeof(enc28j60FramePtr))
  {
		if(frame->type==ETH_ARP)
		{
			arpFilter(frame,len-sizeof(enc28j60FramePtr));
		}
		else if(frame->type==ETH_IP)
		{
			ipRead(frame,len-sizeof(ipPktPtr));
		}
  }
}

void netPool()
{
	uint16_t len;
  enc28j60FramePtr *frame=(void*)netBuf;
	if((len=enc28j60PacketReceive(netBuf,sizeof(netBuf)))>0)
  {
    ethRead(frame,len);
  }
	
}