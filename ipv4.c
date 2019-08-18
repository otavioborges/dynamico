/*
 * ipv4.c
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#include "ipv4.h"
#include "udp.h"

uint32_t IPV4_OWN_IP	= 0;
uint32_t IPV4_NETMASK	= 0;

void IPV4_DefineIP(uint32_t ip, uint32_t netmask){
	IPV4_OWN_IP = ip;
	IPV4_NETMASK = netmask;
}

uint32_t IPV4_GetOwnIP(void){
	return IPV4_OWN_IP;
}

uint32_t IPV4_GetNetmask(void){
	return IPV4_NETMASK;
}

static uint32_t IPV4_CalculateSubnetBroadcast(void){
	return IPV4_OWN_IP | ~IPV4_NETMASK;
}

int IPV4_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *sourceMAC, uint8_t *reply){
	ipv4_header_t *ipv4;
	ipv4_header_t *replyIpv4;
	uint8_t headerLength = 0;

	ipv4 = (ipv4_header_t *)msg;
	if((ipv4->versionIHL >> 4) != 4)
		return ETH_ERROR_WRONG_IP_VERSION;	// TODO: support ipv6

	headerLength = (ipv4->versionIHL & 0x0F) * 4;

	replyIpv4->versionIHL = ipv4->versionIHL;
	replyIpv4->type = ipv4->type;
	replyIpv4->identification = ipv4->identification;
	replyIpv4->flagsOffset = 0;
	replyIpv4->timeToLive = ipv4->timeToLive;
	replyIpv4->protocol = IPv4_PROTOCOL_UDP;
	replyIpv4->checksum = 0;
	replyIpv4->sourceAddr = REVERT_32_BITS(IPV4_OWN_IP);

#if(DYN_CONF_ACCEPT_IPv4_BROADCAST == 1)
	if((ipv4->destAddr == IPV4_IP_BROADCAST) || (ipv4->destAddr == IPV4_OWN_IP)){
#else
	if(ipv4->destAddr == IPV4_OWN_IP){
#endif
		uint8_t *payload = (reply + headerLength);
		int responseLength = 0;
		uint32_t replyOnBroadcast = 0;
		msg += headerLength;
		length -= headerLength;

		if(ipv4->protocol == IPv4_PROTOCOL_UDP){
			responseLength = UDP_ProcessPacket(msg, length, payload, sourceMAC, &replyOnBroadcast);
		}else if(ipv4->protocol == IPv4_PROTOCOL_TCP){
			responseLength = 0;	// TODO: implement TCP
		}else{
			return ETH_ERROR_PROT_NOT_SUPPORTED;
		}

		if(responseLength == 0){
			return 0;
		}else{
			if(replyOnBroadcast == IPV4_REPLY_ON_BROADCAST_ALL)
				replyIpv4->destAddr = IPV4_IP_BROADCAST;
			else if(replyOnBroadcast == IPV4_REPLY_ON_BROADCAST_SUBNET)
				replyIpv4->destAddr = REVERT_32_BITS(IPV4_CalculateSubnetBroadcast());
			else if(replyOnBroadcast == IPV4_REPLY_TO_SENDER)
				replyIpv4->destAddr = ipv4->sourceAddr;
			else	// reply to DHCP new IP
				replyIpv4->destAddr = REVERT_32_BITS(replyOnBroadcast);

			replyIpv4->length = REVERT_16BITS(responseLength + PACKET_IPv4_LENGTH);
			return (responseLength + headerLength);
		}
	}else{
		return ETH_ERROR_WRONG_DEST_IP;
	}
}
